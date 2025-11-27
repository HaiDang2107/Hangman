#include "service/Server.h"
#include <iostream>
#include <signal.h>

static hangman::Server* g_server = nullptr;

void signalHandler(int sig) {
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
