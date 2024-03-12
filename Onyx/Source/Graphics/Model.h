#pragma once

#include <string>
#include <vector>

#include "assimp/Importer.hpp"
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"

namespace Onyx {

    class Model 
    {
    public:
        Model(const char *path)
        {
            LoadModel(path);
        }
        void Draw(Onyx::Shader &shader);	
    private:
        void LoadModel(std::string path);
        void ProcessNode(aiNode *node, const aiScene *scene);
        Mesh ProcessMesh(aiMesh *mesh, const aiScene *scene);
        std::vector<MeshTexture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);

        std::vector<Mesh> m_Meshes;
        std::vector<MeshTexture> m_UsedTextures;
        std::string m_Directory;

    };

}