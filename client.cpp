    #include <iostream>
    #include <cstring>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <thread>
    #include <atomic>
    #include <sstream>
    #include <map>
    #include <termios.h>
    #include <fcntl.h>
    #include <random>
    #include "common.h"
    #include "udp_helper.h"

    // Terminal control functions
    void enableRawMode() {
        struct termios raw;
        tcgetattr(STDIN_FILENO, &raw);
        raw.c_lflag &= ~(ECHO | ICANON);  // Disable echo and canonical mode
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

        // Set stdin to non-blocking
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    void disableRawMode() {
        struct termios raw;
        tcgetattr(STDIN_FILENO, &raw);
        raw.c_lflag |= (ECHO | ICANON);  // Re-enable echo and canonical mode
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

        // Set stdin back to blocking
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    }

    char readKey() {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            return c;
        }
        return 0;
    }

    // Generate a random username
    std::string generateRandomUsername() {
        static const char* prefixes[] = {"Player", "Gamer", "Hunter", "Explorer", "Seeker"};
        static const int numPrefixes = 5;

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> prefixDist(0, numPrefixes - 1);
        std::uniform_int_distribution<> numberDist(1, 999);

        std::string username = prefixes[prefixDist(gen)] + std::to_string(numberDist(gen));
        return username;
    }

    class GameClient {
    private:
        int sockfd;
        struct sockaddr_in serverAddr;
        std::atomic<bool> running;
        std::string username;
        int playerId;
        int x, y;
        int score;
        Position treasure;
        std::map<int, int> playerScores;
        UDPClient* udpClient;

        // Parse position update
        void handlePositionUpdate(std::istringstream& iss) {
            int id;
            iss >> id;

            if (id == playerId) {
                iss >> x >> y;
                std::cout << "You are now at position (" << x << ", " << y << ")" << std::endl;
            }
        }

        // Parse treasure update
        void handleTreasureUpdate(std::istringstream& iss) {
            int tx, ty;
            iss >> tx >> ty;
            treasure = Position(tx, ty);
            std::cout << "Treasure is at position (" << tx << ", " << ty << ")" << std::endl;
        }

        // Parse collection update
        void handleCollectionUpdate(std::istringstream& iss) {
            int id, newScore;
            iss >> id >> newScore;

            playerScores[id] = newScore;

            if (id == playerId) {
                score = newScore;
                std::cout << "You collected the treasure! Your score: " << score << std::endl;
            } else {
                std::cout << "Player " << id << " collected the treasure! Their score: " << newScore << std::endl;
            }
        }

        // Parse scores update
        void handleScoresUpdate(std::istringstream& iss) {
            int playerCount;
            iss >> playerCount;

            playerScores.clear();

            for (int i = 0; i < playerCount; i++) {
                int id, playerScore;
                iss >> id >> playerScore;
                playerScores[id] = playerScore;
            }

            // Find highest score
            int highestScore = -1;
            int leaderId = -1;

            for (const auto& pair : playerScores) {
                if (pair.second > highestScore) {
                    highestScore = pair.second;
                    leaderId = pair.first;
                }
            }

            std::cout << "Scores: ";
            for (const auto& pair : playerScores) {
                std::cout << "Player " << pair.first << ": " << pair.second << "  ";
            }
            std::cout << std::endl;

            if (leaderId != -1) {
                std::cout << "Leader: Player " << leaderId << " with score " << highestScore << std::endl;
            }

            std::cout << "Your score: " << score << std::endl;
        }

        // Parse welcome message
        void handleWelcome(std::istringstream& iss) {
            iss >> playerId >> x >> y;
            std::cout << "Welcome! You are Player " << playerId << " at position (" << x << ", " << y << ")" << std::endl;
        }

        // Parse kick message
        void handleKick(std::istringstream& iss) {
            std::string reason;
            std::getline(iss, reason);
            std::cout << "You have been kicked: " << reason << std::endl;
            running = false;
        }

        // Parse game over message
        void handleGameOver(std::istringstream& iss) {
            int winnerId, winnerScore;
            iss >> winnerId >> winnerScore;

            std::cout << "Game Over! ";
            if (winnerId == playerId) {
                std::cout << "You won with a score of " << winnerScore << "!" << std::endl;
            } else {
                std::cout << "Player " << winnerId << " won with a score of " << winnerScore << "!" << std::endl;
            }

            running = false;
        }

    public:
        GameClient(const std::string& serverIP = "127.0.0.1", int port = DEFAULT_PORT)
            : running(false), username(generateRandomUsername()), playerId(-1), x(0), y(0), score(0), treasure(0, 0) {

            udpClient = new UDPClient(serverIP, port);
        }

        ~GameClient() {
            disableRawMode();
            delete udpClient;
        }

        // Start the client
        void start() {
            running = true;

            std::cout << "Connecting as " << username << "..." << std::endl;

            // Send join request
            std::string joinMsg = "JOIN " + username;
            sendMessage(joinMsg);

            // Start message receiving thread
            std::thread receiveThread(&GameClient::receiveMessages, this);

            // Start input handling
            handleUserInput();

            // Wait for receive thread to finish
            if (receiveThread.joinable()) {
                receiveThread.join();
            }
        }

        // Send message to server
        void sendMessage(const std::string& message) {
            udpClient->sendMessage(message);
        }

        // Receive messages from server
        void receiveMessages() {
            std::string message;

            while (running) {
                if (udpClient->receiveMessage(message)) {
                    processServerMessage(message);
                }

                // Small sleep to prevent CPU hogging
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // Process message from server
        void processServerMessage(const std::string& message) {
            std::istringstream iss(message);
            std::string type;
            iss >> type;

            if (type == "POS") {
                handlePositionUpdate(iss);
            } else if (type == "TREASURE") {
                handleTreasureUpdate(iss);
            } else if (type == "COLLECTED") {
                handleCollectionUpdate(iss);
            } else if (type == "SCORES") {
                handleScoresUpdate(iss);
            } else if (type == "WELCOME") {
                handleWelcome(iss);
            } else if (type == "KICK") {
                handleKick(iss);
            } else if (type == "GAMEOVER") {
                handleGameOver(iss);
            }
        }

        // Handle user input
        void handleUserInput() {
            std::cout << "Game controls: W (up), A (left), S (down), D (right), Q (quit)" << std::endl;

            enableRawMode();

            while (running) {
                char input = readKey();

                if (input == 'Q' || input == 'q') {
                    running = false;
                    break;
                }

                std::string direction;
                switch (input) {
                    case 'W':
                    case 'w':
                        direction = "UP";
                        break;
                    case 'A':
                    case 'a':
                        direction = "LEFT";
                        break;
                    case 'S':
                    case 's':
                        direction = "DOWN";
                        break;
                    case 'D':
                    case 'd':
                        direction = "RIGHT";
                        break;
                    // Handle arrow keys (they send escape sequences)
                    case 27: // ESC
                        if (read(STDIN_FILENO, &input, 1) == 1 && input == '[') {
                            if (read(STDIN_FILENO, &input, 1) == 1) {
                                switch (input) {
                                    case 'A': // Up arrow
                                        direction = "UP";
                                        break;
                                    case 'B': // Down arrow
                                        direction = "DOWN";
                                        break;
                                    case 'C': // Right arrow
                                        direction = "RIGHT";
                                        break;
                                    case 'D': // Left arrow
                                        direction = "LEFT";
                                        break;
                                }
                            }
                        }
                        break;
                    default:
                        // Invalid key, do nothing
                        break;
                }

                if (!direction.empty() && playerId != -1) {
                    std::string moveMsg = "MOVE " + std::to_string(playerId) + " " + direction;
                    sendMessage(moveMsg);
                }

                // Small sleep to prevent CPU hogging
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            disableRawMode();
        }
    };

    int main(int argc, char* argv[]) {
        std::string serverIP = "127.0.0.1";  // Default to localhost

        // If server IP is provided, use it
        if (argc > 1) {
            serverIP = argv[1];
        }

        GameClient client(serverIP);
        client.start();

        return 0;
    }
