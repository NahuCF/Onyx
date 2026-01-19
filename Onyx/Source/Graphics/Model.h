#pragma once

#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>

#include "Mesh.h"

namespace Onyx {

    class Model
    {
    public:
        // loadTextures: if false, skips loading embedded textures (geometry only)
        Model(const char *path, bool loadTextures = true)
            : m_LoadTextures(loadTextures)
        {
            LoadModel(path);
        }
        void Draw(Onyx::Shader &shader);

        // Access meshes for custom rendering
        std::vector<Mesh>& GetMeshes() { return m_Meshes; }
        const std::vector<Mesh>& GetMeshes() const { return m_Meshes; }
    private:
        void LoadModel(std::string path);
        void ProcessNode(aiNode *node, const aiScene *scene, const glm::mat4& parentTransform);
        Mesh ProcessMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4& transform, const std::string& name = "");
        std::vector<MeshTexture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);

        std::vector<Mesh> m_Meshes;
        std::vector<MeshTexture> m_UsedTextures;
        std::string m_Directory;
        bool m_LoadTextures = true;

    };

}