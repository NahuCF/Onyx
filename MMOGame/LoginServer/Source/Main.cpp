#include "LoginServer.h"
#include <iostream>
#include <csignal>
#include <cstdlib>

MMO::LoginServer* g_Server = nullptr;

void SignalHandler(int signal) {
    std::cout << "\nShutting down Login Server..." << std::endl;
    if (g_Server) {
        g_Server->Stop();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== MMO Login Server ===" << std::endl;

    // Setup signal handler
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Connection string from command line or environment
    std::string connectionString;
    if (argc > 1) {
        connectionString = argv[1];
    } else {
        // Build from environment variables
        const char* dbHost = std::getenv("DB_HOST");
        const char* dbPort = std::getenv("DB_PORT");
        const char* dbName = std::getenv("DB_NAME");
        const char* dbUser = std::getenv("DB_USER");
        const char* dbPass = std::getenv("DB_PASS");

        connectionString = "host=";
        connectionString += (dbHost ? dbHost : "localhost");
        connectionString += " port=";
        connectionString += (dbPort ? dbPort : "5432");
        connectionString += " dbname=";
        connectionString += (dbName ? dbName : "mmogame");
        connectionString += " user=";
        connectionString += (dbUser ? dbUser : "postgres");
        connectionString += " password=";
        connectionString += (dbPass ? dbPass : "password");
    }

    const char* serverPort = std::getenv("LOGIN_PORT");
    const char* worldHost = std::getenv("WORLD_HOST");
    const char* worldPort = std::getenv("WORLD_PORT");

    uint16_t port = serverPort ? static_cast<uint16_t>(std::atoi(serverPort)) : 7000;

    // Create and initialize server
    MMO::LoginServer server;
    g_Server = &server;

    // Set world server info
    server.SetWorldServerInfo(
        worldHost ? worldHost : "127.0.0.1",
        worldPort ? static_cast<uint16_t>(std::atoi(worldPort)) : 7001
    );

    if (!server.Initialize(connectionString, port)) {
        std::cerr << "Failed to initialize Login Server" << std::endl;
        return 1;
    }

    // Run server
    server.Run();

    std::cout << "Login Server stopped." << std::endl;
    return 0;
}
