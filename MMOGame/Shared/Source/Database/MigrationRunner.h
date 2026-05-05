#pragma once

#include <filesystem>
#include <string>

namespace MMO {

class Database;

// MigrationRunner — applies versioned SQL migrations from a directory.
//
// Files in the directory must be named NNNN_description.sql with a 4-digit
// numeric prefix; they're applied in numeric order. Applied IDs are tracked
// in the schema_migrations table.
//
// Both LoginServer and WorldServer call ApplyAll on startup. A pg_advisory_lock
// serializes concurrent runners against a fresh DB; the second caller waits,
// then sees the migrations already applied and is a no-op.
class MigrationRunner {
public:
    static constexpr const char* kDefaultMigrationsDir = "MMOGame/Database/migrations";

    // pg_advisory_lock key — fixed across processes. Value is arbitrary; chosen
    // for human recognition (ASCII "MMOG" in upper 32 bits + lock id 1).
    static constexpr long long kAdvisoryLockKey = 0x4D4D4F4700000001LL;

    // Returns true on success (all unapplied migrations applied, or none needed).
    // Returns false if any migration failed; partial application is rolled back
    // by Postgres because each migration runs in its own transaction.
    static bool ApplyAll(Database& db, const std::filesystem::path& dir = ResolveDefaultDir());

private:
    static std::filesystem::path ResolveDefaultDir();
};

} // namespace MMO
