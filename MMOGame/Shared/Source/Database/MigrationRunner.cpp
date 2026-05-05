#include "MigrationRunner.h"
#include "Database.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace MMO {

	namespace {

		struct MigrationFile
		{
			int id;
			std::filesystem::path path;
		};

		bool ParseMigrationId(const std::string& filename, int& outId)
		{
			static const std::regex kPrefix(R"(^(\d{4})_.*\.sql$)", std::regex::icase);
			std::smatch match;
			if (!std::regex_match(filename, match, kPrefix))
			{
				return false;
			}
			outId = std::stoi(match[1].str());
			return true;
		}

		std::vector<MigrationFile> CollectMigrations(const std::filesystem::path& dir)
		{
			std::vector<MigrationFile> files;
			if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
			{
				std::cerr << "[MigrationRunner] migrations directory not found: " << dir << std::endl;
				return files;
			}

			for (const auto& entry : std::filesystem::directory_iterator(dir))
			{
				if (!entry.is_regular_file())
					continue;
				const std::string fname = entry.path().filename().string();
				int id = 0;
				if (!ParseMigrationId(fname, id))
				{
					std::cerr << "[MigrationRunner] skipping unrecognized file: " << fname << std::endl;
					continue;
				}
				files.push_back({id, entry.path()});
			}

			std::sort(files.begin(), files.end(),
					  [](const MigrationFile& a, const MigrationFile& b) { return a.id < b.id; });

			// Detect duplicate IDs (e.g., 0002_foo.sql and 0002_bar.sql).
			for (size_t i = 1; i < files.size(); ++i)
			{
				if (files[i].id == files[i - 1].id)
				{
					std::cerr << "[MigrationRunner] duplicate migration id "
							  << files[i].id << " in " << files[i - 1].path.filename()
							  << " and " << files[i].path.filename() << std::endl;
				}
			}
			return files;
		}

		std::string ReadFile(const std::filesystem::path& path)
		{
			std::ifstream stream(path, std::ios::binary);
			if (!stream)
			{
				throw std::runtime_error("could not open migration file: " + path.string());
			}
			std::ostringstream buffer;
			buffer << stream.rdbuf();
			return buffer.str();
		}

	} // namespace

	std::filesystem::path MigrationRunner::ResolveDefaultDir()
	{
		if (const char* override_dir = std::getenv("DB_MIGRATIONS_DIR"))
		{
			return std::filesystem::path(override_dir);
		}
		return std::filesystem::path(kDefaultMigrationsDir);
	}

	bool MigrationRunner::ApplyAll(Database& db, const std::filesystem::path& dir)
	{
		if (!db.IsConnected())
		{
			std::cerr << "[MigrationRunner] database not connected — skipping migrations" << std::endl;
			return false;
		}

		auto& conn = db.GetRawConnection();

		try
		{
			// Acquire advisory lock FIRST, before any DDL. Concurrent CREATE TABLE
			// IF NOT EXISTS calls in Postgres can race on pg_type uniqueness even
			// though the SQL is "idempotent". The lock serializes startup so the
			// first runner does the work and the second is a no-op.
			// Use pg_advisory_lock at session level (not txn level) so it stays
			// held across the multiple transactions below.
			{
				pqxx::nontransaction nt(conn);
				nt.exec_params("SELECT pg_advisory_lock($1)", kAdvisoryLockKey);
			}

			// Bootstrap schema_migrations. 0001 also creates it; both are CREATE
			// TABLE IF NOT EXISTS so this is safe under the advisory lock.
			{
				pqxx::work txn(conn);
				txn.exec(
					"CREATE TABLE IF NOT EXISTS schema_migrations ("
					"    id          INTEGER PRIMARY KEY,"
					"    applied_at  TIMESTAMPTZ NOT NULL DEFAULT NOW()"
					")");
				txn.commit();
			}

			std::unordered_set<int> applied;
			{
				pqxx::work txn(conn);
				pqxx::result result = txn.exec("SELECT id FROM schema_migrations");
				for (const auto& row : result)
				{
					applied.insert(row[0].as<int>());
				}
				txn.commit();
			}

			const auto migrations = CollectMigrations(dir);
			std::cout << "[MigrationRunner] found " << migrations.size()
					  << " migrations in " << dir.string()
					  << " (" << applied.size() << " already applied)" << std::endl;

			bool ok = true;
			for (const auto& mig : migrations)
			{
				if (applied.count(mig.id))
					continue;

				const auto t0 = std::chrono::steady_clock::now();
				try
				{
					const std::string sql = ReadFile(mig.path);
					pqxx::work txn(conn);
					txn.exec(sql);
					txn.exec_params(
						"INSERT INTO schema_migrations (id) VALUES ($1)",
						mig.id);
					txn.commit();

					const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
										std::chrono::steady_clock::now() - t0)
										.count();
					std::cout << "[MigrationRunner] applied " << mig.path.filename().string()
							  << " (" << ms << " ms)" << std::endl;
				}
				catch (const std::exception& e)
				{
					std::cerr << "[MigrationRunner] failed migration "
							  << mig.path.filename().string() << ": " << e.what() << std::endl;
					ok = false;
					break;
				}
			}

			// Always release the advisory lock, even on failure.
			try
			{
				pqxx::nontransaction nt(conn);
				nt.exec_params("SELECT pg_advisory_unlock($1)", kAdvisoryLockKey);
			}
			catch (const std::exception& e)
			{
				std::cerr << "[MigrationRunner] advisory unlock failed: " << e.what() << std::endl;
			}

			return ok;
		}
		catch (const std::exception& e)
		{
			std::cerr << "[MigrationRunner] fatal: " << e.what() << std::endl;
			return false;
		}
	}

} // namespace MMO
