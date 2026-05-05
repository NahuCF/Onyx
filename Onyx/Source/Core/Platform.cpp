#include "pch.h"
#include "Platform.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace Onyx::Platform {

std::filesystem::path GetExecutablePath() {
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return (len > 0 && len < MAX_PATH) ? std::filesystem::path(buf) : std::filesystem::path{};
#else
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return {};
    buf[n] = '\0';
    return std::filesystem::path(buf);
#endif
}

} // namespace Onyx::Platform
