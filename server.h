#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <cstring>
#include <random>
#include <map>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <mutex>
#include "common.h"
#include "udp_helper.h"

class GameServer {
private:
UDPServer udpServer;
bool running;
std::map<int, Player> players;
int nextPlayerId;
std::random_device rd;
std::mt19937 gen;
Position treasure;
std::chrono::steady_clock::time_point gameStartTime;
std::mutex playersMutex;


    // Generate random position within maze bounds
    Position generateRandomPosition();

    // Check if move is valid
    bool isValidMove(int x, int y);

    // Process player movement
    void processMove(int playerId, Direction dir);

    // Broadcast scores to all players
    void broadcastScores();

    // Check for inactive players
    void checkInactivePlayers();

    // Check if game is over
    bool isGameOver();

    // End the game
    void endGame();

    // Message handling loop
    void messageLoop();

    // Game loop for periodic updates
    void gameLoop();

    // Process received message
    void processMessage(const std::string& message, ClientInfo& clientInfo);

public:
    GameServer(int port = DEFAULT_PORT);

    // Start the game server
    void start();
};

#endif // SERVER_H
