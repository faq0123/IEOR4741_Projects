#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "order.h"

#define PORT 8080
#define BUFFER_SIZE 1024

// Global order book with thread synchronization
std::vector<Order> orderBook;
std::mutex orderBookMutex;

// Handle individual client connection
void handleClient(int clientSocket, struct sockaddr_in clientAddr) {
    char buffer[BUFFER_SIZE];
    std::string clientIP = inet_ntoa(clientAddr.sin_addr);
    int clientPort = ntohs(clientAddr.sin_port);
    
    std::cout << "\n[+] Client connected: " << clientIP << ":" << clientPort << std::endl;
    
    // Send welcome message to client
    std::string welcome = "Connected to Trading Server. Send orders in format: ORDER_TYPE SYMBOL QUANTITY PRICE\n";
    send(clientSocket, welcome.c_str(), welcome.length(), 0);
    
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesRead <= 0) {
            std::cout << "[-] Client disconnected: " << clientIP << ":" << clientPort << std::endl;
            break;
        }
        
        std::string message(buffer);
        // Remove newline characters
        message.erase(std::remove(message.begin(), message.end(), '\n'), message.end());
        message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());
        
        if (message.empty()) {
            continue;
        }
        
        std::cout << "[" << clientIP << ":" << clientPort << "] Received: " << message << std::endl;
        
        // Handle exit command
        if (message == "EXIT" || message == "QUIT") {
            std::string goodbye = "Goodbye!\n";
            send(clientSocket, goodbye.c_str(), goodbye.length(), 0);
            break;
        }
        
        // Parse and validate order
        Order order;
        std::string response;
        
        if (parseOrder(message, order)) {
            // Add to order book with thread safety
            {
                std::lock_guard<std::mutex> lock(orderBookMutex);
                orderBook.push_back(order);
                std::cout << "[ORDER BOOK] Total orders: " << orderBook.size() << std::endl;
            }
            
            // Send confirmation response
            response = "CONFIRMED: " + order.toString() + "\n";
            std::cout << "[RESPONSE] " << response;
        } else {
            // Send error response
            response = "ERROR: Invalid format. Use: ORDER_TYPE SYMBOL QUANTITY PRICE\n";
            std::cout << "[RESPONSE] " << response;
        }
        
        send(clientSocket, response.c_str(), response.length(), 0);
    }
    
    close(clientSocket);
}

// Display all orders in order book
void displayOrderBook() {
    std::cout << "\n========== ORDER BOOK ==========\n";
    std::lock_guard<std::mutex> lock(orderBookMutex);
    
    if (orderBook.empty()) {
        std::cout << "No orders in the book.\n";
    } else {
        for (size_t i = 0; i < orderBook.size(); i++) {
            std::cout << "[" << (i + 1) << "] " 
                      << orderBook[i].timestamp << " | "
                      << orderBook[i].toString() << std::endl;
        }
    }
    std::cout << "================================\n\n";
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    
    std::cout << "========================================\n";
    std::cout << "   TCP Trading Server Starting...\n";
    std::cout << "========================================\n\n";
    
    // Create TCP socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error: Could not create socket\n";
        return 1;
    }
    
    // Enable address reuse
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: setsockopt failed\n";
        close(serverSocket);
        return 1;
    }
    
    // Configure server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    
    // Bind socket to address and port
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Bind failed\n";
        close(serverSocket);
        return 1;
    }
    
    // Listen for incoming connections (queue up to 10)
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Error: Listen failed\n";
        close(serverSocket);
        return 1;
    }
    
    std::cout << "[*] Server listening on port " << PORT << std::endl;
    std::cout << "[*] Waiting for client connections...\n";
    std::cout << "[*] Type Ctrl+C to stop the server\n\n";
    
    // Store client handler threads
    std::vector<std::thread> clientThreads;
    
    // Main accept loop
    while (true) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        
        if (clientSocket < 0) {
            std::cerr << "Error: Accept failed\n";
            continue;
        }
        
        // Create new thread for client
        clientThreads.push_back(std::thread(handleClient, clientSocket, clientAddr));
        clientThreads.back().detach(); // Detach for independent execution
        
        // Display order book periodically
        if (clientThreads.size() % 5 == 0) {
            displayOrderBook();
        }
    }
    
    // Cleanup (unreachable without signal handling)
    close(serverSocket);
    
    return 0;
}
