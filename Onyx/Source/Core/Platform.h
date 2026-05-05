#pragma once

#include <filesystem>

namespace Onyx::Platform {

	// Absolute path to the currently running executable.
	std::filesystem::path GetExecutablePath();

	inline std::filesystem::path GetExecutableDir()
	{
		return GetExecutablePath().parent_path();
	}

} // namespace Onyx::Platform
