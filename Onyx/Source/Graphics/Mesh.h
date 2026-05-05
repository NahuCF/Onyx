#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "Buffers.h"
#include "Maths/Maths.h"
#include "Shader.h"

namespace Onyx {

	// 28-byte GPU vertex (v2 OMDL layout). Normals/tangents are oct-encoded snorm16,
	// UVs are half2, bitangent is reconstructed in the vertex shader as cross(N,T)*sign.
	// Position stays full float3 because world coords need the precision.
	//
	// bitangentSign is snorm16x2 so the struct is a clean 28 B with no trailing pad —
	// matching attribute sizes is the simplest way to make VertexLayout's stride agree
	// with sizeof(MeshVertex). Only .x carries the sign; .y is reserved (always 0).
	struct MeshVertex
	{
		float    position[3];      // offset  0, 12 B  — float3
		int16_t  octNormal[2];     // offset 12,  4 B  — snorm16x2 oct-encoded normal
		uint16_t uvHalf[2];        // offset 16,  4 B  — half2 UV
		int16_t  octTangent[2];    // offset 20,  4 B  — snorm16x2 oct-encoded tangent
		int16_t  bitangentSign[2]; // offset 24,  4 B  — snorm16x2: .x ~ +/-1, .y reserved

		static VertexLayout GetLayout()
		{
			VertexLayout layout;
			layout.PushFloat(3);                                  // location 0: position
			layout.Push(VertexAttributeType::SNorm16x2, true);    // location 1: octNormal
			layout.Push(VertexAttributeType::Half2);              // location 2: uvHalf
			layout.Push(VertexAttributeType::SNorm16x2, true);    // location 3: octTangent
			layout.Push(VertexAttributeType::SNorm16x2, true);    // location 4: bitangentSign
			return layout;
		}
	};
	static_assert(sizeof(MeshVertex) == 28, "MeshVertex must be 28 bytes for the v2 OMDL layout");

	// IEEE-754 fp32 -> fp16 (half) with round-to-nearest-even.
	inline uint16_t FloatToHalf(float f)
	{
		uint32_t x;
		std::memcpy(&x, &f, 4);
		uint32_t sign = (x >> 16) & 0x8000u;
		int32_t  exp  = static_cast<int32_t>((x >> 23) & 0xFFu) - 127 + 15;
		uint32_t mant =  x & 0x7FFFFFu;
		if (exp <= 0)
		{
			if (exp < -10) return static_cast<uint16_t>(sign);
			mant = (mant | 0x800000u) >> (1 - exp);
			if (mant & 0x1000u) mant += 0x2000u;
			return static_cast<uint16_t>(sign | (mant >> 13));
		}
		if (exp >= 31) return static_cast<uint16_t>(sign | 0x7C00u);
		if (mant & 0x1000u)
		{
			mant += 0x2000u;
			if (mant & 0x800000u) { mant = 0; exp++; if (exp >= 31) return static_cast<uint16_t>(sign | 0x7C00u); }
		}
		return static_cast<uint16_t>(sign | (static_cast<uint32_t>(exp) << 10) | (mant >> 13));
	}

	// Octahedral encode of a unit-length vec3 to two snorm16 components.
	// Reference: Cigolle et al, "A Survey of Efficient Representations for Independent Unit Vectors", JCGT 2014.
	inline void OctEncodeNormal(const glm::vec3& n, int16_t out[2])
	{
		const float ax = std::fabs(n.x), ay = std::fabs(n.y), az = std::fabs(n.z);
		float L = ax + ay + az;
		if (L < 1e-20f) L = 1.0f;
		float ox = n.x / L;
		float oy = n.y / L;
		if (n.z < 0.0f)
		{
			float tx = (1.0f - std::fabs(oy)) * (ox >= 0 ? 1.0f : -1.0f);
			float ty = (1.0f - std::fabs(ox)) * (oy >= 0 ? 1.0f : -1.0f);
			ox = tx; oy = ty;
		}
		auto enc = [](float v) {
			if (v < -1.0f) v = -1.0f; else if (v > 1.0f) v = 1.0f;
			return static_cast<int16_t>(std::round(v * 32767.0f));
		};
		out[0] = enc(ox);
		out[1] = enc(oy);
	}

	// Build a quantized MeshVertex from raw float fields (already in world space).
	// Bitangent is only used to compute the sign; the shader reconstructs full
	// bitangent as cross(N,T) * sign(bitangentSign.x).
	inline MeshVertex MakeMeshVertex(const glm::vec3& position,
									 const glm::vec3& normal,
									 const glm::vec2& texCoord,
									 const glm::vec3& tangent,
									 const glm::vec3& bitangent)
	{
		MeshVertex v{};
		v.position[0] = position.x; v.position[1] = position.y; v.position[2] = position.z;
		OctEncodeNormal(normal,  v.octNormal);
		OctEncodeNormal(tangent, v.octTangent);
		v.uvHalf[0] = FloatToHalf(texCoord.x);
		v.uvHalf[1] = FloatToHalf(texCoord.y);
		const glm::vec3 c = glm::cross(normal, tangent);
		const float dot = c.x * bitangent.x + c.y * bitangent.y + c.z * bitangent.z;
		v.bitangentSign[0] = (dot < 0.0f) ? static_cast<int16_t>(-32767) : static_cast<int16_t>(32767);
		v.bitangentSign[1] = 0;
		return v;
	}

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