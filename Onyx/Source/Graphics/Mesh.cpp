#include "pch.h"

#include "Mesh.h"

namespace Onyx {

	Mesh::Mesh(std::vector<MeshVertex> vertices, std::vector<unsigned int> indices, std::vector<MeshTexture> textures, const std::string& name)
		: m_Name(name), m_Vertices(std::move(vertices)), m_Indices(std::move(indices)), m_Textures(std::move(textures))
	{
		SetupMesh();
		ComputeBounds();
	}

	Mesh::Mesh(std::vector<MeshVertex> vertices, std::vector<unsigned int> indices, std::vector<MeshTexture> textures, const std::string& name, bool deferGPU)
		: m_Name(name), m_Vertices(std::move(vertices)), m_Indices(std::move(indices)), m_Textures(std::move(textures))
	{
		ComputeBounds();
		if (!deferGPU)
		{
			SetupMesh();
		}
	}

	Mesh::Mesh(const std::string& name, const glm::vec3& boundsMin, const glm::vec3& boundsMax)
		: m_Name(name), m_BoundsMin(boundsMin), m_BoundsMax(boundsMax), m_Center((boundsMin + boundsMax) * 0.5f)
	{
	}

	void Mesh::EnsureGPU()
	{
		if (!m_GPUReady && !m_Vertices.empty())
		{
			SetupMesh();
		}
	}

	void Mesh::Draw(Shader& shader)
	{
		EnsureGPU();

		unsigned int diffuseNr = 1;
		unsigned int specularNr = 1;

		for (unsigned int i = 0; i < m_Textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);

			std::string number;
			std::string name = m_Textures[i].type;
			if (name == "diffuse")
				number = std::to_string(diffuseNr++);
			else if (name == "specular")
				number = std::to_string(specularNr++);

			shader.SetInt("material." + name + number, i);
			glBindTexture(GL_TEXTURE_2D, m_Textures[i].id);
		}

		glActiveTexture(GL_TEXTURE0);

		glBindVertexArray(m_VAO);
		glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
	}

	void Mesh::DrawGeometryOnly()
	{
		EnsureGPU();

		glBindVertexArray(m_VAO);
		glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
	}

	Mesh::~Mesh()
	{
		if (m_VAO)
			glDeleteVertexArrays(1, &m_VAO);
		if (m_VBO)
			glDeleteBuffers(1, &m_VBO);
		if (m_EBO)
			glDeleteBuffers(1, &m_EBO);
	}

	Mesh::Mesh(Mesh&& other) noexcept
		: m_Name(std::move(other.m_Name)), m_Vertices(std::move(other.m_Vertices)), m_Indices(std::move(other.m_Indices)), m_Textures(std::move(other.m_Textures)), m_VAO(other.m_VAO), m_VBO(other.m_VBO), m_EBO(other.m_EBO), m_GPUReady(other.m_GPUReady), m_BoundsMin(other.m_BoundsMin), m_BoundsMax(other.m_BoundsMax), m_Center(other.m_Center)
	{
		other.m_VAO = 0;
		other.m_VBO = 0;
		other.m_EBO = 0;
		other.m_GPUReady = false;
	}

	void Mesh::ComputeBounds()
	{
		if (m_Vertices.empty())
			return;
		m_BoundsMin = glm::vec3(std::numeric_limits<float>::max());
		m_BoundsMax = glm::vec3(std::numeric_limits<float>::lowest());
		for (const auto& v : m_Vertices)
		{
			glm::vec3 p(v.position[0], v.position[1], v.position[2]);
			m_BoundsMin = glm::min(m_BoundsMin, p);
			m_BoundsMax = glm::max(m_BoundsMax, p);
		}
		m_Center = (m_BoundsMin + m_BoundsMax) * 0.5f;
	}

	void Mesh::SetupMesh()
	{
		glGenVertexArrays(1, &m_VAO);
		glGenBuffers(1, &m_VBO);
		glGenBuffers(1, &m_EBO);

		glBindVertexArray(m_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

		glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(MeshVertex), &m_Vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), &m_Indices[0], GL_STATIC_DRAW);

		// Position (location 0): float3
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, position));
		// Normal (location 1): snorm16x2 oct-encoded
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_SHORT, GL_TRUE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, octNormal));
		// TexCoord (location 2): half2
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_HALF_FLOAT, GL_FALSE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, uvHalf));
		// Tangent (location 3): snorm16x2 oct-encoded
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 2, GL_SHORT, GL_TRUE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, octTangent));
		// BitangentSign (location 4): snorm16x2; shader uses .x as sign, .y reserved
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 2, GL_SHORT, GL_TRUE, sizeof(MeshVertex), (void*)offsetof(MeshVertex, bitangentSign));

		glBindVertexArray(0);

		m_GPUReady = true;
	}

} // namespace Onyx