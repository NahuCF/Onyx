#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

class dtNavMesh;

namespace Editor3D {

	// =============================================================================
	// NavMeshDebugView — loads the same .nav file the WorldServer reads and
	// produces a flat triangle list (positions + per-poly colour) for the
	// editor viewport overlay. Owns its own dtNavMesh; nothing GPU-related
	// lives here, so the rendering side can build VBOs however it likes.
	//
	// Usage:
	//   NavMeshDebugView v;
	//   v.LoadFromFile("Data/maps/001/navmesh.nav");
	//   std::vector<float> verts;       // x,y,z, r,g,b,a per vertex (7 floats)
	//   std::vector<uint32_t> indices;  // 3 per triangle
	//   v.BuildRenderGeometry(verts, indices, /*yBias=*/0.05f);
	// =============================================================================

	class NavMeshDebugView
	{
	public:
		NavMeshDebugView();
		~NavMeshDebugView();

		NavMeshDebugView(const NavMeshDebugView&) = delete;
		NavMeshDebugView& operator=(const NavMeshDebugView&) = delete;

		bool LoadFromFile(const std::filesystem::path& path);
		bool IsLoaded() const { return m_NavMesh != nullptr; }
		void Unload();

		const std::filesystem::path& GetSourcePath() const { return m_SourcePath; }

		// Walks every tile / poly / detail-tri and emits interleaved (pos.xyz,
		// color.rgba) vertices + a triangle index list. yBias is added to each
		// vertex's Y to keep the overlay above terrain without z-fighting.
		// outVerts and outIndices are cleared at the start.
		// Returns the polygon count consumed.
		int BuildRenderGeometry(std::vector<float>& outVerts,
								std::vector<uint32_t>& outIndices,
								float yBias = 0.05f) const;

	private:
		dtNavMesh* m_NavMesh = nullptr;
		std::filesystem::path m_SourcePath;
	};

} // namespace Editor3D
