#ifndef UDP_HELPER_H
#define UDP_HELPER_H

#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include "common.h"

// Structure to store client information
struct ClientInfo {
    struct sockaddr_in addr;
    socklen_t addrLen;
    int playerId;

    ClientInfo() : addrLen(sizeof(addr)), playerId(-1) {
        memset(&addr, 0, sizeof(addr));
    }

    ClientInfo(const struct sockaddr_in& _addr, int _id) 
        : addrLen(sizeof(addr)), playerId(_id) {
        addr = _addr;
    }

    bool operator==(const ClientInfo& other) const {
        return addr.sin_addr.s_addr == other.addr.sin_addr.s_addr && 
               addr.sin_port == other.addr.sin_port;
    }
};

// Helper class for UDP server operations
class UDPServer {
private:
    int sockfd;
    struct sockaddr_in serverAddr;
    std::map<int, ClientInfo> clients;  // Map player ID to client info

    // Set socket to non-blocking mode
    bool setNonBlocking(int sock) {
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags == -1) {
            std::cerr << "Error getting socket flags" << std::endl;
            return false;
        }

        flags |= O_NONBLOCK;
        if (fcntl(sock, F_SETFL, flags) == -1) {
            std::cerr << "Error setting socket to non-blocking mode" << std::endl;
            return false;
        }

        return true;
    }

public:
    UDPServer(int port = DEFAULT_PORT) : sockfd(-1) {
        // Create UDP socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            std::cerr << "Error creating socket" << std::endl;
            exit(EXIT_FAILURE);
        }

        // Configure server address
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        serverAddr.sin_port = htons(port);

        // Bind socket
        if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Error binding socket" << std::endl;
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Set socket to non-blocking mode
        if (!setNonBlocking(sockfd)) {
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        std::cout << "UDP server initialized on port " << port << std::endl;
    }

    ~UDPServer() {
        if (sockfd >= 0) {
            close(sockfd);
        }
    }

    // Receive message with timeout
    bool receiveMessage(std::string& message, ClientInfo& clientInfo, int timeoutMs = 100) {
        fd_set readfds;
        struct timeval tv;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        tv.tv_sec = 0;
        tv.tv_usec = timeoutMs * 1000;

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0) {
            std::cerr << "Select error" << std::endl;
            return false;
        }

        if (activity == 0) {
            // Timeout, no data available
            return false;
        }

        if (FD_ISSET(sockfd, &readfds)) {
            char buffer[MAX_BUFFER_SIZE];
            memset(buffer, 0, MAX_BUFFER_SIZE);

            int bytesReceived = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0,
                                        (struct sockaddr*)&clientInfo.addr, &clientInfo.addrLen);

            if (bytesReceived > 0) {
                message = std::string(buffer, bytesReceived);
                return true;
            }
        }

        return false;
    }

    // Send message to specific client
    bool sendMessage(const ClientInfo& clientInfo, const std::string& message) {
        int bytesSent = sendto(sockfd, message.c_str(), message.length(), 0,
                              (struct sockaddr*)&clientInfo.addr, clientInfo.addrLen);

        return bytesSent == static_cast<int>(message.length());
    }

    // Register client
    void registerClient(int playerId, const ClientInfo& clientInfo) {
        clients[playerId] = clientInfo;
    }

    // Get client by player ID
    ClientInfo* getClient(int playerId) {
        auto it = clients.find(playerId);
        if (it != clients.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // Remove client
    void removeClient(int playerId) {
        clients.erase(playerId);
    }

    // Broadcast message to all clients
    void broadcastMessage(const std::string& message) {
        for (const auto& pair : clients) {
            sendMessage(pair.second, message);
        }
    }
};

// Helper class for UDP client operations
class UDPClient {
private:
    int sockfd;
    struct sockaddr_in serverAddr;

    // Set socket to non-blocking mode
    bool setNonBlocking(int sock) {
        int flags = fcntl(sock, F_GETFL, 0);
        if (flags == -1) {
            std::cerr << "Error getting socket flags" << std::endl;
            return false;
        }

        flags |= O_NONBLOCK;
        if (fcntl(sock, F_SETFL, flags) == -1) {
            std::cerr << "Error setting socket to non-blocking mode" << std::endl;
            return false;
        }

        return true;
    }

public:
    UDPClient(const std::string& serverIP, int port = DEFAULT_PORT) : sockfd(-1) {
        // Create UDP socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            std::cerr << "Error creating socket" << std::endl;
            exit(EXIT_FAILURE);
        }

        // Configure server address
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);

        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
            std::cerr << "Invalid address/ Address not supported" << std::endl;
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // Set socket to non-blocking mode
        if (!setNonBlocking(sockfd)) {
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

    ~UDPClient() {
        if (sockfd >= 0) {
            close(sockfd);
        }
    }

    // Send message to server
    bool sendMessage(const std::string& message) {
        int bytesSent = sendto(sockfd, message.c_str(), message.length(), 0,
                              (struct sockaddr*)&serverAddr, sizeof(serverAddr));

        return bytesSent == static_cast<int>(message.length());
    }

    // Receive message with timeout
    bool receiveMessage(std::string& message, int timeoutMs = 100) {
        fd_set readfds;
        struct timeval tv;

        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        tv.tv_sec = 0;
        tv.tv_usec = timeoutMs * 1000;

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0) {
            std::cerr << "Select error" << std::endl;
            return false;
        }

        if (activity == 0) {
            // Timeout, no data available
            return false;
        }

        if (FD_ISSET(sockfd, &readfds)) {
            char buffer[MAX_BUFFER_SIZE];
            memset(buffer, 0, MAX_BUFFER_SIZE);

            struct sockaddr_in serverResponseAddr;
            socklen_t serverLen = sizeof(serverResponseAddr);

            int bytesReceived = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0,
                                        (struct sockaddr*)&serverResponseAddr, &serverLen);

            if (bytesReceived > 0) {
                message = std::string(buffer, bytesReceived);
                return true;
            }
        }

        return false;
    }
};

#endif // UDP_HELPER_H
