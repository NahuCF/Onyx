#pragma once

#include "Subprocess.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace MMO {

// LocalRunSession — orchestrates Mode A "Run Locally" from the editor:
//   1. Run ExportForRuntime(devDir) — writes Data/ + migration.sql.
//   2. Apply migration.sql against the local Postgres.
//   3. Spawn MMOLoginServer + MMOWorldServer + MMOClient as subprocesses.
//   4. Provide IsRunning()/Stop()/Status() for the editor's status footer.
//
// All four binaries live next to the editor (same RUNTIME_OUTPUT_DIRECTORY).
class LocalRunSession {
public:
    enum class Status {
        Idle,
        Exporting,
        ApplyingMigration,
        StartingLogin,
        StartingWorld,
        StartingClient,
        Running,
        Stopping,
        Stopped,
        Failed
    };

    LocalRunSession() = default;
    ~LocalRunSession();

    // Begin the session. Returns false if any step fails before the client launches.
    // On failure, m_LastError carries a human-readable message and Status() == Failed.
    bool Start(const std::filesystem::path& editorBinaryDir,
               const std::filesystem::path& devDataDir,
               uint32_t mapId);

    // Detect crashed children; updates Status() to Stopped if any has exited.
    void Tick();

    // Stop all spawned processes (reverse start order).
    void Stop();

    Status             GetStatus() const { return m_Status; }
    const std::string& LastError() const { return m_LastError; }

private:
    bool ApplyMigration(const std::filesystem::path& sqlPath);

    // DSN built from env vars (DB_HOST, DB_USER, DB_PASS, DB_NAME) — same as
    // the servers expect. Defaults match the dev convention (root/root/mmogame).
    std::string BuildDbConnString() const;

    std::vector<std::string> BuildServerEnv() const;

    Status m_Status = Status::Idle;
    std::string m_LastError;

    Subprocess m_LoginServer;
    Subprocess m_WorldServer;
    Subprocess m_Client;
};

} // namespace MMO
