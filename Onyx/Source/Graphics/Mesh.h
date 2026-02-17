#pragma once

#include <string>
#include <vector>

#include "Shader.h"
#include "Buffers.h"
#include "Maths/Maths.h"

namespace Onyx {

    struct MeshVertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    struct MeshTexture 
    {
        unsigned int id;
        std::string type;
        std::string path;
    };  

    class Mesh
    {
    public:
        Mesh(std::vector<MeshVertex> vertices, std::vector<unsigned int> indices, std::vector<MeshTexture> textures, const std::string& name = "");
        ~Mesh();

        std::string m_Name;
        std::vector<MeshVertex> m_Vertices;
        std::vector<unsigned int> m_Indices;
        std::vector<MeshTexture> m_Textures;

        void Draw(Shader &shader);
        void DrawGeometryOnly();  // Draw without rebinding textures

        const glm::vec3& GetBoundsMin() const { return m_BoundsMin; }
        const glm::vec3& GetBoundsMax() const { return m_BoundsMax; }
        const glm::vec3& GetCenter() const { return m_Center; }

    private:
        void SetupMesh();
        void ComputeBounds();

        unsigned int m_VAO, m_VBO, m_EBO;
        glm::vec3 m_BoundsMin{0.0f};
        glm::vec3 m_BoundsMax{0.0f};
        glm::vec3 m_Center{0.0f};
    };

}