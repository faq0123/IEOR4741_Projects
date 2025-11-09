# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread

# Target executables
SERVER = server
CLIENT = client

# Source files
SERVER_SRC = server.cpp
CLIENT_SRC = client.cpp

# Header files
HEADERS = order.h

# Default target - build both server and client
all: $(SERVER) $(CLIENT)
	@echo "=========================================="
	@echo "Build completed successfully!"
	@echo "=========================================="
	@echo "Run './server' to start the server"
	@echo "Run './client' to start a client"
	@echo "=========================================="

# Build server
$(SERVER): $(SERVER_SRC) $(HEADERS)
	@echo "Building server..."
	$(CXX) $(CXXFLAGS) $(SERVER_SRC) -o $(SERVER)

# Build client
$(CLIENT): $(CLIENT_SRC) $(HEADERS)
	@echo "Building client..."
	$(CXX) $(CXXFLAGS) $(CLIENT_SRC) -o $(CLIENT)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(SERVER) $(CLIENT)
	@echo "Clean completed!"

# Run server
run-server: $(SERVER)
	@echo "Starting server..."
	./$(SERVER)

# Run client
run-client: $(CLIENT)
	@echo "Starting client..."
	./$(CLIENT)

# Help target
help:
	@echo "=========================================="
	@echo "TCP Trading System - Makefile Help"
	@echo "=========================================="
	@echo "Available targets:"
	@echo "  make          - Build both server and client"
	@echo "  make server   - Build only the server"
	@echo "  make client   - Build only the client"
	@echo "  make clean    - Remove built executables"
	@echo "  make run-server - Build and run server"
	@echo "  make run-client - Build and run client"
	@echo "  make help     - Show this help message"
	@echo "=========================================="

.PHONY: all clean run-server run-client help
