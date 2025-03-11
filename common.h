#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <cstdint>
#include <vector>
#include <chrono>

// Game constants
constexpr int DEFAULT_PORT = 8080;
constexpr int MAX_BUFFER_SIZE = 1024;
constexpr int GAME_DURATION_SECONDS = 60;
constexpr int INACTIVITY_TIMEOUT_SECONDS = 10;

// Maze dimensions
constexpr int MAZE_WIDTH = 10;
constexpr int MAZE_HEIGHT = 10;

// Message types
enum class MessageType {
    JOIN,
    WELCOME,
    MOVE,
    POS,
    TREASURE,
    COLLECTED,
    SCORES,
    KICK,
    GAMEOVER
};

// Direction enum
enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

// Convert string to Direction
inline Direction stringToDirection(const std::string& dir) {
    if (dir == "UP" || dir == "W" || dir == "w") return Direction::UP;
    if (dir == "DOWN" || dir == "S" || dir == "s") return Direction::DOWN;
    if (dir == "LEFT" || dir == "A" || dir == "a") return Direction::LEFT;
    if (dir == "RIGHT" || dir == "D" || dir == "d") return Direction::RIGHT;
    return Direction::DOWN; // Default
}

// Player structure
struct Player {
    int id;
    std::string username;
    int x;
    int y;
    int score;
    std::chrono::steady_clock::time_point lastActivity;

    // Default constructor (required for std::map)
    Player() : id(-1), username(""), x(0), y(0), score(0),
               lastActivity(std::chrono::steady_clock::now()) {}

    Player(int _id, const std::string& _username, int _x, int _y)
        : id(_id), username(_username), x(_x), y(_y), score(0),
          lastActivity(std::chrono::steady_clock::now()) {}
};

// Position structure
struct Position {
    int x;
    int y;

    Position(int _x = 0, int _y = 0) : x(_x), y(_y) {}

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

#endif // COMMON_H
