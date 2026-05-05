#include "Model.h"

#include <functional>
#include <iostream>

#include "Buffers.h"
#include "VertexLayout.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_STATIC
#include <stb_image.h>

namespace Onyx {

	unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

	// Convert Assimp matrix to GLM matrix
	static glm::mat4 AiMatrixToGlm(const aiMatrix4x4& from)
	{
		glm::mat4 to;
		to[0][0] = from.a1;
		to[1][0] = from.a2;
		to[2][0] = from.a3;
		to[3][0] = from.a4;
		to[0][1] = from.b1;
		to[1][1] = from.b2;
		to[2][1] = from.b3;
		to[3][1] = from.b4;
		to[0][2] = from.c1;
		to[1][2] = from.c2;
		to[2][2] = from.c3;
		to[3][2] = from.c4;
		to[0][3] = from.d1;
		to[1][3] = from.d2;
		to[2][3] = from.d3;
		to[3][3] = from.d4;
		return to;
	}

	void Onyx::Model::LoadModel(std::string path)
	{
		Assimp::Importer import;
		const aiScene* scene = import.ReadFile(path,
											   aiProcess_Triangulate |
												   aiProcess_GenSmoothNormals |
												   aiProcess_CalcTangentSpace);
		// Note: NOT using aiProcess_PreTransformVertices to preserve separate meshes
		// Each mesh will need its node transform applied manually
		// Note: NOT using aiProcess_FlipUVs because Texture class already flips with stbi

		if (!scene)
		{
			std::cout << "ERROR::ASSIMP:: Failed to load: " << path << std::endl;
			std::cout << "  Reason: " << import.GetErrorString() << std::endl;
			return;
		}

		if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "ERROR::ASSIMP:: Scene is incomplete or has no root node" << std::endl;
			return;
		}

		m_Directory = path.substr(0, path.find_last_of('/'));

		ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f));
	}

	Model::~Model() = default;

	// Construct from pre-parsed CPU data — defers per-mesh GL to first DrawGeometryOnly() call.
	// Only BuildMergedBuffers() (called after this) creates GL resources immediately.
	Model::Model(std::vector<CpuMeshData>&& meshData, const std::string& directory)
		: m_Directory(directory), m_LoadTextures(false)
	{
		for (auto& cpuMesh : meshData)
		{
			// Resolve texture IDs on main thread if paths were stored
			for (auto& tex : cpuMesh.texturePaths)
			{
				if (tex.id == 0 && !tex.path.empty())
				{
					tex.id = TextureFromFile(tex.path.c_str(), m_Directory);
				}
			}
			// deferGPU=true: skip SetupMesh(), GL created lazily on first Draw call
			m_Meshes.emplace_back(std::move(cpuMesh.vertices), std::move(cpuMesh.indices),
								  std::move(cpuMesh.texturePaths), cpuMesh.name, true);
		}
	}

	// Lightweight: creates bounds-only Mesh objects for async staged upload.
	// No vertex data stored — MergedBuffers will be set separately by AssetManager.
	Model::Model(const std::string& directory, const std::vector<MeshBoundsInfo>& meshBounds)
		: m_Directory(directory), m_LoadTextures(false)
	{
		m_Meshes.reserve(meshBounds.size());
		for (const auto& mb : meshBounds)
		{
			m_Meshes.emplace_back(mb.name, mb.boundsMin, mb.boundsMax);
		}
	}

	// Static: parse model file into CPU-only data (no GL calls, thread-safe)
	std::vector<CpuMeshData> Model::ParseFromFile(const std::string& path, std::string& outDirectory, bool loadTexturePaths)
	{
		std::vector<CpuMeshData> result;

		Assimp::Importer import;
		const aiScene* scene = import.ReadFile(path,
											   aiProcess_Triangulate |
												   aiProcess_GenSmoothNormals |
												   aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::cout << "ERROR::ASSIMP::ParseFromFile: Failed to load: " << path << std::endl;
			if (scene)
				std::cout << "  Reason: " << import.GetErrorString() << std::endl;
			return result;
		}

		outDirectory = path.substr(0, path.find_last_of('/'));

		// Recursive lambda to traverse node hierarchy
		std::function<void(aiNode*, const glm::mat4&)> processNode =
			[&](aiNode* node, const glm::mat4& parentTransform) {
				glm::mat4 nodeTransform = AiMatrixToGlm(node->mTransformation);
				glm::mat4 globalTransform = parentTransform * nodeTransform;

				for (unsigned int i = 0; i < node->mNumMeshes; i++)
				{
					aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
					std::string meshName = mesh->mName.C_Str();
					std::string nodeName = node->mName.C_Str();
					if (meshName.empty() || meshName.find("Mesh") == 0)
					{
						meshName = nodeName;
					}

					CpuMeshData cpuMesh;
					cpuMesh.name = meshName;

					glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(globalTransform)));

					for (unsigned int v = 0; v < mesh->mNumVertices; v++)
					{
						MeshVertex vertex;
						glm::vec3 localPos(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
						glm::vec4 transformedPos = globalTransform * glm::vec4(localPos, 1.0f);
						vertex.position = glm::vec3(transformedPos);

						if (mesh->HasNormals())
						{
							glm::vec3 localNormal(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z);
							vertex.normal = glm::normalize(normalMatrix * localNormal);
						}

						if (mesh->mTextureCoords[0])
						{
							vertex.texCoord = glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
						}
						else
						{
							vertex.texCoord = glm::vec2(0.0f);
						}

						if (mesh->HasTangentsAndBitangents())
						{
							glm::vec3 localTangent(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z);
							glm::vec3 localBitangent(mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z);
							vertex.tangent = glm::normalize(normalMatrix * localTangent);
							vertex.bitangent = glm::normalize(normalMatrix * localBitangent);
						}
						else
						{
							vertex.tangent = glm::vec3(0.0f);
							vertex.bitangent = glm::vec3(0.0f);
						}

						cpuMesh.vertices.push_back(vertex);
					}

					for (unsigned int f = 0; f < mesh->mNumFaces; f++)
					{
						aiFace& face = mesh->mFaces[f];
						for (unsigned int j = 0; j < face.mNumIndices; j++)
							cpuMesh.indices.push_back(face.mIndices[j]);
					}

					if (loadTexturePaths)
					{
						aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
						auto loadPaths = [&](aiTextureType type, const std::string& typeName) {
							for (unsigned int t = 0; t < material->GetTextureCount(type); t++)
							{
								aiString str;
								material->GetTexture(type, t, &str);
								MeshTexture tex;
								tex.id = 0; // no GL texture yet
								tex.type = typeName;
								tex.path = str.C_Str();
								cpuMesh.texturePaths.push_back(tex);
							}
						};
						loadPaths(aiTextureType_DIFFUSE, "texture_diffuse");
						loadPaths(aiTextureType_SPECULAR, "texture_specular");
						loadPaths(aiTextureType_HEIGHT, "texture_normal");
						loadPaths(aiTextureType_AMBIENT, "texture_height");
					}

					result.push_back(std::move(cpuMesh));
				}

				for (unsigned int i = 0; i < node->mNumChildren; i++)
				{
					processNode(node->mChildren[i], globalTransform);
				}
			};

		processNode(scene->mRootNode, glm::mat4(1.0f));
		return result;
	}

	void Model::BuildMergedBuffers()
	{
		if (m_Meshes.empty() || m_Merged.vao != nullptr)
			return;

		// Count totals
		uint32_t totalVerts = 0;
		uint32_t totalIndices = 0;
		for (const auto& mesh : m_Meshes)
		{
			totalVerts += static_cast<uint32_t>(mesh.m_Vertices.size());
			totalIndices += static_cast<uint32_t>(mesh.m_Indices.size());
		}

		// Concatenate vertex and index data, record per-mesh offsets
		std::vector<MeshVertex> allVertices;
		std::vector<uint32_t> allIndices;
		allVertices.reserve(totalVerts);
		allIndices.reserve(totalIndices);

		m_Merged.meshInfos.resize(m_Meshes.size());

		uint32_t vertexOffset = 0;
		uint32_t indexOffset = 0;
		for (size_t i = 0; i < m_Meshes.size(); i++)
		{
			const auto& mesh = m_Meshes[i];

			m_Merged.meshInfos[i].indexCount = static_cast<uint32_t>(mesh.m_Indices.size());
			m_Merged.meshInfos[i].firstIndex = indexOffset;
			m_Merged.meshInfos[i].baseVertex = static_cast<int32_t>(vertexOffset);

			allVertices.insert(allVertices.end(), mesh.m_Vertices.begin(), mesh.m_Vertices.end());
			allIndices.insert(allIndices.end(), mesh.m_Indices.begin(), mesh.m_Indices.end());

			vertexOffset += static_cast<uint32_t>(mesh.m_Vertices.size());
			indexOffset += static_cast<uint32_t>(mesh.m_Indices.size());
		}

		m_Merged.totalVertices = totalVerts;
		m_Merged.totalIndices = totalIndices;

		// Create VBO, EBO, VAO using engine abstractions
		m_Merged.vbo = std::make_unique<VertexBuffer>(
			static_cast<const void*>(allVertices.data()),
			static_cast<uint32_t>(allVertices.size() * sizeof(MeshVertex)));

		m_Merged.ebo = std::make_unique<IndexBuffer>(
			static_cast<const void*>(allIndices.data()),
			static_cast<uint32_t>(allIndices.size() * sizeof(uint32_t)));

		m_Merged.vao = std::make_unique<VertexArray>();

		auto layout = MeshVertex::GetLayout();
		m_Merged.vao->SetVertexBuffer(m_Merged.vbo.get());
		m_Merged.vao->SetIndexBuffer(m_Merged.ebo.get());
		m_Merged.vao->SetLayout(layout);
	}

	void Onyx::Model::Draw(Onyx::Shader& shader)
	{
		for (unsigned int i = 0; i < m_Meshes.size(); i++)
			m_Meshes[i].Draw(shader);
	}

	void Model::DrawMergedMesh(size_t meshIndex) const
	{
		if (!HasMergedBuffers())
			return;
		const auto& merged = m_Merged;
		if (meshIndex >= merged.meshInfos.size())
			return;

		const auto& info = merged.meshInfos[meshIndex];
		merged.vao->Bind();
		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			static_cast<GLsizei>(info.indexCount),
			GL_UNSIGNED_INT,
			reinterpret_cast<void*>(static_cast<uintptr_t>(info.firstIndex * sizeof(uint32_t))),
			info.baseVertex);
		merged.vao->UnBind();
	}

	void Model::DrawAllMergedMeshes() const
	{
		if (!HasMergedBuffers())
			return;
		const auto& merged = m_Merged;

		merged.vao->Bind();
		for (const auto& info : merged.meshInfos)
		{
			glDrawElementsBaseVertex(
				GL_TRIANGLES,
				static_cast<GLsizei>(info.indexCount),
				GL_UNSIGNED_INT,
				reinterpret_cast<void*>(static_cast<uintptr_t>(info.firstIndex * sizeof(uint32_t))),
				info.baseVertex);
		}
		merged.vao->UnBind();
	}

	void Onyx::Model::ProcessNode(aiNode* node, const aiScene* scene, const glm::mat4& parentTransform)
	{
		// Accumulate the transform from parent
		glm::mat4 nodeTransform = AiMatrixToGlm(node->mTransformation);
		glm::mat4 globalTransform = parentTransform * nodeTransform;

		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			// Use node name if mesh name is empty or generic (Blender exports object names as node names)
			std::string meshName = mesh->mName.C_Str();
			std::string nodeName = node->mName.C_Str();
			if (meshName.empty() || meshName.find("Mesh") == 0)
			{
				meshName = nodeName; // Use node name instead
			}
			m_Meshes.push_back(ProcessMesh(mesh, scene, globalTransform, meshName));
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, globalTransform);
		}
	}

	Mesh Onyx::Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& transform, const std::string& name)
	{
		std::vector<MeshVertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<MeshTexture> textures;
		// Use provided name, or fall back to mesh name from Assimp
		std::string meshName = name.empty() ? mesh->mName.C_Str() : name;

		// Normal matrix for transforming normals (inverse transpose of upper-left 3x3)
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

		// Set vertices and textures coords for the mesh
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
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
			if (mesh->mTextureCoords[0])
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
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		// process materials (only if texture loading is enabled)
		if (m_LoadTextures)
		{
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

	std::vector<MeshTexture> Onyx::Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
	{
		std::vector<MeshTexture> textures;

		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);

			bool skip = false;

			for (unsigned int j = 0; j < m_UsedTextures.size(); j++)
			{
				if (std::strcmp(m_UsedTextures[j].path.data(), str.C_Str()) == 0)
				{
					textures.push_back(m_UsedTextures[j]);
					skip = true;
					break;
				}
			}

			if (!skip)
			{
				MeshTexture texture;
				texture.id = TextureFromFile(str.C_Str(), m_Directory);
				texture.type = typeName;
				texture.path = str.C_Str();
				m_UsedTextures.push_back(texture);
				textures.push_back(texture);
			}
		}

		return textures;
	}

	unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma)
	{
		std::string filename = std::string(path);
		filename = directory + '/' + filename;

		stbi_set_flip_vertically_on_load(true);
		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			GLenum format = GL_RGBA;
			if (nrComponents == 1)
				format = GL_RED;
			else if (nrComponents == 3)
				format = GL_RGB;

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
			glDeleteTextures(1, &textureID);
			return 0;
		}

		return textureID;
	}

} // namespace Onyx