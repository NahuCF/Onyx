#include "LocalRunSession.h"

#include "../World/EditorWorldSystem.h"

#ifdef HAS_DATABASE
#include "../../../Shared/Source/Database/Database.h"
#include "../../../Shared/Source/Database/MigrationRunner.h"
#include <pqxx/pqxx>
#endif

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

namespace MMO {

	namespace {

		const char* GetEnvOr(const char* name, const char* fallback)
		{
			const char* v = std::getenv(name);
			return (v && *v) ? v : fallback;
		}

#ifdef _WIN32
		constexpr const char* kExeSuffix = ".exe";
#else
		constexpr const char* kExeSuffix = "";
#endif

		std::filesystem::path BinaryPath(const std::filesystem::path& dir, const char* name)
		{
			return dir / (std::string(name) + kExeSuffix);
		}

	} // namespace

	LocalRunSession::~LocalRunSession()
	{
		Stop();
	}

	std::string LocalRunSession::BuildDbConnString() const
	{
		std::ostringstream s;
		s << "host=" << GetEnvOr("DB_HOST", "localhost")
		  << " port=" << GetEnvOr("DB_PORT", "5432")
		  << " user=" << GetEnvOr("DB_USER", "root")
		  << " password=" << GetEnvOr("DB_PASS", "root")
		  << " dbname=" << GetEnvOr("DB_NAME", "mmogame");
		return s.str();
	}

	std::vector<std::string> LocalRunSession::BuildServerEnv() const
	{
		return {
			std::string("DB_HOST=") + GetEnvOr("DB_HOST", "localhost"),
			std::string("DB_PORT=") + GetEnvOr("DB_PORT", "5432"),
			std::string("DB_USER=") + GetEnvOr("DB_USER", "root"),
			std::string("DB_PASS=") + GetEnvOr("DB_PASS", "root"),
			std::string("DB_NAME=") + GetEnvOr("DB_NAME", "mmogame"),
		};
	}

	bool LocalRunSession::ApplyMigration(const std::filesystem::path& sqlPath)
	{
		std::ifstream in(sqlPath, std::ios::binary);
		if (!in)
		{
			m_LastError = "migration.sql not found at " + sqlPath.string();
			return false;
		}
		std::ostringstream sb;
		sb << in.rdbuf();
		const std::string sql = sb.str();
		if (sql.empty())
		{
			m_LastError = "migration.sql is empty (no entities to apply)";
			return false;
		}

#ifdef HAS_DATABASE
		try
		{
			// Apply schema migrations first so the editor's data migration has the
			// tables (creature_template entry references etc.) it depends on. Even
			// though the servers also do this on boot, doing it here closes a
			// bootstrap race on a fresh DB: the editor's data migration runs before
			// the servers boot, and would otherwise hit FK violations.
			Database db;
			if (!db.Connect(BuildDbConnString()))
			{
				m_LastError = "Could not connect to local Postgres for migration apply";
				return false;
			}
			if (!MigrationRunner::ApplyAll(db))
			{
				m_LastError = "Schema migrations failed";
				return false;
			}

			// Now apply the editor's data migration. The SQL emitted by
			// MigrationSqlWriter wraps each entity in its own BEGIN/COMMIT. Use
			// nontransaction.exec so libpq executes the whole script in one
			// round-trip and the inner BEGIN/COMMIT pairs apply as written.
			pqxx::nontransaction nt(db.GetRawConnection());
			nt.exec(sql);
		}
		catch (const std::exception& e)
		{
			m_LastError = std::string("migration.sql apply failed: ") + e.what();
			return false;
		}

		return true;
#else
		(void)sql;
		m_LastError = "Run Locally is unavailable: this build was compiled without "
					  "libpqxx (Postgres client). Install libpqxx and rebuild.";
		return false;
#endif
	}

	bool LocalRunSession::Start(const std::filesystem::path& editorBinaryDir,
								const std::filesystem::path& devDataDir,
								uint32_t mapId)
	{
		if (m_Status != Status::Idle && m_Status != Status::Stopped && m_Status != Status::Failed)
		{
			m_LastError = "session already running";
			return false;
		}
		m_LastError.clear();

		// Step 1 — export. The editor caller will have invoked ExportForRuntime
		// already (see Editor3DLayer); we just verify migration.sql exists.
		m_Status = Status::Exporting;
		const std::filesystem::path sqlPath = devDataDir / "migration.sql";
		if (!std::filesystem::exists(sqlPath))
		{
			m_Status = Status::Failed;
			m_LastError = "Run Locally requires that the editor first emits " + sqlPath.string() + " (caller should invoke ExportForRuntime before Start).";
			return false;
		}

		// Step 2 — apply migration.sql to the local Postgres.
		m_Status = Status::ApplyingMigration;
		if (!ApplyMigration(sqlPath))
		{
			m_Status = Status::Failed;
			return false;
		}

		const auto envOverrides = BuildServerEnv();

		// Step 3 — spawn LoginServer.
		m_Status = Status::StartingLogin;
		if (!m_LoginServer.Start(BinaryPath(editorBinaryDir, "MMOLoginServer"), {}, envOverrides))
		{
			m_Status = Status::Failed;
			m_LastError = "Failed to start MMOLoginServer";
			return false;
		}
		// Brief wait so the server gets past schema migrations + listens on 7000.
		std::this_thread::sleep_for(std::chrono::milliseconds(750));

		// Step 4 — spawn WorldServer.
		m_Status = Status::StartingWorld;
		if (!m_WorldServer.Start(BinaryPath(editorBinaryDir, "MMOWorldServer"), {}, envOverrides))
		{
			m_Status = Status::Failed;
			m_LastError = "Failed to start MMOWorldServer";
			Stop();
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(750));

		// Step 5 — spawn MMOClient (no auto-login token in v1).
		m_Status = Status::StartingClient;
		if (!m_Client.Start(BinaryPath(editorBinaryDir, "MMOClient"), {}, envOverrides))
		{
			m_Status = Status::Failed;
			m_LastError = "Failed to start MMOClient";
			Stop();
			return false;
		}

		m_Status = Status::Running;
		return true;
	}

	void LocalRunSession::Tick()
	{
		if (m_Status != Status::Running)
			return;

		const bool loginAlive = m_LoginServer.IsRunning();
		const bool worldAlive = m_WorldServer.IsRunning();
		const bool clientAlive = m_Client.IsRunning();

		if (!loginAlive || !worldAlive || !clientAlive)
		{
			// If any process exited, stop the rest so the editor doesn't end up
			// with orphaned servers running against a dead client.
			Stop();
		}
	}

	void LocalRunSession::Stop()
	{
		if (m_Status == Status::Idle || m_Status == Status::Stopped)
		{
			m_Status = Status::Stopped;
			return;
		}
		m_Status = Status::Stopping;
		// Reverse start order.
		m_Client.Stop();
		m_WorldServer.Stop();
		m_LoginServer.Stop();
		m_Status = Status::Stopped;
	}

} // namespace MMO
