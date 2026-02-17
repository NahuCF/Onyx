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

    class Model
    {
    public:
        Model(const char *path, bool loadTextures = true)
            : m_LoadTextures(loadTextures)
        {
            LoadModel(path);
        }
        ~Model();

        void Draw(Onyx::Shader &shader);

        std::vector<Mesh>& GetMeshes() { return m_Meshes; }
        const std::vector<Mesh>& GetMeshes() const { return m_Meshes; }

        void BuildMergedBuffers();
        const MergedBuffers& GetMergedBuffers() const { return m_Merged; }
        bool HasMergedBuffers() const { return m_Merged.vao != nullptr; }

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