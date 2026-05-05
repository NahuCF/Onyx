// Benchmark: FBX vs current .omdl (v1) vs proposed .omdl (v2).
//
// v1 = current engine format: 56 B/vertex (pos3+normal3+uv2+tangent3+bitangent3),
//      uint32 indices, no welding. Read via MMO::ReadOmdl (fstream into vectors).
//
// v2 = proposed format implemented inline in this benchmark:
//      - aiProcess_JoinIdenticalVertices on import (welds duplicate corners).
//      - 24 B/vertex: pos float3 (12) + oct-snorm16 normal (4) + half2 uv (4) +
//        oct-snorm16 tangent + sign packed in tangent.w bit (4). Bitangent dropped
//        and reconstructed in shader as cross(N,T)*sign.
//      - uint16 indices when totalVerts < 65536, else uint32.
//      - Load path: memory-mapped file, zero-copy pointers into the mapping
//        (no std::vector allocation, no fread copy).

#include "Model/OmdlReader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

using Clock = std::chrono::steady_clock;
using ms = std::chrono::duration<double, std::milli>;

namespace {

struct MeshVertex56 {
    float pos[3];
    float normal[3];
    float uv[2];
    float tangent[3];
    float bitangent[3];
};
static_assert(sizeof(MeshVertex56) == 56, "vertex layout mismatch");

#pragma pack(push, 1)
struct QuantVertex24 {
    float    pos[3];     // 12
    int16_t  octN[2];    //  4   normal: oct-encoded, snorm16x2
    uint16_t uvHalf[2];  //  4   uv: half2
    int16_t  octT[2];    //  4   tangent oct-snorm16x2; LSB of octT[0] holds sign(bitangent)
};
static_assert(sizeof(QuantVertex24) == 24, "quant vertex layout mismatch");
#pragma pack(pop)

constexpr uint32_t OMDL_V2_MAGIC   = 0x324C444Fu; // "OD2 "
constexpr uint32_t OMDL_V2_VERSION = 2;
constexpr uint32_t OMDL_V2_FLAG_U16_INDICES = 1u << 0;

#pragma pack(push, 1)
struct OmdlV2Header {
    uint32_t magic;
    uint32_t version;
    uint32_t flags;
    uint32_t meshCount;
    uint32_t totalVertices;
    uint32_t totalIndices;
    float    boundsMin[3];
    float    boundsMax[3];
};
struct OmdlV2MeshInfo {
    uint32_t indexCount;
    uint32_t firstIndex;
    int32_t  baseVertex;
    float    boundsMin[3];
    float    boundsMax[3];
};
#pragma pack(pop)

// ---------------------------------------------------------------------------
// Encoding helpers

static uint16_t FloatToHalf(float f) {
    // IEEE 754 fp32 -> fp16, round-to-nearest-even, no NaN/Inf finesse.
    uint32_t x;
    std::memcpy(&x, &f, 4);
    uint32_t sign = (x >> 16) & 0x8000u;
    int32_t  exp  = static_cast<int32_t>((x >> 23) & 0xFFu) - 127 + 15;
    uint32_t mant =  x & 0x7FFFFFu;
    if (exp <= 0) {
        if (exp < -10) return static_cast<uint16_t>(sign);
        mant = (mant | 0x800000u) >> (1 - exp);
        if (mant & 0x1000u) mant += 0x2000u;
        return static_cast<uint16_t>(sign | (mant >> 13));
    }
    if (exp >= 31) return static_cast<uint16_t>(sign | 0x7C00u);
    if (mant & 0x1000u) {
        mant += 0x2000u;
        if (mant & 0x800000u) { mant = 0; exp++; if (exp >= 31) return static_cast<uint16_t>(sign | 0x7C00u); }
    }
    return static_cast<uint16_t>(sign | (static_cast<uint32_t>(exp) << 10) | (mant >> 13));
}

static void OctEncode(const float n[3], int16_t out[2]) {
    float ax = std::fabs(n[0]), ay = std::fabs(n[1]), az = std::fabs(n[2]);
    float L = ax + ay + az;
    if (L < 1e-20f) L = 1.0f;
    float ox = n[0] / L;
    float oy = n[1] / L;
    if (n[2] < 0.0f) {
        float tx = (1.0f - std::fabs(oy)) * (ox >= 0 ? 1.0f : -1.0f);
        float ty = (1.0f - std::fabs(ox)) * (oy >= 0 ? 1.0f : -1.0f);
        ox = tx; oy = ty;
    }
    auto enc = [](float v) {
        v = std::max(-1.0f, std::min(1.0f, v));
        return static_cast<int16_t>(std::round(v * 32767.0f));
    };
    out[0] = enc(ox);
    out[1] = enc(oy);
}

// ---------------------------------------------------------------------------
// Memory-mapped reader: opens file, maps view, returns header + ptrs.

struct MappedFile {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap  = nullptr;
    void*  base  = nullptr;
    size_t size  = 0;
    ~MappedFile() {
        if (base) UnmapViewOfFile(base);
        if (hMap) CloseHandle(hMap);
        if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    }
};

static bool MapFile(const std::string& path, MappedFile& out) {
    out.hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (out.hFile == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz; if (!GetFileSizeEx(out.hFile, &sz)) return false;
    out.size = static_cast<size_t>(sz.QuadPart);
    out.hMap = CreateFileMappingA(out.hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!out.hMap) return false;
    out.base = MapViewOfFile(out.hMap, FILE_MAP_READ, 0, 0, 0);
    return out.base != nullptr;
}

// ---------------------------------------------------------------------------
// Common timing harness

struct LoadResult {
    double elapsedMs = 0.0;
    uint64_t vertexCount = 0;
    uint64_t indexCount = 0;
    uint64_t fileBytes = 0;
    size_t bytesPerVertex = 0;
    size_t bytesPerIndex = 0;
};

LoadResult LoadFbx(const std::string& path) {
    LoadResult r;
    r.fileBytes = std::filesystem::file_size(path);
    auto t0 = Clock::now();
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cerr << "FBX load failed: " << importer.GetErrorString() << "\n";
        return r;
    }
    std::vector<MeshVertex56> verts;
    std::vector<uint32_t> idx;
    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        const uint32_t base = static_cast<uint32_t>(verts.size());
        verts.reserve(verts.size() + mesh->mNumVertices);
        for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
            MeshVertex56 mv{};
            mv.pos[0]=mesh->mVertices[v].x; mv.pos[1]=mesh->mVertices[v].y; mv.pos[2]=mesh->mVertices[v].z;
            if (mesh->HasNormals()) { mv.normal[0]=mesh->mNormals[v].x; mv.normal[1]=mesh->mNormals[v].y; mv.normal[2]=mesh->mNormals[v].z; }
            if (mesh->mTextureCoords[0]) { mv.uv[0]=mesh->mTextureCoords[0][v].x; mv.uv[1]=mesh->mTextureCoords[0][v].y; }
            if (mesh->HasTangentsAndBitangents()) {
                mv.tangent[0]=mesh->mTangents[v].x;     mv.tangent[1]=mesh->mTangents[v].y;     mv.tangent[2]=mesh->mTangents[v].z;
                mv.bitangent[0]=mesh->mBitangents[v].x; mv.bitangent[1]=mesh->mBitangents[v].y; mv.bitangent[2]=mesh->mBitangents[v].z;
            }
            verts.push_back(mv);
        }
        idx.reserve(idx.size() + mesh->mNumFaces * 3);
        for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned k = 0; k < face.mNumIndices; ++k) idx.push_back(base + face.mIndices[k]);
        }
    }
    auto t1 = Clock::now();
    r.elapsedMs = ms(t1 - t0).count();
    r.vertexCount = verts.size();
    r.indexCount = idx.size();
    r.bytesPerVertex = 56;
    r.bytesPerIndex = 4;
    return r;
}

LoadResult LoadOmdlV1(const std::string& path) {
    LoadResult r;
    r.fileBytes = std::filesystem::file_size(path);
    auto t0 = Clock::now();
    MMO::OmdlMapped data;
    if (!MMO::ReadOmdl(path, data)) { std::cerr << "OMDL v1 load failed\n"; return r; }
    // Zero-copy reader: ReadOmdl only walks the header. To make this comparable to
    // the FBX path (which produces ready-to-use bytes) and to the v2 bench path
    // (which faults the pages explicitly), force the OS to fault the blob pages.
    volatile uint8_t sink = 0;
    const uint8_t* vb = static_cast<const uint8_t*>(data.vertexData);
    for (size_t off = 0; off < data.vertexBytes; off += 4096) sink ^= vb[off];
    if (data.vertexBytes > 0) sink ^= vb[data.vertexBytes - 1];
    const uint8_t* ib = static_cast<const uint8_t*>(data.indexData);
    for (size_t off = 0; off < data.indexBytes; off += 4096) sink ^= ib[off];
    if (data.indexBytes > 0) sink ^= ib[data.indexBytes - 1];
    (void)sink;
    auto t1 = Clock::now();
    r.elapsedMs = ms(t1 - t0).count();
    r.vertexCount = data.header.totalVertices;
    r.indexCount = data.header.totalIndices;
    const bool u16 = (data.header.flags & MMO::OMDL_FLAG_U16_INDICES) != 0;
    r.bytesPerVertex = MMO::OMDL_VERTEX_BYTES;
    r.bytesPerIndex = u16 ? 2 : 4;
    return r;
}

// v2 reader: mmap + verify magic + grab pointers. Zero copies.
LoadResult LoadOmdlV2Mapped(const std::string& path) {
    LoadResult r;
    r.fileBytes = std::filesystem::file_size(path);
    auto t0 = Clock::now();
    MappedFile mf;
    if (!MapFile(path, mf)) { std::cerr << "OMDL v2 mmap failed\n"; return r; }
    const auto* hdr = static_cast<const OmdlV2Header*>(mf.base);
    if (hdr->magic != OMDL_V2_MAGIC || hdr->version != OMDL_V2_VERSION) {
        std::cerr << "OMDL v2 bad magic/version\n";
        return r;
    }
    const uint8_t* p = static_cast<const uint8_t*>(mf.base) + sizeof(OmdlV2Header);
    const auto* meshes = reinterpret_cast<const OmdlV2MeshInfo*>(p);
    p += sizeof(OmdlV2MeshInfo) * hdr->meshCount;
    const QuantVertex24* verts = reinterpret_cast<const QuantVertex24*>(p);
    p += sizeof(QuantVertex24) * hdr->totalVertices;
    const void* indices = p;
    // Touch one byte from each blob so the OS faults the pages — fair vs fread which
    // forces the read. Without this, mmap could "succeed" without ever touching disk.
    volatile uint8_t sink = 0;
    const uint8_t* vb = reinterpret_cast<const uint8_t*>(verts);
    const size_t vbBytes = static_cast<size_t>(hdr->totalVertices) * sizeof(QuantVertex24);
    const size_t step = 4096; // OS page size
    for (size_t off = 0; off < vbBytes; off += step) sink ^= vb[off];
    if (vbBytes > 0) sink ^= vb[vbBytes - 1];
    const size_t ibBytes = static_cast<size_t>(hdr->totalIndices) *
        ((hdr->flags & OMDL_V2_FLAG_U16_INDICES) ? 2 : 4);
    const uint8_t* ib = static_cast<const uint8_t*>(indices);
    for (size_t off = 0; off < ibBytes; off += step) sink ^= ib[off];
    if (ibBytes > 0) sink ^= ib[ibBytes - 1];
    (void)sink;
    auto t1 = Clock::now();
    r.elapsedMs = ms(t1 - t0).count();
    r.vertexCount = hdr->totalVertices;
    r.indexCount = hdr->totalIndices;
    r.bytesPerVertex = 24;
    r.bytesPerIndex = (hdr->flags & OMDL_V2_FLAG_U16_INDICES) ? 2 : 4;
    (void)meshes;
    return r;
}

// One-time export of FBX -> OMDL v2 (welded + quantized + u16 idx if possible)
bool ExportV2(const std::string& fbxPath, const std::string& outPath) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        fbxPath,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        std::cerr << "v2 export FBX load failed: " << importer.GetErrorString() << "\n"; return false;
    }
    std::vector<QuantVertex24> verts;
    std::vector<uint32_t> idx32;
    std::vector<OmdlV2MeshInfo> meshes;
    float bMin[3] = { 1e30f, 1e30f, 1e30f };
    float bMax[3] = { -1e30f, -1e30f, -1e30f };
    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        OmdlV2MeshInfo mi{};
        mi.firstIndex = static_cast<uint32_t>(idx32.size());
        mi.baseVertex = static_cast<int32_t>(verts.size());
        float mMin[3] = { 1e30f, 1e30f, 1e30f };
        float mMax[3] = { -1e30f, -1e30f, -1e30f };
        for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
            QuantVertex24 qv{};
            qv.pos[0]=mesh->mVertices[v].x; qv.pos[1]=mesh->mVertices[v].y; qv.pos[2]=mesh->mVertices[v].z;
            for (int k = 0; k < 3; ++k) {
                if (qv.pos[k] < mMin[k]) mMin[k] = qv.pos[k];
                if (qv.pos[k] > mMax[k]) mMax[k] = qv.pos[k];
            }
            float n[3] = {0,1,0};
            if (mesh->HasNormals()) { n[0]=mesh->mNormals[v].x; n[1]=mesh->mNormals[v].y; n[2]=mesh->mNormals[v].z; }
            OctEncode(n, qv.octN);
            float u = 0, vv = 0;
            if (mesh->mTextureCoords[0]) { u = mesh->mTextureCoords[0][v].x; vv = mesh->mTextureCoords[0][v].y; }
            qv.uvHalf[0] = FloatToHalf(u); qv.uvHalf[1] = FloatToHalf(vv);
            float t[3] = {1,0,0};
            float biSign = 1.0f;
            if (mesh->HasTangentsAndBitangents()) {
                t[0]=mesh->mTangents[v].x; t[1]=mesh->mTangents[v].y; t[2]=mesh->mTangents[v].z;
                // Sign of (cross(N, T) . B)
                float cx = n[1]*t[2]-n[2]*t[1];
                float cy = n[2]*t[0]-n[0]*t[2];
                float cz = n[0]*t[1]-n[1]*t[0];
                float dot = cx*mesh->mBitangents[v].x + cy*mesh->mBitangents[v].y + cz*mesh->mBitangents[v].z;
                biSign = (dot < 0.0f) ? -1.0f : 1.0f;
            }
            OctEncode(t, qv.octT);
            // Encode sign bit: force LSB of octT[0] to 0/1 (cheap; one quant step of error in tangent.x).
            if (biSign < 0) qv.octT[0] = static_cast<int16_t>(qv.octT[0] | 1);
            else            qv.octT[0] = static_cast<int16_t>(qv.octT[0] & ~1);
            verts.push_back(qv);
        }
        for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned k = 0; k < face.mNumIndices; ++k) idx32.push_back(static_cast<uint32_t>(mi.baseVertex) + face.mIndices[k]);
        }
        mi.indexCount = static_cast<uint32_t>(idx32.size()) - mi.firstIndex;
        std::memcpy(mi.boundsMin, mMin, sizeof(mMin));
        std::memcpy(mi.boundsMax, mMax, sizeof(mMax));
        for (int k = 0; k < 3; ++k) {
            if (mMin[k] < bMin[k]) bMin[k] = mMin[k];
            if (mMax[k] > bMax[k]) bMax[k] = mMax[k];
        }
        meshes.push_back(mi);
    }

    OmdlV2Header hdr{};
    hdr.magic = OMDL_V2_MAGIC;
    hdr.version = OMDL_V2_VERSION;
    hdr.meshCount = static_cast<uint32_t>(meshes.size());
    hdr.totalVertices = static_cast<uint32_t>(verts.size());
    hdr.totalIndices  = static_cast<uint32_t>(idx32.size());
    bool useU16 = hdr.totalVertices < 65536u;
    hdr.flags = useU16 ? OMDL_V2_FLAG_U16_INDICES : 0u;
    std::memcpy(hdr.boundsMin, bMin, sizeof(bMin));
    std::memcpy(hdr.boundsMax, bMax, sizeof(bMax));

    std::ofstream f(outPath, std::ios::binary);
    if (!f.is_open()) return false;
    f.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    f.write(reinterpret_cast<const char*>(meshes.data()), sizeof(OmdlV2MeshInfo) * meshes.size());
    f.write(reinterpret_cast<const char*>(verts.data()), sizeof(QuantVertex24) * verts.size());
    if (useU16) {
        std::vector<uint16_t> idx16(idx32.size());
        for (size_t i = 0; i < idx32.size(); ++i) idx16[i] = static_cast<uint16_t>(idx32[i]);
        f.write(reinterpret_cast<const char*>(idx16.data()), 2 * idx16.size());
    } else {
        f.write(reinterpret_cast<const char*>(idx32.data()), 4 * idx32.size());
    }
    return f.good();
}

// ---------------------------------------------------------------------------
// Output

void Print(const char* label, const LoadResult& r) {
    std::cout << "  " << std::left << std::setw(14) << label << " | "
              << std::right << std::setw(11) << std::fixed << std::setprecision(3) << r.elapsedMs << " ms"
              << " | file=" << std::setw(11) << r.fileBytes << " B"
              << " | verts=" << std::setw(8) << r.vertexCount
              << " | idx=" << std::setw(8) << r.indexCount
              << " | " << r.bytesPerVertex << "B/v "
              << r.bytesPerIndex << "B/i"
              << "\n";
}

LoadResult RunBest(const char* label, const std::string& path,
                   LoadResult (*fn)(const std::string&), int runs) {
    LoadResult best; best.elapsedMs = 1e18;
    double sum = 0.0;
    for (int i = 0; i < runs; ++i) {
        LoadResult r = fn(path);
        sum += r.elapsedMs;
        if (r.elapsedMs < best.elapsedMs) best = r;
        std::cout << "  [" << label << " run " << (i + 1) << "] "
                  << std::fixed << std::setprecision(3) << r.elapsedMs << " ms\n";
    }
    std::cout << "  [" << label << "] avg=" << (sum / runs) << " ms  best=" << best.elapsedMs << " ms\n";
    return best;
}

void RunModel(const std::string& fbxPath, const std::string& v1Path) {
    std::cout << "===============================================================\n";
    std::cout << "Model: " << fbxPath << "\n";
    std::cout << "===============================================================\n";

    constexpr int kRuns = 5;

    // Build v2 once (not timed)
    std::string v2Path = fbxPath + ".v2.omdl";
    std::cout << "[setup] Exporting v2 (welded + quantized + u16 idx)...\n";
    if (!ExportV2(fbxPath, v2Path)) {
        std::cerr << "[setup] v2 export failed\n";
        return;
    }
    std::cout << "[setup] v2 written: " << v2Path << " ("
              << std::filesystem::file_size(v2Path) << " B)\n\n";

    std::cout << "--- FBX (Assimp full pipeline + walk into MeshVertex56) ---\n";
    LoadResult fbx = RunBest("fbx", fbxPath, LoadFbx, kRuns);

    LoadResult v1{};
    if (!v1Path.empty() && std::filesystem::exists(v1Path)) {
        std::cout << "\n--- OMDL v1 (current engine, fstream + 56B verts) ---\n";
        v1 = RunBest("omdl v1", v1Path, LoadOmdlV1, kRuns);
    }

    std::cout << "\n--- OMDL v2 (mmap + 24B quantized verts + u16 idx) ---\n";
    LoadResult v2 = RunBest("omdl v2", v2Path, LoadOmdlV2Mapped, kRuns);

    std::cout << "\nSummary (best of " << kRuns << "):\n";
    Print("FBX",     fbx);
    if (v1.elapsedMs > 0) Print("OMDL v1",   v1);
    Print("OMDL v2", v2);

    if (v2.elapsedMs > 0) {
        std::cout << "\n  v2 vs FBX:    " << std::fixed << std::setprecision(2)
                  << (fbx.elapsedMs / v2.elapsedMs) << "x faster\n";
        if (v1.elapsedMs > 0) {
            std::cout << "  v2 vs v1:     " << std::fixed << std::setprecision(2)
                      << (v1.elapsedMs / v2.elapsedMs) << "x faster\n";
            std::cout << "  v2 file/v1:   " << std::fixed << std::setprecision(2)
                      << (100.0 * static_cast<double>(v2.fileBytes) / v1.fileBytes) << "% of v1 size\n";
        }
        std::cout << "  v2 file/FBX:  " << std::fixed << std::setprecision(2)
                  << (100.0 * static_cast<double>(v2.fileBytes) / fbx.fileBytes) << "% of FBX size\n";
        std::cout << "  vert dedup:   " << std::fixed << std::setprecision(2)
                  << (100.0 * static_cast<double>(v2.vertexCount) / fbx.vertexCount)
                  << "% of pre-weld vert count\n";
    }
    std::cout << "\n";
}

}

int main() {
    RunModel("C:/Users/god/Desktop/Onyx/MMOGame/assets/models/CelticIdol/Celtic_Idol.fbx",
             "C:/Users/god/Desktop/Onyx/Data/models/Celtic_Idol.omdl");

    RunModel("C:/Users/god/Desktop/Onyx/MMOGame/assets/models/testJunto2.fbx",
             "C:/Users/god/Desktop/Onyx/Data/models/testJunto2.omdl");
    return 0;
}
