#pragma once

#include <Graphics/Material.h>
#include <string>

namespace Onyx {
	class AssetManager;
}

namespace Editor3D {

	bool SaveMaterial(const Onyx::Material& mat, const std::string& path);
	bool LoadMaterial(const std::string& path, Onyx::Material& out);
	void ScanAndRegisterMaterials(const std::string& assetRoot, Onyx::AssetManager& assets);
	void EnsureDefaultMaterials(const std::string& materialsDir, Onyx::AssetManager& assets);

} // namespace Editor3D
