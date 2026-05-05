#include "MaterialSerializer.h"
#include <Graphics/AssetManager.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace Editor3D {

	bool SaveMaterial(const Onyx::Material& mat, const std::string& path)
	{
		nlohmann::json j;
		j["name"] = mat.name;
		j["id"] = mat.id;
		j["albedo"] = mat.albedoPath;
		j["normal"] = mat.normalPath;
		j["rma"] = mat.rmaPath;
		j["tilingScale"] = mat.tilingScale;
		j["normalStrength"] = mat.normalStrength;

		std::ofstream file(path);
		if (!file.is_open())
		{
			std::cerr << "[Material] Failed to save: " << path << std::endl;
			return false;
		}

		file << j.dump(4);
		return true;
	}

	bool LoadMaterial(const std::string& path, Onyx::Material& out)
	{
		std::ifstream file(path);
		if (!file.is_open())
		{
			std::cerr << "[Material] Failed to load: " << path << std::endl;
			return false;
		}

		nlohmann::json j;
		try
		{
			file >> j;
		}
		catch (const nlohmann::json::parse_error& e)
		{
			std::cerr << "[Material] Parse error in " << path << ": " << e.what() << std::endl;
			return false;
		}

		out.name = j.value("name", "Unnamed");
		out.id = j.value("id", "");

		// Backward compat: accept both "albedo" and "diffuse" keys
		if (j.contains("albedo"))
		{
			out.albedoPath = j.value("albedo", "");
		}
		else
		{
			out.albedoPath = j.value("diffuse", "");
		}

		out.normalPath = j.value("normal", "");
		out.rmaPath = j.value("rma", "");
		out.tilingScale = j.value("tilingScale", 8.0f);
		out.normalStrength = j.value("normalStrength", 1.0f);
		out.filePath = path;

		return true;
	}

	void ScanAndRegisterMaterials(const std::string& assetRoot, Onyx::AssetManager& assets)
	{
		namespace fs = std::filesystem;

		if (!fs::exists(assetRoot))
			return;

		for (const auto& entry : fs::recursive_directory_iterator(assetRoot))
		{
			std::string ext = entry.path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

			if (ext == ".terrainmat" || ext == ".material")
			{
				Onyx::Material mat;
				if (LoadMaterial(entry.path().generic_string(), mat))
				{
					if (mat.id.empty())
					{
						mat.id = entry.path().stem().string();
					}
					mat.filePath = entry.path().generic_string();
					assets.RegisterMaterial(mat);
				}
			}
		}
	}

	void EnsureDefaultMaterials(const std::string& materialsDir, Onyx::AssetManager& assets)
	{
		if (!assets.GetAllMaterialIds().empty())
			return;

		std::filesystem::create_directories(materialsDir);

		struct DefaultMat
		{
			const char* name;
			const char* id;
			uint8_t r, g, b;
		};

		DefaultMat defaults[] = {
			{"Grass", "grass", 80, 140, 50},
			{"Dirt", "dirt", 140, 100, 60},
			{"Rock", "rock", 130, 130, 130},
			{"Sand", "sand", 200, 180, 120},
		};

		for (auto& def : defaults)
		{
			Onyx::Material mat;
			mat.name = def.name;
			mat.id = def.id;
			mat.tilingScale = 8.0f;
			mat.normalStrength = 1.0f;
			mat.filePath = materialsDir + "/" + std::string(def.id) + ".material";
			mat.albedoPath = "__solid_" + std::to_string(def.r) + "_" + std::to_string(def.g) + "_" + std::to_string(def.b);

			SaveMaterial(mat, mat.filePath);
			assets.RegisterMaterial(mat);
		}
	}

} // namespace Editor3D
