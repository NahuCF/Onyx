#pragma once

#include "AssetHandle.h"
#include "Material.h"
#include "Model.h"
#include "Texture.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Onyx {

	class AnimatedModel;

	enum class ModelLoadStatus : uint8_t
	{
		NotRequested,
		Queued,
		Parsing,
		ReadyForGPU,
		Uploading,
		Ready,
		Failed
	};

	class AssetManager
	{
	public:
		AssetManager();
		~AssetManager();

		ModelHandle LoadModel(const std::string& path, bool loadTextures = false);
		Model* Get(ModelHandle handle);
		Model* FindModel(const std::string& path);

		AnimatedModelHandle LoadAnimatedModel(const std::string& path);
		AnimatedModel* Get(AnimatedModelHandle handle);
		AnimatedModel* FindAnimatedModel(const std::string& path);

		TextureHandle LoadTexture(const std::string& path);
		Texture* Get(TextureHandle handle);

		Texture* ResolveTexture(const std::string& path);

		Material& CreateMaterial(const std::string& id, const std::string& name = "");
		void RegisterMaterial(const Material& mat);
		std::string GenerateUniqueMaterialId(const std::string& baseName) const;
		Material* GetMaterial(const std::string& id);
		const Material* GetMaterial(const std::string& id) const;
		bool HasMaterial(const std::string& id) const;
		bool RemoveMaterial(const std::string& id);
		std::vector<std::string> GetAllMaterialIds() const;

		Texture* GetDefaultAlbedo() const { return m_DefaultAlbedo.get(); }
		Texture* GetDefaultNormal() const { return m_DefaultNormal.get(); }

		void Reload(ModelHandle handle);
		void Reload(AnimatedModelHandle handle);

		// Async model loading
		ModelLoadStatus GetModelStatus(const std::string& path) const;
		void RequestModelAsync(const std::string& path, bool checkAnimated = true);
		void ProcessGPUUploads(int maxPerFrame = 2);

	private:
		uint32_t m_NextId = 1;

		std::unordered_map<std::string, ModelHandle> m_ModelPaths;
		std::unordered_map<std::string, AnimatedModelHandle> m_AnimModelPaths;
		std::unordered_map<std::string, TextureHandle> m_TexturePaths;

		std::unordered_map<uint32_t, std::unique_ptr<Model>> m_Models;
		std::unordered_map<uint32_t, std::unique_ptr<AnimatedModel>> m_AnimModels;
		std::unordered_map<uint32_t, std::unique_ptr<Texture>> m_Textures;

		std::unordered_map<uint32_t, std::string> m_IdToPath;

		std::unordered_map<std::string, Material> m_Materials;

		std::unique_ptr<Texture> m_DefaultAlbedo;
		std::unique_ptr<Texture> m_DefaultNormal;

		// Pre-allocated GPU buffer pool — avoids VRAM allocation stalls during model upload
		struct PooledBufferSet
		{
			std::unique_ptr<VertexArray> vao;
			std::unique_ptr<VertexBuffer> vbo;
			std::unique_ptr<IndexBuffer> ebo;
			size_t vboCapacity = 0;
			size_t eboCapacity = 0;
		};

		std::deque<PooledBufferSet> m_StaticBufferPool;

		// Large pre-allocated buffers for big models (no VAO — layout varies by model type)
		struct LargePoolEntry
		{
			std::unique_ptr<VertexBuffer> vbo;
			std::unique_ptr<IndexBuffer> ebo;
			size_t vboCapacity = 0;
			size_t eboCapacity = 0;
		};
		std::deque<LargePoolEntry> m_LargeBufferPool;

		void InitBufferPool();
		MergedBuffers BuildMergedFromPool(
			const void* vertexData, size_t vertexBytes,
			const void* indexData, size_t indexBytes,
			uint32_t totalVertices, uint32_t totalIndices,
			std::vector<MergedMeshInfo>&& meshInfos);

		// Async loading infrastructure
		struct PendingLoad
		{
			std::string path;
			bool checkAnimated = true;
			std::atomic<ModelLoadStatus> status{ModelLoadStatus::Queued};
			// Populated by background thread:
			std::string directory;
			std::unique_ptr<AnimatedModel> parsedAnimModel;
			bool isAnimated = false;
			// Pre-concatenated merged data (populated by background thread, consumed by GPU upload)
			std::vector<uint8_t> mergedVertexData;
			std::vector<uint8_t> mergedIndexData;
			std::vector<MergedMeshInfo> mergedMeshInfos;
			std::vector<MeshBoundsInfo> meshBounds; // per-mesh bounds computed on background thread
			uint32_t mergedTotalVertices = 0;
			uint32_t mergedTotalIndices = 0;
		};

		mutable std::mutex m_LoadMutex;
		std::deque<std::shared_ptr<PendingLoad>> m_ParseQueue;
		std::deque<std::shared_ptr<PendingLoad>> m_UploadQueue;
		std::unordered_map<std::string, std::shared_ptr<PendingLoad>> m_PendingLoads;
		std::thread m_LoadThread;
		std::condition_variable m_LoadCV;
		std::atomic<bool> m_ShutdownLoader{false};

		void LoaderThreadFunc();

		template <typename VertexT, typename MeshRange>
		void MergeMeshesToPending(const MeshRange& meshes, PendingLoad& out,
								  std::vector<MeshBoundsInfo>* boundsOut);

		// Async texture loading — stbi_load on background thread, glTexImage2D on main thread
		struct PendingTextureLoad
		{
			std::string path;
			PreloadedImage preloaded;
		};

		std::mutex m_TextureLoadMutex;
		std::condition_variable m_TextureLoadCV;
		std::thread m_TextureLoadThread;
		std::deque<std::shared_ptr<PendingTextureLoad>> m_TextureParseQueue;					// bg thread consumes
		std::deque<std::shared_ptr<PendingTextureLoad>> m_TextureReadyQueue;					// main thread consumes
		std::unordered_map<std::string, std::shared_ptr<PendingTextureLoad>> m_PendingTextures; // main thread only

		void TextureLoaderThreadFunc();

		// Staged GPU upload for large models (streamed across multiple frames)
		static constexpr size_t UPLOAD_CHUNK_BYTES = 4 * 1024 * 1024; // 4MB per frame

		struct StagedUpload
		{
			std::shared_ptr<PendingLoad> pending;
			std::unique_ptr<Model> staticModel;
			std::unique_ptr<AnimatedModel> animModel;
			bool isAnimated = false;

			std::unique_ptr<VertexArray> vao;
			std::unique_ptr<VertexBuffer> vbo;
			std::unique_ptr<IndexBuffer> ebo;

			size_t vboUploaded = 0;
			size_t eboUploaded = 0;
		};

		std::unique_ptr<StagedUpload> m_ActiveUpload;

		void BeginStagedUpload(std::shared_ptr<PendingLoad> pending);
		void ContinueStagedUpload();
		void FinalizeStagedUpload();
	};

} // namespace Onyx
