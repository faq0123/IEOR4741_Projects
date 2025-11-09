#!/bin/bash

# Sample test orders for TCP Trading System

echo "=========================================="
echo "TCP Trading System - Sample Test Orders"
echo "=========================================="
echo ""

echo "=== Client 1 Orders ==="
echo "BUY AAPL 100 150.25"
echo "BUY AAPL 50 151.00"
echo ""

echo "=== Client 2 Orders ==="
echo "SELL TSLA 50 230.10"
echo "SELL TSLA 25 231.50"
echo ""

echo "=== Client 3 Orders ==="
echo "BUY GOOGL 20 2800.00"
echo "BUY GOOGL 15 2805.75"
echo ""

echo "=== Client 4 Orders ==="
echo "SELL MSFT 75 310.50"
echo "BUY MSFT 100 309.25"
echo ""

echo "=== Client 5 Orders ==="
echo "BUY AMZN 10 3250.00"
echo "SELL AMZN 5 3255.50"
echo ""

echo "=== Invalid Order Examples ==="
echo "BUY MSFT         (missing quantity and price)"
echo "SELL AAPL -10 150  (negative quantity)"
echo ""

echo "=========================================="
echo "Demo Sequence:"
echo "=========================================="
echo "1. Start server: ./server"
echo "2. Start 5 clients: ./client (in 5 terminals)"
echo "3. Submit one order from each client"
echo "4. Show concurrent processing"
echo "5. Test invalid order"
echo "6. Exit all clients: EXIT"
echo "=========================================="
