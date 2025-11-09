#!/bin/bash

# Simple test to verify everything works
# Run this before recording your video

echo "=========================================="
echo "Pre-Demo Test"
echo "=========================================="
echo ""

# Clean up
echo "[1/5] Cleaning up any previous processes..."
killall server client 2>/dev/null
lsof -i :8080 | grep -v COMMAND | awk '{print $2}' | xargs kill -9 2>/dev/null
sleep 1

# Build
echo "[2/5] Building project..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "  ✓ Build successful"
else
    echo "  ✗ Build failed!"
    exit 1
fi

# Start server
echo "[3/5] Testing server..."
./server > /tmp/test_server.log 2>&1 &
SERVER_PID=$!
sleep 2

if ps -p $SERVER_PID > /dev/null 2>&1 && grep -q "Server listening" /tmp/test_server.log; then
    echo "  ✓ Server started on port 8080"
else
    echo "  ✗ Server failed to start"
    cat /tmp/test_server.log
    kill $SERVER_PID 2>/dev/null
    exit 1
fi

# Test client connection
echo "[4/5] Testing client connection..."
(sleep 1; echo "BUY AAPL 100 150.25"; sleep 1; echo "EXIT") | ./client > /tmp/test_client.log 2>&1
if grep -q "CONFIRMED" /tmp/test_client.log; then
    echo "  ✓ Client connected and order processed"
else
    echo "  ✗ Client test failed"
    cat /tmp/test_client.log
fi

# Cleanup
echo "[5/5] Cleaning up..."
kill $SERVER_PID 2>/dev/null
sleep 1
rm -f /tmp/test_*.log

echo ""
echo "=========================================="
echo "Pre-Demo Test: PASSED ✓"
echo "=========================================="
echo ""
echo "Your system is ready for demo!"
echo ""
echo "Recommended demo method:"
echo "  1. Open 6 terminal windows"
echo "  2. Terminal 1: ./server"
echo "  3. Terminals 2-6: ./client"
echo "  4. Use sample_orders.sh for test data"
echo ""
echo "Or run: ./demo_helper.sh for guided demo"
echo ""
