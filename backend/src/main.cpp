#include "network/Server.h"
#include <iostream>
#include <signal.h>

static hangman::Server* g_server = nullptr;

void signalHandler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    if (g_server) {
        std::cout << "\nShutting down server..." << std::endl;
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    int port = 5000;  // Default port

    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port number" << std::endl;
            return 1;
        }
    }

    try {
        hangman::Server server(port);
        g_server = &server;

        // Initialize server (load database)
        if (!server.initialize("database/account.txt")) {
            std::cerr << "Failed to initialize server" << std::endl;
            return 1;
        }

        // Handle Ctrl+C
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        // Run server (blocks)
        server.run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
