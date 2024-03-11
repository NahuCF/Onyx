#include "pch.h"

#include "Mesh.h"

namespace Onyx {

    Mesh::Mesh(std::vector<MeshVertex> vertices, std::vector<unsigned int> indices, std::vector<MeshTexture> textures)
        : m_Vertices(vertices)
        , m_Indices(indices)
        , m_Textures(textures)
    {
        SetupMesh();
    }

    void Mesh::Draw(Shader &shader)
    {
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;

        for(unsigned int i = 0; i < m_Textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);

            std::string number;
            std::string name = m_Textures[i].type;
            if(name == "diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "specular")
                number = std::to_string(specularNr++);

            shader.SetInt(("material." + name + number).c_str(), i);
            glBindTexture(GL_TEXTURE_2D, m_Textures[i].id);
        }

        glActiveTexture(GL_TEXTURE0);

        glBindVertexArray(m_VAO);
        glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    Mesh::~Mesh() 
	{
		//delete m_VAO;
		//delete m_VBO;
		//delete m_EBO;
	}

    void Mesh::SetupMesh()
    {
        //this->m_VAO = new VertexArray();
        //this->m_VBO = new VertexBuffer()

        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);
        glGenBuffers(1, &m_EBO);
    
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

        glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(MeshVertex), &m_Vertices[0], GL_STATIC_DRAW);  

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int),  &m_Indices[0], GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)0);
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, normal));
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, texCoord));

        glBindVertexArray(0);
    }

}