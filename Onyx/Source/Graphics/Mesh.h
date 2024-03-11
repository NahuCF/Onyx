#pragma once

#include <string>
#include <vector>

#include "Shader.h"
#include "Buffers.h"
#include "Maths/Maths.h"

namespace Onyx {

    struct Vertex 
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
    };

    struct Texture 
    {
        unsigned int id;
        std::string type;
    };  

    class Mesh
    {
    public:
        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
        ~Mesh();

        std::vector<Vertex> m_Vertices;
        std::vector<unsigned int> m_Indices;
        std::vector<Texture> m_Textures;

        void Draw(Shader &shader);
    private:
        void SetupMesh();

		//VertexArray* m_VAO;
		//VertexBuffer* m_VBO;
		//IndexBuffer* m_EBO;
        unsigned int m_VAO, m_VBO, m_EBO;
    };

}