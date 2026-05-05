#pragma once

#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace MMO {

// Subprocess — thin POSIX/Win abstraction for spawning + tracking + signalling
// a single child process. Used by LocalRunSession to manage MMOLoginServer,
// MMOWorldServer, and MMOClient subprocesses launched from the editor.
//
// Move-only: a Subprocess owns its child handle. Destructor terminates the
// child if still running.
class Subprocess {
public:
    Subprocess() = default;
    ~Subprocess();

    Subprocess(const Subprocess&) = delete;
    Subprocess& operator=(const Subprocess&) = delete;
    Subprocess(Subprocess&& other) noexcept;
    Subprocess& operator=(Subprocess&& other) noexcept;

    // Spawn the process. envOverrides is a list of "NAME=value" strings prepended
    // to the child's environment. cwd defaults to the parent's CWD if empty.
    // Returns false (and prints a message) on failure.
    bool Start(const std::filesystem::path& exe,
               const std::vector<std::string>& args,
               const std::vector<std::string>& envOverrides = {},
               const std::filesystem::path& cwd = {});

    // Non-blocking liveness check; reaps if exited.
    bool IsRunning();

    // Send SIGTERM (POSIX) / TerminateProcess (Windows). Reaps the child.
    void Stop();

    // Last known exit code (only valid after IsRunning() returned false).
    int ExitCode() const { return m_ExitCode; }

private:
    void Reset();

#ifdef _WIN32
    PROCESS_INFORMATION m_Pi{};
    bool                m_Started = false;
#else
    int m_Pid = -1;
#endif
    bool m_Exited = false;
    int  m_ExitCode = 0;
};

} // namespace MMO
