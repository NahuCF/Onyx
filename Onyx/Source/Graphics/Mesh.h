#pragma once

#include <string>
#include <vector>

#include "Buffers.h"
#include "Maths/Maths.h"
#include "Shader.h"

namespace Onyx {

	struct MeshVertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoord;
		glm::vec3 tangent;
		glm::vec3 bitangent;

		static VertexLayout GetLayout()
		{
			VertexLayout layout;
			layout.PushFloat(3); // Position  (location 0)
			layout.PushFloat(3); // Normal    (location 1)
			layout.PushFloat(2); // TexCoord  (location 2)
			layout.PushFloat(3); // Tangent   (location 3)
			layout.PushFloat(3); // Bitangent (location 4)
			return layout;
		}
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
		Mesh(std::vector<MeshVertex> vertices, std::vector<unsigned int> indices, std::vector<MeshTexture> textures, const std::string& name, bool deferGPU);
		Mesh(const std::string& name, const glm::vec3& boundsMin, const glm::vec3& boundsMax);
		~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh(Mesh&& other) noexcept;
		Mesh& operator=(Mesh&& other) = delete;

		std::string m_Name;
		std::vector<MeshVertex> m_Vertices;
		std::vector<unsigned int> m_Indices;
		std::vector<MeshTexture> m_Textures;

		void Draw(Shader& shader);
		void DrawGeometryOnly(); // Draw without rebinding textures

		const glm::vec3& GetBoundsMin() const { return m_BoundsMin; }
		const glm::vec3& GetBoundsMax() const { return m_BoundsMax; }
		const glm::vec3& GetCenter() const { return m_Center; }

	private:
		void SetupMesh();
		void EnsureGPU();
		void ComputeBounds();

		unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;
		bool m_GPUReady = false;
		glm::vec3 m_BoundsMin{0.0f};
		glm::vec3 m_BoundsMax{0.0f};
		glm::vec3 m_Center{0.0f};
	};

} // namespace Onyx