#include "WorldServer.h"
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

MMO::WorldServer* g_Server = nullptr;

static uint16_t ParsePort(const char* str, uint16_t defaultVal)
{
	if (!str)
		return defaultVal;
	char* end = nullptr;
	errno = 0;
	long val = std::strtol(str, &end, 10);
	if (errno != 0 || end == str || *end != '\0' || val < 1 || val > 65535)
	{
		std::cerr << "Invalid port '" << str << "', using default " << defaultVal << '\n';
		return defaultVal;
	}
	return static_cast<uint16_t>(val);
}

void SignalHandler(int signal)
{
	std::cout << "\nShutting down World Server..." << '\n';
	if (g_Server)
	{
		g_Server->Stop();
	}
}

int main(int argc, char* argv[])
{
	std::cout << "=== MMO World Server ===" << '\n';

	// Setup signal handler
	std::signal(SIGINT, SignalHandler);
	std::signal(SIGTERM, SignalHandler);

	// Get configuration from environment or use defaults
	const char* serverPort = std::getenv("WORLD_PORT");
	uint16_t port = ParsePort(serverPort, 7001);

	// Database connection string
	const char* dbHost = std::getenv("DB_HOST");
	const char* dbUser = std::getenv("DB_USER");
	const char* dbPass = std::getenv("DB_PASS");
	const char* dbName = std::getenv("DB_NAME");

	std::string dbConnStr = "host=" + std::string(dbHost ? dbHost : "localhost") +
							" user=" + std::string(dbUser ? dbUser : "root") +
							" password=" + std::string(dbPass ? dbPass : "root") +
							" dbname=" + std::string(dbName ? dbName : "mmogame");

	// Create and initialize server
	MMO::WorldServer server;
	g_Server = &server;

	if (!server.Initialize(port, dbConnStr))
	{
		std::cerr << "Failed to initialize World Server" << '\n';
		return 1;
	}

	// Run server
	server.Run();

	std::cout << "World Server stopped." << '\n';
	return 0;
}
