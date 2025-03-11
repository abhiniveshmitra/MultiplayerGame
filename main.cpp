#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "common.h"

// Forward declarations for the classes we'll use
class GameServer;
class GameClient;

// Function declarations
void runServer();
void runClient(const std::string& serverIP, const std::string& username);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << std::endl;
        std::cerr << "  Server mode: " << argv[0] << " server" << std::endl;
        std::cerr << "  Client mode: " << argv[0] << " client <server_ip> <username>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string mode = argv[1];

    if (mode == "server") {
        runServer();
    } else if (mode == "client") {
        if (argc < 4) {
            std::cerr << "Client mode requires server IP and username" << std::endl;
            std::cerr << "Usage: " << argv[0] << " client <server_ip> <username>" << std::endl;
            return EXIT_FAILURE;
        }

        std::string serverIP = argv[2];
        std::string username = argv[3];

        runClient(serverIP, username);
    } else {
        std::cerr << "Invalid mode. Use 'server' or 'client'" << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}

// Include implementation files after main to avoid duplicate main definitions
#include "server.cpp"
#include "client.cpp"

void runServer() {
    GameServer server;
    server.start();
}

void runClient(const std::string& serverIP, const std::string& username) {
    GameClient client(serverIP, DEFAULT_PORT, username);
    client.start();
}
