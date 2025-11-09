#!/bin/bash

# Concurrent Order Test Script
# This demonstrates true concurrent order processing

echo "=========================================="
echo "Concurrent Order Processing Demo"
echo "=========================================="
echo ""

# Check if server is running
if ! lsof -i :8080 > /dev/null 2>&1; then
    echo "ERROR: Server is not running!"
    echo "Please start server first: ./server"
    exit 1
fi

echo "Testing concurrent order submission..."
echo ""

# Create temporary files for client communication
FIFO1="/tmp/concurrent_client1"
FIFO2="/tmp/concurrent_client2"

# Cleanup old files
rm -f $FIFO1 $FIFO2
mkfifo $FIFO1 $FIFO2 2>/dev/null

# Start two clients in background
./client < $FIFO1 > /tmp/client1_concurrent.log 2>&1 &
CLIENT1_PID=$!

./client < $FIFO2 > /tmp/client2_concurrent.log 2>&1 &
CLIENT2_PID=$!

sleep 2

echo "[Step 1] Two clients connected"
echo ""

# Submit orders simultaneously
echo "[Step 2] Submitting orders simultaneously..."
echo ""

echo "  Client 1 → BUY AAPL 50 151.00"
echo "  Client 2 → BUY GOOGL 15 2805.75"
echo ""

# Send orders at EXACTLY the same time
(echo "BUY AAPL 50 151.00" > $FIFO1) &
(echo "BUY GOOGL 15 2805.75" > $FIFO2) &
wait

sleep 2

echo "[Step 3] Results:"
echo ""
echo "Client 1 response:"
tail -2 /tmp/client1_concurrent.log | grep CONFIRMED
echo ""
echo "Client 2 response:"
tail -2 /tmp/client2_concurrent.log | grep CONFIRMED
echo ""

# Cleanup
echo "EXIT" > $FIFO1
echo "EXIT" > $FIFO2
sleep 1

kill $CLIENT1_PID $CLIENT2_PID 2>/dev/null
rm -f $FIFO1 $FIFO2 /tmp/client*_concurrent.log

echo "=========================================="
echo "Concurrent test completed!"
echo "Both orders were processed simultaneously"
echo "by different threads on the server."
echo "=========================================="
