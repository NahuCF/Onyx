#pragma once

#include <string>
#include <vector>
#include <memory>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>

#include "Mesh.h"
#include "Buffers.h"
#include "VertexLayout.h"

namespace Onyx {

    struct MergedMeshInfo {
        uint32_t indexCount;
        uint32_t firstIndex;
        int32_t  baseVertex;
    };

    struct MergedBuffers {
        std::unique_ptr<VertexArray> vao;
        std::unique_ptr<VertexBuffer> vbo;
        std::unique_ptr<IndexBuffer> ebo;
        uint32_t totalVertices = 0;
        uint32_t totalIndices = 0;
        std::vector<MergedMeshInfo> meshInfos;
    };

    struct CpuMeshData {
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<MeshTexture> texturePaths;  // paths stored, id=0 (no GL texture yet)
        std::string name;
    };

    struct MeshBoundsInfo {
        std::string name;
        glm::vec3 boundsMin{0.0f};
        glm::vec3 boundsMax{0.0f};
    };

    class Model
    {
    public:
        Model(const char *path, bool loadTextures = true)
            : m_LoadTextures(loadTextures)
        {
            LoadModel(path);
        }

        // Construct from pre-parsed CPU data (GPU upload only — must be called on main thread)
        Model(std::vector<CpuMeshData>&& meshData, const std::string& directory);

        // Lightweight constructor for async staged upload — creates bounds-only Mesh objects.
        // No vertex data stored, no ComputeBounds iteration. MergedBuffers set separately.
        Model(const std::string& directory, const std::vector<MeshBoundsInfo>& meshBounds);

        ~Model();

        void Draw(Onyx::Shader &shader);

        // Draw a single mesh via merged buffers (non-batched, for picking/wireframe)
        void DrawMergedMesh(size_t meshIndex) const;
        // Draw all meshes via merged buffers (non-batched, for spawn rendering)
        void DrawAllMergedMeshes() const;

        std::vector<Mesh>& GetMeshes() { return m_Meshes; }
        const std::vector<Mesh>& GetMeshes() const { return m_Meshes; }

        void BuildMergedBuffers();
        void SetMergedBuffers(MergedBuffers&& merged) { m_Merged = std::move(merged); }
        const MergedBuffers& GetMergedBuffers() const { return m_Merged; }
        bool HasMergedBuffers() const { return m_Merged.vao != nullptr; }

        static std::vector<CpuMeshData> ParseFromFile(const std::string& path, std::string& outDirectory, bool loadTexturePaths = true);

    private:
        void LoadModel(std::string path);
        void ProcessNode(aiNode *node, const aiScene *scene, const glm::mat4& parentTransform);
        Mesh ProcessMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4& transform, const std::string& name = "");
        std::vector<MeshTexture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);

        std::vector<Mesh> m_Meshes;
        std::vector<MeshTexture> m_UsedTextures;
        std::string m_Directory;
        bool m_LoadTextures = true;

        MergedBuffers m_Merged;
    };

}