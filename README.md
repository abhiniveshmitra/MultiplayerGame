# 🎮 Real-Time Multiplayer Maze Game

[![C++](https://img.shields.io/badge/language-C%2B%2B17-blue)](https://en.cppreference.com/w/cpp/17)
[![UDP](https://img.shields.io/badge/networking-UDP-green)]()

A C++ real-time multiplayer maze game that features:
- ⚡ Non-blocking **UDP socket networking**
- 🧵 **Multithreading** for input and networking
- 🎮 Real-time raw terminal input using `termios`
- 🔄 Custom client-server protocol for player movement, treasure collection, and score sync

---

## 🚀 Why this project is impressive

✅ Low-level **socket programming** without game engine libraries  
✅ Custom protocol for multiplayer state sync  
✅ Thread-safe handling of input, networking, and game logic  
✅ Real-time gameplay with minimal CPU usage via sleep control  
✅ Handles multiple players, treasure spawning, and scoreboards  

---

## 🛠 Setup Instructions

### 1️⃣ Clone the repo
```bash
git clone https://github.com/abhiniveshmitra/Multiplayer-Maze-Game.git
cd Multiplayer-Maze-Game
```

---

### 2️⃣ Build the project
Make sure you have `g++` (C++17 or higher) installed:
```bash
g++ main.cpp -o maze_game -pthread
```
You may need to adjust the compile command if using separate files.

---

### 3️⃣ Run the server
Open **Terminal 1**
```bash
./maze_game server
```

---

### 4️⃣ Run a client
Open **Terminal 2**
```bash
./maze_game client 127.0.0.1 <username>
```
You can open multiple terminals to run multiple clients.

---

## ⚠️ Limitations

- In-memory game state (no persistence)
- No encryption (UDP packets sent in plaintext)
- Minimal error recovery on packet loss (UDP’s nature)

---

## 💡 Potential Extensions

- Add TCP fallback or reliability layer over UDP  
- Implement map generation / AI bots  
- Visualize maze grid in terminal with ANSI graphics  
- Add scoreboard persistence to file or database  

---

## 📂 Example Run

```plaintext
Welcome! You are Player 1 at position (2, 3)
Treasure is at position (5, 5)
You collected the treasure! Your score: 1
Scores: Player 1: 1  Player 2: 0  
Leader: Player 1 with score 1
```
