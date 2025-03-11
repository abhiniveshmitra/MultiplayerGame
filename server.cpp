#include "server.h"

Position GameServer::generateRandomPosition() {
    std::uniform_int_distribution<> distX(1, MAZE_WIDTH);
    std::uniform_int_distribution<> distY(1, MAZE_HEIGHT);
    return Position(distX(gen), distY(gen));
}

bool GameServer::isValidMove(int x, int y) {
    return x >= 1 && x <= MAZE_WIDTH && y >= 1 && y <= MAZE_HEIGHT;
}

void GameServer::processMove(int playerId, Direction dir) {
    std::lock_guard<std::mutex> lock(playersMutex);

    if (players.find(playerId) == players.end()) {
        return; // Player not found
    }

    Player& player = players[playerId];
    int newX = player.x;
    int newY = player.y;

    switch (dir) {
        case Direction::UP:
            newY--;
            break;
        case Direction::DOWN:
            newY++;
            break;
        case Direction::LEFT:
            newX--;
            break;
        case Direction::RIGHT:
            newX++;
            break;
    }

    if (isValidMove(newX, newY)) {
        player.x = newX;
        player.y = newY;
    }

    player.lastActivity = std::chrono::steady_clock::now();

    // Send position update to the player
    std::string posMsg = "POS " + std::to_string(player.id) + " " + 
                         std::to_string(player.x) + " " + 
                         std::to_string(player.y);

    ClientInfo* clientInfo = udpServer.getClient(player.id);
    if (clientInfo) {
        udpServer.sendMessage(*clientInfo, posMsg);
    }

    // Check if player reached treasure
    if (player.x == treasure.x && player.y == treasure.y) {
        player.score++;

        // Broadcast collection message
        std::string collectMsg = "COLLECTED " + std::to_string(player.id) + " " + 
                                 std::to_string(player.score);
        udpServer.broadcastMessage(collectMsg);

        // Respawn treasure
        treasure = generateRandomPosition();

        // Broadcast new treasure position
        std::string treasureMsg = "TREASURE " + std::to_string(treasure.x) + " " + 
                                  std::to_string(treasure.y);
        udpServer.broadcastMessage(treasureMsg);

        // Broadcast updated scores
        broadcastScores();
    }
}

void GameServer::broadcastScores() {
    std::stringstream ss;
    ss << "SCORES " << players.size();

    for (const auto& pair : players) {
        const Player& player = pair.second;
        ss << " " << player.id << " " << player.score;
    }

    udpServer.broadcastMessage(ss.str());
}

void GameServer::checkInactivePlayers() {
    std::lock_guard<std::mutex> lock(playersMutex);
    auto now = std::chrono::steady_clock::now();

    std::vector<int> playersToRemove;

    for (const auto& pair : players) {
        const Player& player = pair.second;
        auto inactiveTime = std::chrono::duration_cast<std::chrono::seconds>(
            now - player.lastActivity).count();

        if (inactiveTime > INACTIVITY_TIMEOUT_SECONDS) {
            playersToRemove.push_back(player.id);
        }
    }

    for (int id : playersToRemove) {
        ClientInfo* clientInfo = udpServer.getClient(id);
        if (clientInfo) {
            std::string kickMsg = "KICK Inactivity timeout";
            udpServer.sendMessage(*clientInfo, kickMsg);
            udpServer.removeClient(id);
        }
        players.erase(id);
    }
}

bool GameServer::isGameOver() {
    auto now = std::chrono::steady_clock::now();
    auto gameTime = std::chrono::duration_cast<std::chrono::seconds>(
        now - gameStartTime).count();

    return gameTime >= GAME_DURATION_SECONDS;
}

void GameServer::endGame() {
    std::lock_guard<std::mutex> lock(playersMutex);

    // Find winner
    int winnerId = -1;
    int highestScore = -1;

    for (const auto& pair : players) {
        const Player& player = pair.second;
        if (player.score > highestScore) {
            highestScore = player.score;
            winnerId = player.id;
        }
    }

    // Broadcast game over message
    std::string gameOverMsg = "GAMEOVER " + std::to_string(winnerId) + " " + 
                             std::to_string(highestScore);
    udpServer.broadcastMessage(gameOverMsg);

    running = false;
}

// Assuming the class declaration order is: udpServer, running, players, treasure, nextPlayerId, gameStartTime, playersMutex, rd, gen
GameServer::GameServer(int port) 
    : udpServer(port), running(false), players(), nextPlayerId(1),
      rd(), gen(rd()), treasure(generateRandomPosition()),
      gameStartTime(), playersMutex() {

    std::cout << "Game server started on port " << port << std::endl;
}


void GameServer::start() {
    running = true;
    gameStartTime = std::chrono::steady_clock::now();

    // Start game loop in a separate thread
    std::thread gameThread(&GameServer::gameLoop, this);

    // Start message handling loop
    messageLoop();

    // Wait for game thread to finish
    if (gameThread.joinable()) {
        gameThread.join();
    }
}

void GameServer::gameLoop() {
    while (running) {
        // Check for inactive players
        checkInactivePlayers();

        // Check if game is over
        if (isGameOver()) {
            endGame();
            break;
        }

        // Sleep for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void GameServer::messageLoop() {
    std::string message;
    ClientInfo clientInfo;

    while (running) {
        if (udpServer.receiveMessage(message, clientInfo)) {
            processMessage(message, clientInfo);
        }
    }
}

void GameServer::processMessage(const std::string& message, ClientInfo& clientInfo) {
    std::istringstream iss(message);
    std::string type;
    iss >> type;

    if (type == "JOIN") {
        std::string username;
        iss >> username;

        // Create new player
        Position startPos = generateRandomPosition();
        Player newPlayer(nextPlayerId, username, startPos.x, startPos.y);

        {
            std::lock_guard<std::mutex> lock(playersMutex);
            players[newPlayer.id] = newPlayer;
        }

        // Register client
        clientInfo.playerId = newPlayer.id;
        udpServer.registerClient(newPlayer.id, clientInfo);

        // Send welcome message
        std::string welcomeMsg = "WELCOME " + std::to_string(newPlayer.id) + " " +
                                std::to_string(newPlayer.x) + " " +
                                std::to_string(newPlayer.y);
        udpServer.sendMessage(clientInfo, welcomeMsg);

        // Send treasure position
        std::string treasureMsg = "TREASURE " + std::to_string(treasure.x) + " " +
                                 std::to_string(treasure.y);
        udpServer.sendMessage(clientInfo, treasureMsg);

        // Broadcast updated scores
        broadcastScores();

        // Increment player ID for next player
        nextPlayerId++;
    }
    else if (type == "MOVE") {
        int playerId;
        std::string dirStr;
        iss >> playerId >> dirStr;

        Direction dir = stringToDirection(dirStr);
        processMove(playerId, dir);
    }
}
int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;

    // Allow optional port specification
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    std::cout << "Starting maze game server on port " << port << std::endl;

    GameServer server(port);
    server.start();

    return 0;
}

