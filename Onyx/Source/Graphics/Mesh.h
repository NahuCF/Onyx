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
        Mesh(std::vector<MeshVertex> vertices, std::vector<unsigned int> indices, std::vector<MeshTexture> textures);
        ~Mesh();

        std::vector<MeshVertex> m_Vertices;
        std::vector<unsigned int> m_Indices;
        std::vector<MeshTexture> m_Textures;

        void Draw(Shader &shader);
    private:
        void SetupMesh();

		//VertexArray* m_VAO;
		//VertexBuffer* m_VBO;
		//IndexBuffer* m_EBO;
        unsigned int m_VAO, m_VBO, m_EBO;
    };

}