#include "WorldServer.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <string>

MMO::WorldServer* g_Server = nullptr;

void SignalHandler(int signal) {
    std::cout << "\nShutting down World Server..." << std::endl;
    if (g_Server) {
        g_Server->Stop();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== MMO World Server ===" << std::endl;

    // Setup signal handler
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Get configuration from environment or use defaults
    const char* serverPort = std::getenv("WORLD_PORT");
    uint16_t port = serverPort ? static_cast<uint16_t>(std::atoi(serverPort)) : 7001;

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

    if (!server.Initialize(port, dbConnStr)) {
        std::cerr << "Failed to initialize World Server" << std::endl;
        return 1;
    }

    // Run server
    server.Run();

    std::cout << "World Server stopped." << std::endl;
    return 0;
}
