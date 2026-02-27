#include "TerrainMeshGenerator.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

namespace MMO {

void GenerateTerrainMesh(const TerrainChunkData& data,
                         const TerrainMeshOptions& options,
                         TerrainMeshData& out) {
    if (data.heightmap.empty()) return;

    const int dataRes = TERRAIN_CHUNK_RESOLUTION;
    const int dataQuads = dataRes - 1;
    const int meshRes = options.meshResolution;
    const int meshQuads = meshRes - 1;

    int totalCornerVerts = meshRes * meshRes;
    int totalCenterVerts = options.diamondGrid ? (meshQuads * meshQuads) : 0;
    int totalVerts = totalCornerVerts + totalCenterVerts;
    int trisPerQuad = options.diamondGrid ? 4 : 2;

    out.vertices.clear();
    out.vertices.reserve(totalVerts * 8);
    out.indices.clear();
    out.indices.reserve(meshQuads * meshQuads * trisPerQuad * 3);

    float meshStep = TERRAIN_CHUNK_SIZE / static_cast<float>(meshQuads);
    float dataStep = TERRAIN_CHUNK_SIZE / static_cast<float>(dataQuads);

    // Height sampling with cross-chunk boundary support
    auto sampleHeightInt = [&](int sx, int sz) -> float {
        if (sx >= 0 && sx < dataRes && sz >= 0 && sz < dataRes) {
            return data.heightmap[sz * dataRes + sx];
        }
        if (options.heightSampler) {
            return options.heightSampler(data.chunkX, data.chunkZ, sx, sz);
        }
        sx = std::clamp(sx, 0, dataRes - 1);
        sz = std::clamp(sz, 0, dataRes - 1);
        return data.heightmap[sz * dataRes + sx];
    };

    // Bilinear interpolation for variable-resolution meshes
    auto sampleHeightBilinear = [&](float fx, float fz) -> float {
        fx = std::clamp(fx, 0.0f, static_cast<float>(dataRes - 1));
        fz = std::clamp(fz, 0.0f, static_cast<float>(dataRes - 1));
        int x0 = std::min(static_cast<int>(fx), dataRes - 2);
        int z0 = std::min(static_cast<int>(fz), dataRes - 2);
        float tx = fx - x0;
        float tz = fz - z0;
        float h00 = data.heightmap[z0 * dataRes + x0];
        float h10 = data.heightmap[z0 * dataRes + x0 + 1];
        float h01 = data.heightmap[(z0 + 1) * dataRes + x0];
        float h11 = data.heightmap[(z0 + 1) * dataRes + x0 + 1];
        return h00 * (1 - tx) * (1 - tz) + h10 * tx * (1 - tz) +
               h01 * (1 - tx) * tz + h11 * tx * tz;
    };

    // --- Compute normals at data resolution ---
    const int dataTotalVerts = dataRes * dataRes;
    std::vector<glm::vec3> dataNormals(dataTotalVerts);

    for (int z = 0; z < dataRes; z++) {
        for (int x = 0; x < dataRes; x++) {
            glm::vec3 normal;

            if (options.sobelNormals) {
                float h00 = sampleHeightInt(x - 1, z - 1);
                float h10 = sampleHeightInt(x,     z - 1);
                float h20 = sampleHeightInt(x + 1, z - 1);
                float h01 = sampleHeightInt(x - 1, z    );
                float h21 = sampleHeightInt(x + 1, z    );
                float h02 = sampleHeightInt(x - 1, z + 1);
                float h12 = sampleHeightInt(x,     z + 1);
                float h22 = sampleHeightInt(x + 1, z + 1);

                float gx = -h00 + h20 - 2.0f * h01 + 2.0f * h21 - h02 + h22;
                float gz =  h00 + 2.0f * h10 + h20 - h02 - 2.0f * h12 - h22;

                normal = glm::normalize(glm::vec3(-gx, 8.0f * dataStep, -gz));
            } else {
                float hL = sampleHeightInt(x - 1, z);
                float hR = sampleHeightInt(x + 1, z);
                float hD = sampleHeightInt(x, z - 1);
                float hU = sampleHeightInt(x, z + 1);

                normal = glm::normalize(glm::vec3(hL - hR, 2.0f * dataStep, hD - hU));
            }

            dataNormals[z * dataRes + x] = normal;
        }
    }

    // --- Optional 5-tap smooth filter ---
    if (options.smoothNormals) {
        auto computeSobelAt = [&](int sx, int sz) -> glm::vec3 {
            float h00 = sampleHeightInt(sx - 1, sz - 1);
            float h10 = sampleHeightInt(sx,     sz - 1);
            float h20 = sampleHeightInt(sx + 1, sz - 1);
            float h01 = sampleHeightInt(sx - 1, sz    );
            float h21 = sampleHeightInt(sx + 1, sz    );
            float h02 = sampleHeightInt(sx - 1, sz + 1);
            float h12 = sampleHeightInt(sx,     sz + 1);
            float h22 = sampleHeightInt(sx + 1, sz + 1);
            float gx = -h00 + h20 - 2.0f * h01 + 2.0f * h21 - h02 + h22;
            float gz =  h00 + 2.0f * h10 + h20 - h02 - 2.0f * h12 - h22;
            return glm::normalize(glm::vec3(-gx, 8.0f * dataStep, -gz));
        };

        auto getNormal = [&](int nx, int nz) -> glm::vec3 {
            if (nx >= 0 && nx < dataRes && nz >= 0 && nz < dataRes)
                return dataNormals[nz * dataRes + nx];
            return computeSobelAt(nx, nz);
        };

        std::vector<glm::vec3> smoothed(dataTotalVerts);
        for (int z = 0; z < dataRes; z++) {
            for (int x = 0; x < dataRes; x++) {
                glm::vec3 sum = dataNormals[z * dataRes + x];
                sum += getNormal(x - 1, z);
                sum += getNormal(x + 1, z);
                sum += getNormal(x, z - 1);
                sum += getNormal(x, z + 1);
                smoothed[z * dataRes + x] = glm::normalize(sum / 5.0f);
            }
        }
        dataNormals = std::move(smoothed);
    }

    // Bilinear normal interpolation for variable-resolution meshes
    auto sampleNormalBilinear = [&](float fx, float fz) -> glm::vec3 {
        fx = std::clamp(fx, 0.0f, static_cast<float>(dataRes - 1));
        fz = std::clamp(fz, 0.0f, static_cast<float>(dataRes - 1));
        int x0 = std::min(static_cast<int>(fx), dataRes - 2);
        int z0 = std::min(static_cast<int>(fz), dataRes - 2);
        float tx = fx - x0;
        float tz = fz - z0;
        glm::vec3 n00 = dataNormals[z0 * dataRes + x0];
        glm::vec3 n10 = dataNormals[z0 * dataRes + x0 + 1];
        glm::vec3 n01 = dataNormals[(z0 + 1) * dataRes + x0];
        glm::vec3 n11 = dataNormals[(z0 + 1) * dataRes + x0 + 1];
        return glm::normalize(n00 * (1 - tx) * (1 - tz) + n10 * tx * (1 - tz) +
                              n01 * (1 - tx) * tz + n11 * tx * tz);
    };

    // --- Build corner vertices ---
    float worldOriginX = data.chunkX * TERRAIN_CHUNK_SIZE;
    float worldOriginZ = data.chunkZ * TERRAIN_CHUNK_SIZE;

    for (int z = 0; z < meshRes; z++) {
        for (int x = 0; x < meshRes; x++) {
            float hx = x * static_cast<float>(dataQuads) / meshQuads;
            float hz = z * static_cast<float>(dataQuads) / meshQuads;

            float px = worldOriginX + x * meshStep;
            float pz = worldOriginZ + z * meshStep;
            float py = sampleHeightBilinear(hx, hz);

            out.vertices.push_back(px);
            out.vertices.push_back(py);
            out.vertices.push_back(pz);

            glm::vec3 n = sampleNormalBilinear(hx, hz);
            out.vertices.push_back(n.x);
            out.vertices.push_back(n.y);
            out.vertices.push_back(n.z);

            out.vertices.push_back(static_cast<float>(x) / meshQuads);
            out.vertices.push_back(static_cast<float>(z) / meshQuads);
        }
    }

    // --- Build diamond center vertices (optional) ---
    if (options.diamondGrid) {
        for (int z = 0; z < meshQuads; z++) {
            for (int x = 0; x < meshQuads; x++) {
                float hx = (x + 0.5f) * static_cast<float>(dataQuads) / meshQuads;
                float hz = (z + 0.5f) * static_cast<float>(dataQuads) / meshQuads;

                float cx = worldOriginX + (x + 0.5f) * meshStep;
                float cz = worldOriginZ + (z + 0.5f) * meshStep;
                float cy = sampleHeightBilinear(hx, hz);

                out.vertices.push_back(cx);
                out.vertices.push_back(cy);
                out.vertices.push_back(cz);

                glm::vec3 cn = sampleNormalBilinear(hx, hz);
                out.vertices.push_back(cn.x);
                out.vertices.push_back(cn.y);
                out.vertices.push_back(cn.z);

                out.vertices.push_back((x + 0.5f) / meshQuads);
                out.vertices.push_back((z + 0.5f) / meshQuads);
            }
        }
    }

    // --- Build indices (skip holes) ---
    for (int z = 0; z < meshQuads; z++) {
        for (int x = 0; x < meshQuads; x++) {
            int holeX = std::min(x * TERRAIN_HOLE_GRID_SIZE / meshQuads, TERRAIN_HOLE_GRID_SIZE - 1);
            int holeZ = std::min(z * TERRAIN_HOLE_GRID_SIZE / meshQuads, TERRAIN_HOLE_GRID_SIZE - 1);
            uint64_t holeBit = 1ULL << (holeZ * TERRAIN_HOLE_GRID_SIZE + holeX);
            if (data.holeMask & holeBit) continue;

            uint32_t TL = z * meshRes + x;
            uint32_t TR = TL + 1;
            uint32_t BL = (z + 1) * meshRes + x;
            uint32_t BR = BL + 1;

            if (options.diamondGrid) {
                uint32_t C = totalCornerVerts + z * meshQuads + x;

                out.indices.push_back(TL); out.indices.push_back(C);  out.indices.push_back(TR);
                out.indices.push_back(TR); out.indices.push_back(C);  out.indices.push_back(BR);
                out.indices.push_back(BR); out.indices.push_back(C);  out.indices.push_back(BL);
                out.indices.push_back(BL); out.indices.push_back(C);  out.indices.push_back(TL);
            } else {
                out.indices.push_back(TL); out.indices.push_back(BL); out.indices.push_back(TR);
                out.indices.push_back(TR); out.indices.push_back(BL); out.indices.push_back(BR);
            }
        }
    }

    out.indexCount = static_cast<uint32_t>(out.indices.size());

    // --- Padded heightmap for shader normal computation (optional) ---
    if (options.generatePaddedHeightmap && !data.heightmap.empty()) {
        const int pRes = TERRAIN_CHUNK_RESOLUTION + 2;
        out.paddedHeightmapResolution = pRes;
        out.paddedHeightmap.resize(pRes * pRes);

        for (int z = 0; z < TERRAIN_CHUNK_RESOLUTION; z++)
            for (int x = 0; x < TERRAIN_CHUNK_RESOLUTION; x++)
                out.paddedHeightmap[(z + 1) * pRes + (x + 1)] = data.heightmap[z * TERRAIN_CHUNK_RESOLUTION + x];

        for (int z = -1; z <= TERRAIN_CHUNK_RESOLUTION; z++) {
            out.paddedHeightmap[(z + 1) * pRes + 0] = sampleHeightInt(-1, z);
            out.paddedHeightmap[(z + 1) * pRes + (pRes - 1)] = sampleHeightInt(TERRAIN_CHUNK_RESOLUTION, z);
        }
        for (int x = 0; x < TERRAIN_CHUNK_RESOLUTION; x++) {
            out.paddedHeightmap[0 * pRes + (x + 1)] = sampleHeightInt(x, -1);
            out.paddedHeightmap[(pRes - 1) * pRes + (x + 1)] = sampleHeightInt(x, TERRAIN_CHUNK_RESOLUTION);
        }
    }

    // --- Split splatmap into two RGBA textures ---
    if (!data.splatmap.empty()) {
        SplitSplatmapToRGBA(data.splatmap, out.splatmapRGBA0, out.splatmapRGBA1);
    }
}

} // namespace MMO
