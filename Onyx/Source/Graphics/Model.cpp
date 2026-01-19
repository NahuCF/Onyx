#include "Model.h"

#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_STATIC
#include <stb_image.h>

namespace Onyx {

    unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma = false);

    // Convert Assimp matrix to GLM matrix
    static glm::mat4 AiMatrixToGlm(const aiMatrix4x4& from)
    {
        glm::mat4 to;
        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
        return to;
    }

    void Onyx::Model::LoadModel(std::string path)
    {
        std::cout << "[Model] Loading: " << path << std::endl;

        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace);
        // Note: NOT using aiProcess_PreTransformVertices to preserve separate meshes
        // Each mesh will need its node transform applied manually
        // Note: NOT using aiProcess_FlipUVs because Texture class already flips with stbi

        if(!scene)
        {
            std::cout << "ERROR::ASSIMP:: Failed to load: " << path << std::endl;
            std::cout << "  Reason: " << import.GetErrorString() << std::endl;
            return;
        }

        // Print what Assimp found even if scene is incomplete
        std::cout << "[Model] Scene loaded with flags: " << scene->mFlags << std::endl;
        std::cout << "[Model]   Meshes: " << scene->mNumMeshes << std::endl;
        std::cout << "[Model]   Materials: " << scene->mNumMaterials << std::endl;
        std::cout << "[Model]   Textures: " << scene->mNumTextures << std::endl;
        std::cout << "[Model]   Animations: " << scene->mNumAnimations << std::endl;
        std::cout << "[Model]   Lights: " << scene->mNumLights << std::endl;
        std::cout << "[Model]   Cameras: " << scene->mNumCameras << std::endl;
        std::cout << "[Model]   Has root node: " << (scene->mRootNode ? "yes" : "no") << std::endl;

        if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cout << "ERROR::ASSIMP:: Scene is incomplete or has no root node" << std::endl;
            return;
        }

        std::cout << "[Model] Scene has " << scene->mNumMeshes << " meshes total" << std::endl;

        m_Directory = path.substr(0, path.find_last_of('/'));

        ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f));

        std::cout << "[Model] Loaded " << m_Meshes.size() << " meshes from " << path << std::endl;
    }

    void Onyx::Model::Draw(Onyx::Shader &shader) 
    {
        for(unsigned int i = 0; i < m_Meshes.size(); i++)
            m_Meshes[i].Draw(shader);
    }

    void Onyx::Model::ProcessNode(aiNode *node, const aiScene *scene, const glm::mat4& parentTransform)
    {
        // Accumulate the transform from parent
        glm::mat4 nodeTransform = AiMatrixToGlm(node->mTransformation);
        glm::mat4 globalTransform = parentTransform * nodeTransform;

        std::cout << "[Model] Processing node: '" << node->mName.C_Str()
                  << "' with " << node->mNumMeshes << " meshes, "
                  << node->mNumChildren << " children" << std::endl;

        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            // Use node name if mesh name is empty or generic (Blender exports object names as node names)
            std::string meshName = mesh->mName.C_Str();
            std::string nodeName = node->mName.C_Str();
            if (meshName.empty() || meshName.find("Mesh") == 0) {
                meshName = nodeName;  // Use node name instead
            }
            std::cout << "[Model]   Mesh[" << m_Meshes.size() << "]: node='" << nodeName
                      << "' mesh='" << mesh->mName.C_Str() << "' -> using name: '" << meshName
                      << "' (" << mesh->mNumVertices << " verts)" << std::endl;
            m_Meshes.push_back(ProcessMesh(mesh, scene, globalTransform, meshName));
        }

        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene, globalTransform);
        }

    }

    Mesh Onyx::Model::ProcessMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4& transform, const std::string& name)
    {
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<MeshTexture> textures;
        // Use provided name, or fall back to mesh name from Assimp
        std::string meshName = name.empty() ? mesh->mName.C_Str() : name;

        // Normal matrix for transforming normals (inverse transpose of upper-left 3x3)
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

        // Set vertices and textures coords for the mesh
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            // Set Vertex vertices position
            MeshVertex vertex;
            glm::vec3 localPos(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            // Apply transform to position
            glm::vec4 transformedPos = transform * glm::vec4(localPos, 1.0f);
            vertex.position = glm::vec3(transformedPos);

            // Set Vertex normals
            if (mesh->HasNormals())
            {
                glm::vec3 localNormal(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                // Apply normal matrix to normal
                vertex.normal = glm::normalize(normalMatrix * localNormal);
            }

            // Set Vertex texture coords
            if(mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.texCoord = vec;
            }
            else
            {
                vertex.texCoord = glm::vec2(0.0f, 0.0f);
            }

            // Set Vertex tangent
            if (mesh->HasTangentsAndBitangents())
            {
                glm::vec3 localTangent(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                glm::vec3 localBitangent(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
                // Apply normal matrix to tangent/bitangent
                vertex.tangent = glm::normalize(normalMatrix * localTangent);
                vertex.bitangent = glm::normalize(normalMatrix * localBitangent);
            }
            else
            {
                vertex.tangent = glm::vec3(0.0f);
                vertex.bitangent = glm::vec3(0.0f);
            }

            vertices.push_back(vertex);
        } 

        // Set indices of mesh
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }  

        // process materials (only if texture loading is enabled)
        if (m_LoadTextures) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
            // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
            // Same applies to other texture as the following list summarizes:
            // diffuse: texture_diffuseN
            // specular: texture_specularN
            // normal: texture_normalN

            // 1. diffuse maps
            std::vector<MeshTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            // 2. specular maps
            std::vector<MeshTexture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            // 3. normal maps
            std::vector<MeshTexture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            // 4. height maps
            std::vector<MeshTexture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
            textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        }

        return Onyx::Mesh(vertices, indices, textures, meshName);
    }

    std::vector<MeshTexture> Onyx::Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName) 
    {
        std::vector<MeshTexture> textures;
        
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            bool skip = false;

            for(unsigned int j = 0; j < m_UsedTextures.size(); j++)
            {
                if(std::strcmp(m_UsedTextures[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(m_UsedTextures[j]);
                    skip = true; 
                    break;
                }
            }

            if(!skip)
            {  
                MeshTexture texture;
                texture.id = TextureFromFile(str.C_Str(), m_Directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                m_UsedTextures.push_back(texture);
            }
        }

        return textures;
    }

    unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma)
    {
        std::string filename = std::string(path);
        filename = directory + '/' + filename;

		stbi_set_flip_vertically_on_load(true);
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }

}