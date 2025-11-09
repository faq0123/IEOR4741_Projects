# Session 9: TCP Client-Server Trading System

**Author:** Your Name  
**Date:** November 8, 2025  
**Course:** C++ Programming

---

## Project Overview

This project implements a simplified trading system using TCP sockets in C++. The system consists of:

- **TCP Server**: Accepts multiple concurrent client connections, processes trading orders, and maintains an in-memory order book
- **TCP Client**: Connects to the server, allows users to input trading orders, and displays server responses
- **Multi-threaded Architecture**: Handles multiple clients simultaneously using C++ threads

---

## Project Structure

```
session9/
├── order.h          # Order structure and parsing utilities
├── server.cpp       # TCP server implementation
├── client.cpp       # TCP client implementation
├── Makefile         # Build configuration
└── README.md        # This file
```

---

## Build Instructions

### Prerequisites

- C++ Compiler: g++ with C++11 support
- Operating System: macOS, Linux, or Unix-like system
- Make: For building the project

### Compilation

Build both server and client:
```bash
make
```

Build individually:
```bash
make server    # Build only the server
make client    # Build only the client
```

Clean build artifacts:
```bash
make clean
```

---

## Running the System

### Step 1: Start the Server

```bash
./server
```

Expected output:
```
==========================================
   TCP Trading Server Starting...
==========================================

[*] Server listening on port 8080
[*] Waiting for client connections...
[*] Type Ctrl+C to stop the server
```

### Step 2: Start Clients

Open new terminal windows and run:
```bash
./client
```

For the 5-client demo, open 5 separate terminals.

---

## Using the Trading Client

Enter orders in this format:
```
ORDER_TYPE SYMBOL QUANTITY PRICE
```

### Valid Examples:

```
BUY AAPL 100 150.25
SELL TSLA 50 230.10
BUY GOOGL 20 2800.00
SELL MSFT 75 310.50
BUY AMZN 10 3250.00
```

### Commands:

- **EXIT** or **QUIT**: Disconnect from server

---

## Sample Test Cases

| Client Input            | Expected Server Response                 |
|------------------------|------------------------------------------|
| BUY TSLA 50 230.10     | CONFIRMED: BUY TSLA 50 @ 230.10         |
| SELL GOOGL 20 2800.00  | CONFIRMED: SELL GOOGL 20 @ 2800.00      |
| BUY AAPL 100 150.25    | CONFIRMED: BUY AAPL 100 @ 150.25        |
| BUY MSFT               | ERROR: Invalid format...                 |
| SELL AMZN -10 100      | ERROR: Invalid format...                 |

---

## Multi-Client Demo Video

### Recording Setup

1. Start server in one terminal
2. Open 5 terminal windows
3. Use screen recording software:
   - macOS: QuickTime Player
   - OBS Studio (cross-platform)
   - Zoom screen share

### Demo Script (2-5 minutes)

**Part 1: Introduction (0:00-0:30)**
- Show project structure
- Explain system architecture

**Part 2: Start Server (0:30-0:45)**
- Run `./server`
- Show server listening on port 8080

**Part 3: Connect 5 Clients (0:45-1:30)**
- Start each client one by one
- Show connection messages
- Demonstrate concurrent connections

**Part 4: Submit Orders (1:30-3:30)**
- Client 1: BUY AAPL 100 150.25
- Client 2: SELL TSLA 50 230.10
- Client 3: BUY GOOGL 20 2800.00
- Client 4: SELL MSFT 75 310.50
- Client 5: BUY AMZN 10 3250.00
- Show server processing
- Show client confirmations

**Part 5: Error Handling (3:30-4:00)**
- Send invalid order: BUY MSFT
- Show error response

**Part 6: Concurrency (4:00-4:30)**
- Send orders from multiple clients rapidly
- Show independent processing

**Part 7: Conclusion (4:30-5:00)**
- Type EXIT on all clients
- Show clean disconnection
- Stop server

### Video Checklist

- Shows 1 server + 5 clients running
- Demonstrates concurrent order processing
- Shows valid order confirmations
- Shows error handling
- Clear narration or annotations
- 2-5 minutes duration

---

## Key Features

### Server Features

- Multi-client support using threads
- Thread-safe order book with mutex
- Order validation (format, type, quantity, price)
- Real-time logging
- Graceful error handling
- Port reuse configuration

### Client Features

- Connection retry logic (3 attempts)
- Asynchronous message receiving
- Input validation
- User-friendly interface
- Clean disconnect

---

## Sample Output

### Server Output:

```
========================================
   TCP Trading Server Starting...
========================================

[*] Server listening on port 8080
[*] Waiting for client connections...

[+] Client connected: 127.0.0.1:54321

[127.0.0.1:54321] Received: BUY AAPL 100 150.25
[ORDER BOOK] Total orders: 1
[RESPONSE] CONFIRMED: BUY AAPL 100 @ 150.25
```

### Client Output:

```
========================================
   TCP Trading Client Starting...
========================================

[*] Connecting to server 127.0.0.1:8080...
[+] Connected to server successfully!

[SERVER] Connected to Trading Server.

Enter order (or EXIT to quit): BUY AAPL 100 150.25
[SERVER] CONFIRMED: BUY AAPL 100 @ 150.25

Enter order (or EXIT to quit): EXIT
[*] Disconnecting...
[*] Client closed.
```

---

## Technical Details

### Order Structure

```cpp
struct Order {
    std::string type;      // BUY or SELL
    std::string symbol;    // Stock symbol
    int quantity;          // Share quantity
    double price;          // Price per share
    std::string timestamp; // Order timestamp
};
```

### Message Protocol

**Client to Server:**
```
ORDER_TYPE SYMBOL QUANTITY PRICE\n
```

**Server to Client:**
```
CONFIRMED: ORDER_TYPE SYMBOL QUANTITY @ PRICE\n
ERROR: Invalid format. Use: ORDER_TYPE SYMBOL QUANTITY PRICE\n
```

### Thread Safety

- Mutex protection for order book
- Detached threads for clients
- Lock guards for auto management

---

## Troubleshooting

### "Bind failed" Error

**Problem:** Port 8080 in use  
**Solution:** 
- Stop other programs using port 8080
- Wait a few seconds
- Or change PORT in source code

### "Connection failed" Error

**Problem:** Server not running  
**Solution:** 
- Start server first: `./server`
- Verify server is listening

### Compilation Errors

**Problem:** Missing C++11 support  
**Solution:** Use g++ with -std=c++11 (included in Makefile)

---

## Learning Outcomes

1. TCP Socket Programming
2. Multithreading in C++
3. Thread Synchronization
4. Network Protocol Design
5. Client-Server Architecture

---

## Submission Checklist

- server.cpp with comments
- client.cpp with comments
- order.h header file
- Makefile
- README.md
- Sample logs (above)
- Multi-client demo video
- Code compiles without errors
- Runs with 5 concurrent clients

---

## Video Demo Link

**[Insert video link here]**

Platforms:
- YouTube (unlisted)
- Google Drive
- Dropbox

---

## Notes

- Default port: 8080
- Tested on macOS with g++ 11.0
- Supports unlimited clients (system limited)
- Order book persists in memory
- All connections logged with timestamps

---

## Future Enhancements

- Persistent order book
- Order matching engine
- Market depth display
- User authentication
- SSL/TLS encryption
- Web dashboard
- Order cancellation
