#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include "order.h"

#define PORT 8080
#define BUFFER_SIZE 1024

// Receive messages from server in separate thread
void receiveMessages(int socket) {
    char buffer[BUFFER_SIZE];
    
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = recv(socket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesRead <= 0) {
            std::cout << "\n[!] Connection to server lost.\n";
            break;
        }
        
        std::cout << "[SERVER] " << buffer;
        std::cout.flush();
    }
}

int main(int argc, char* argv[]) {
    int clientSocket;
    struct sockaddr_in serverAddr;
    std::string serverIP = "127.0.0.1"; // Default to localhost
    
    // Allow custom server IP via command line
    if (argc > 1) {
        serverIP = argv[1];
    }
    
    std::cout << "========================================\n";
    std::cout << "   TCP Trading Client Starting...\n";
    std::cout << "========================================\n\n";
    
    // Create TCP socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Error: Could not create socket\n";
        return 1;
    }
    
    // Configure server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    
    // Convert IP address string to binary
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Error: Invalid address\n";
        close(clientSocket);
        return 1;
    }
    
    std::cout << "[*] Connecting to server " << serverIP << ":" << PORT << "...\n";
    
    // Attempt connection with retry logic
    int maxRetries = 3;
    int retryCount = 0;
    
    while (retryCount < maxRetries) {
        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            retryCount++;
            std::cerr << "[!] Connection failed (attempt " << retryCount << "/" << maxRetries << ")\n";
            
            if (retryCount < maxRetries) {
                std::cout << "[*] Retrying in 2 seconds...\n";
                sleep(2);
            } else {
                std::cerr << "[!] Could not connect to server. Is it running?\n";
                close(clientSocket);
                return 1;
            }
        } else {
            break;
        }
    }
    
    std::cout << "[+] Connected to server successfully!\n\n";
    
    // Start thread for async message receiving
    std::thread receiveThread(receiveMessages, clientSocket);
    receiveThread.detach();
    
    // Wait for welcome message
    sleep(1);
    
    // Display usage instructions
    std::cout << "\n========================================\n";
    std::cout << "Trading Client Instructions:\n";
    std::cout << "========================================\n";
    std::cout << "Format: ORDER_TYPE SYMBOL QUANTITY PRICE\n";
    std::cout << "Example: BUY AAPL 100 150.25\n";
    std::cout << "         SELL TSLA 50 230.10\n";
    std::cout << "Commands:\n";
    std::cout << "  EXIT or QUIT - Disconnect from server\n";
    std::cout << "========================================\n\n";
    
    // Main loop for user input
    std::string input;
    while (true) {
        std::cout << "Enter order (or EXIT to quit): ";
        std::getline(std::cin, input);
        
        if (input.empty()) {
            continue;
        }
        
        // Send message to server
        std::string message = input + "\n";
        if (send(clientSocket, message.c_str(), message.length(), 0) < 0) {
            std::cerr << "[!] Failed to send message\n";
            break;
        }
        
        // Check for exit command
        if (input == "EXIT" || input == "QUIT") {
            std::cout << "[*] Disconnecting...\n";
            sleep(1); // Wait for goodbye message
            break;
        }
        
        // Brief pause for response
        usleep(100000); // 100ms
    }
    
    // Cleanup
    close(clientSocket);
    std::cout << "[*] Client closed.\n";
    
    return 0;
}
