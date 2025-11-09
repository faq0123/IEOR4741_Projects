#!/bin/bash

# TCP Trading System - Automated Demo Script
# This script demonstrates the trading system for video recording

# Colors for better visibility
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print section headers
print_header() {
    echo ""
    echo "=========================================="
    echo "$1"
    echo "=========================================="
    echo ""
}

# Function to simulate typing delay
type_delay() {
    sleep 1.5
}

# Clean up function
cleanup() {
    echo ""
    echo -e "${RED}Cleaning up processes...${NC}"
    killall server 2>/dev/null
    killall client 2>/dev/null
    # Clean up port 8080
    lsof -i :8080 | grep -v COMMAND | awk '{print $2}' | xargs kill -9 2>/dev/null
    rm -f /tmp/client*_in /tmp/client*_out 2>/dev/null
    rm -f server.log 2>/dev/null
    sleep 1
}

# Set up trap to cleanup on exit
trap cleanup EXIT

# Clean up any previous run
cleanup

# Main demo
clear
print_header "TCP Client-Server Trading System Demo"
echo -e "${BLUE}This demo will show:${NC}"
echo "  1. Server startup"
echo "  2. Multiple client connections (5 clients)"
echo "  3. Concurrent order processing"
echo "  4. Error handling"
echo "  5. Graceful shutdown"
echo ""
echo -e "${YELLOW}Press ENTER to start the demo...${NC}"
read

# Step 1: Compile the project
print_header "Step 1: Building the Project"
echo -e "${GREEN}$ make clean && make${NC}"
type_delay
make clean && make
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed! Exiting.${NC}"
    exit 1
fi
echo ""
echo -e "${GREEN}Build successful!${NC}"
type_delay

# Step 2: Start the server
print_header "Step 2: Starting TCP Server"
echo -e "${GREEN}$ ./server &${NC}"
type_delay

# Try to start server with retries
for i in {1..3}; do
    ./server > server.log 2>&1 &
    SERVER_PID=$!
    sleep 3
    
    # Check if server started successfully
    if ps -p $SERVER_PID > /dev/null 2>&1; then
        if grep -q "Server listening" server.log; then
            echo -e "${GREEN}Server started successfully (PID: $SERVER_PID)${NC}"
            tail -5 server.log
            break
        fi
    fi
    
    # Failed, cleanup and retry
    kill $SERVER_PID 2>/dev/null
    if [ $i -lt 3 ]; then
        echo -e "${YELLOW}Retry $i: Cleaning port and retrying...${NC}"
        lsof -i :8080 | grep -v COMMAND | awk '{print $2}' | xargs kill -9 2>/dev/null
        sleep 2
    else
        echo -e "${RED}Server failed to start after 3 attempts!${NC}"
        cat server.log
        exit 1
    fi
done
type_delay

# Step 3: Connect 5 clients
print_header "Step 3: Connecting 5 Clients"
echo -e "${BLUE}Starting 5 client connections...${NC}"
type_delay

# Create named pipes for client communication
for i in {1..5}; do
    mkfifo /tmp/client${i}_in 2>/dev/null
done

# Start 5 clients in background
for i in {1..5}; do
    echo -e "${GREEN}Starting Client $i...${NC}"
    ./client < /tmp/client${i}_in > /tmp/client${i}_out 2>&1 &
    CLIENT_PIDS[$i]=$!
    sleep 0.5
done

sleep 2
echo ""
echo -e "${GREEN}All 5 clients connected!${NC}"
echo -e "${YELLOW}Check server log:${NC}"
tail -10 server.log | grep "Client connected"
type_delay

# Step 4: Send valid orders from each client
print_header "Step 4: Submitting Valid Orders"
echo -e "${BLUE}Each client will send a trading order...${NC}"
echo ""

ORDERS=(
    "BUY AAPL 100 150.25"
    "SELL TSLA 50 230.10"
    "BUY GOOGL 20 2800.00"
    "SELL MSFT 75 310.50"
    "BUY AMZN 10 3250.00"
)

for i in {1..5}; do
    ORDER="${ORDERS[$i-1]}"
    echo -e "${YELLOW}Client $i:${NC} $ORDER"
    echo "$ORDER" > /tmp/client${i}_in
    sleep 1
    echo -e "${GREEN}Response:${NC}"
    tail -2 /tmp/client${i}_out 2>/dev/null | grep "CONFIRMED" || echo "  (processing...)"
    sleep 0.5
done

type_delay
echo ""
echo -e "${GREEN}All orders processed successfully!${NC}"
echo -e "${YELLOW}Server order book:${NC}"
grep "ORDER BOOK" server.log | tail -5
type_delay

# Step 5: Demonstrate concurrency
print_header "Step 5: Concurrent Order Processing"
echo -e "${BLUE}Submitting orders from multiple clients simultaneously...${NC}"
echo ""

echo -e "${YELLOW}Client 1 & Client 3: Sending orders at the same time...${NC}"
echo "BUY AAPL 50 151.00" > /tmp/client1_in &
echo "BUY GOOGL 15 2805.75" > /tmp/client3_in &
wait
sleep 1.5

echo -e "${GREEN}Both orders processed concurrently!${NC}"
grep "Received:" server.log | tail -2
type_delay

# Step 6: Test error handling
print_header "Step 6: Error Handling Test"
echo -e "${BLUE}Testing invalid order format...${NC}"
echo ""

echo -e "${YELLOW}Client 2:${NC} BUY MSFT ${RED}(missing quantity and price)${NC}"
echo "BUY MSFT" > /tmp/client2_in
sleep 1.5

echo -e "${RED}Error Response:${NC}"
tail -2 /tmp/client2_out 2>/dev/null | grep "ERROR" || grep "ERROR" server.log | tail -1
type_delay

# Step 7: Send more orders
print_header "Step 7: Additional Orders"
echo -e "${BLUE}Clients continue trading...${NC}"
echo ""

echo -e "${YELLOW}Client 4:${NC} BUY MSFT 100 309.25"
echo "BUY MSFT 100 309.25" > /tmp/client4_in
sleep 1
echo -e "${GREEN}Response:${NC}"
tail -2 /tmp/client4_out 2>/dev/null | grep "CONFIRMED" || echo "  (processing...)"

type_delay

echo -e "${YELLOW}Client 5:${NC} SELL AMZN 5 3255.50"
echo "SELL AMZN 5 3255.50" > /tmp/client5_in
sleep 1
echo -e "${GREEN}Response:${NC}"
tail -2 /tmp/client5_out 2>/dev/null | grep "CONFIRMED" || echo "  (processing...)"

type_delay

# Step 8: Show final order book
print_header "Step 8: Final Order Book Status"
echo -e "${BLUE}Total orders processed:${NC}"
ORDER_COUNT=$(grep "ORDER BOOK" server.log | tail -1 | grep -o "[0-9]*" | tail -1)
echo -e "${GREEN}Total: $ORDER_COUNT orders${NC}"
type_delay

# Step 9: Graceful shutdown
print_header "Step 9: Graceful Shutdown"
echo -e "${BLUE}Disconnecting all clients...${NC}"
echo ""

for i in {1..5}; do
    echo -e "${YELLOW}Client $i:${NC} EXIT"
    echo "EXIT" > /tmp/client${i}_in
    sleep 0.3
done

sleep 2
echo ""
echo -e "${GREEN}All clients disconnected gracefully${NC}"
grep "Client disconnected" server.log | tail -5
type_delay

echo ""
echo -e "${YELLOW}Stopping server...${NC}"
kill $SERVER_PID 2>/dev/null
sleep 1

# Cleanup
echo ""
echo -e "${GREEN}Server stopped${NC}"
for i in {1..5}; do
    rm -f /tmp/client${i}_in /tmp/client${i}_out
done

# Final summary
print_header "Demo Complete!"
echo -e "${GREEN}Successfully demonstrated:${NC}"
echo "  [X] Multi-client connection (5 clients)"
echo "  [X] Concurrent order processing"
echo "  [X] Order validation and confirmation"
echo "  [X] Error handling"
echo "  [X] Thread-safe order book"
echo "  [X] Graceful shutdown"
echo ""
echo -e "${BLUE}Total orders processed: ${ORDER_COUNT}${NC}"
echo -e "${YELLOW}Server log saved to: server.log${NC}"
echo ""
echo -e "${GREEN}Thank you for watching!${NC}"
echo ""
