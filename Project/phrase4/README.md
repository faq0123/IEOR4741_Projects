# High-Frequency Trading System - Phase 4 Project
## IEOR E4741 - Advanced C++ for Financial Engineering

---

## Project Overview

This project implements a **high-performance, low-latency trading system** prototype in C++, designed for high-frequency trading (HFT) applications. The system demonstrates advanced C++ concepts including:

- **Template metaprogramming** for type-safe, generic components
- **Smart pointers** (`unique_ptr`, `shared_ptr`) for automatic memory management
- **Memory pool allocation** for reduced allocation overhead
- **Cache-line alignment** (`alignas(64)`) for optimal CPU cache utilization
- **RAII patterns** for resource safety
- **Compile-time assertions** (`static_assert`) for type safety
- **High-resolution timing** for nanosecond-precision latency measurement

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     HFT Trading System                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐       ┌──────────────────┐               │
│  │ MarketDataFeed  │──────▶│   OrderBook      │               │
│  │  (Simulator)    │       │  (Template-based)│               │
│  └─────────────────┘       └──────────┬───────┘               │
│                                       │                         │
│  ┌─────────────────┐                 │                         │
│  │ OrderManager    │◀────────────────┤                         │
│  │  (OMS)          │                 │                         │
│  └─────────────────┘                 ▼                         │
│                            ┌──────────────────┐                │
│                            │ MatchingEngine   │                │
│                            │  (Core Logic)    │                │
│                            └─────────┬────────┘                │
│                                      │                         │
│                                      ▼                         │
│                            ┌──────────────────┐                │
│                            │  TradeLogger     │                │
│                            │  (RAII-based)    │                │
│                            └──────────────────┘                │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Project Structure

```
hft_project/
├── include/                    # Header files
│   ├── MarketData.hpp         # Market data structures & simulator
│   ├── Order.hpp              # Templated order structure
│   ├── OrderBook.hpp          # Limit order book with memory pool
│   ├── MatchingEngine.hpp     # Order matching logic
│   ├── OrderManager.hpp       # Order management system
│   ├── TradeLogger.hpp        # RAII-based trade logging
│   └── Timer.hpp              # High-resolution timing utility
│
├── src/                       # Implementation files
│   ├── MarketData.cpp         # Market data simulator
│   ├── OrderBook.cpp          # (Template implementations in .hpp)
│   ├── MatchingEngine.cpp     # (Template implementations in .hpp)
│   ├── OrderManager.cpp       # (Template implementations in .hpp)
│   ├── TradeLogger.cpp        # (Template implementations in .hpp)
│   └── main.cpp               # Main simulation program
│
├── test/                      # Test programs
│   └── test_latency.cpp       # Comprehensive latency benchmarks
│
├── bin/                       # Compiled executables (created by build)
├── build/                     # Build directory (created by CMake)
├── CMakeLists.txt            # Build configuration
└── README.md                 # This file
```

---

## Build Instructions

### Prerequisites
- **C++17** compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake** 3.10 or higher
- **Make** (Unix/Linux/macOS) or **MSBuild** (Windows)

### Building on macOS/Linux

```bash
# Navigate to project directory
cd phrase4

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Executables will be in ../bin/
```

### Building on Windows

```cmd
# Navigate to project directory
cd phrase4

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -G "Visual Studio 16 2019"

# Build
cmake --build . --config Release

# Executables will be in ..\bin\
```

### Quick Build Commands

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug && make

# Release build (optimized)
cmake .. -DCMAKE_BUILD_TYPE=Release && make

# Clean build
make clean
```

---

##  Running the System

### Main Trading Simulation

```bash
# From the project root directory
./bin/hft_app
```

**This will run:**
- Basic simulation (10K ticks)
- Aggressive matching simulation (5K orders)
- Stress test (100K ticks)
- Generate trade logs: `trades_*.log`

### Latency Benchmark Tests

```bash
./bin/test_latency
```

**This will run:**
- Basic latency test
- High-load latency test
- Burst latency test
- Consistency test across different loads
- Comparative analysis

---

##  Performance Metrics

The system measures **tick-to-trade latency**: the time from receiving market data to completing an order match.

### Sample Output

```
======================================================================
Tick-to-Trade Latency Analysis (nanoseconds)
======================================================================
Sample Size:     10000
Min:             450 ns
Max:             15230 ns
Mean:            1250.50 ns
Std Dev:         890.23 ns
Median (P50):    1100 ns
P95:             2340 ns
P99:             3890 ns
======================================================================
```

### Key Metrics Explained

- **P50 (Median)**: Half of all operations complete faster than this
- **P95**: 95% of operations complete faster than this
- **P99**: Critical for HFT - 99% of operations complete faster than this
- **P99.9**: Ultra-low latency threshold

---

##  Core Components

### 1. **MarketData** (Cache-Aligned)
```cpp
struct alignas(64) MarketData {
    std::string symbol;
    double bid_price;
    double ask_price;
    // ... with high_resolution_clock timestamp
};
```
- 64-byte alignment for CPU cache optimization
- Sub-microsecond timestamp precision

### 2. **Order** (Template-Based)
```cpp
template <typename PriceType, typename OrderIdType>
struct Order {
    static_assert(std::is_integral<OrderIdType>::value, 
                  "Order ID must be an integer");
    // ... order fields
};
```
- Compile-time type checking
- Generic for different price/ID types

### 3. **OrderBook** (with Memory Pool)
- Uses `std::multimap` for O(log n) insertion/lookup
- Custom memory pool allocator to reduce allocation overhead
- Separate buy/sell order management
- Smart pointer (`unique_ptr`) ownership

### 4. **MatchingEngine**
- Price-time priority matching
- Supports partial fills
- Returns vector of executed trades
- Optimized for cache locality

### 5. **OrderManager**
- Tracks order states: NEW, PARTIAL_FILLED, FILLED, CANCELLED
- Uses `shared_ptr` for order info sharing
- Auto-incremented order IDs

### 6. **TradeLogger** (RAII)
- Automatic file management (open/close)
- Batch writing for performance
- CSV format output
- Safe resource cleanup via destructor

---

##  Experiments & Benchmarks

### Experiment 1: Load Scaling
**Goal:** Test latency consistency under increasing load

**Method:** Run with 1K, 10K, 100K ticks

**Expected Result:** P99 latency should remain stable

### Experiment 2: Memory Alignment Impact
**Goal:** Measure cache-line alignment benefit

**Method:** Compare with/without `alignas(64)`

**Expected Result:** Better cache hit rate with alignment

### Experiment 3: Smart Pointer Overhead
**Goal:** Measure smart pointer vs raw pointer performance

**Method:** Profile execution time

**Expected Result:** Minimal overhead with modern compilers

### Experiment 4: Order Book Depth
**Goal:** Test performance with varying book sizes

**Method:** Pre-populate with 0, 100, 500, 1000 orders

**Expected Result:** Logarithmic degradation (due to multimap)

---

##  Benchmark Results

### System Specifications (Example)
- **CPU:** Apple M1 / Intel i7-10700K
- **RAM:** 16GB
- **Compiler:** GCC 11.2 / Clang 13 with -O3
- **OS:** macOS Monterey / Ubuntu 22.04

### Typical Performance
| Metric | Value |
|--------|-------|
| Mean Latency | ~1-2 μs |
| P99 Latency | ~3-5 μs |
| P99.9 Latency | ~8-15 μs |
| Throughput | ~500K - 1M ticks/sec |

*Results vary based on hardware and system load*

---

##  Advanced C++ Features Used

###  Templates
- `Order<PriceType, OrderIdType>`
- `OrderBook<PriceType, OrderIdType>`
- Generic matching engine

###  Smart Pointers
- `std::unique_ptr` for exclusive ownership (orders)
- `std::shared_ptr` for shared ownership (order info)
- Automatic memory management (no manual `delete`)

###  RAII (Resource Acquisition Is Initialization)
- `TradeLogger` manages file handles
- Automatic cleanup in destructors
- Exception-safe resource management

###  Memory Pool Allocation
- Custom allocator for order objects
- Reduces heap fragmentation
- Improves allocation speed

###  Compile-Time Checks
```cpp
static_assert(std::is_integral<OrderIdType>::value, 
              "Order ID must be an integer");
```

###  Cache-Line Alignment
```cpp
struct alignas(64) MarketData { ... };
```

###  Move Semantics
- Efficient transfer of order ownership
- Avoids unnecessary copies

###  High-Resolution Timing
```cpp
std::chrono::high_resolution_clock
std::chrono::nanoseconds
```

---

##  Sample Usage

```cpp
// Create components
OrderBook<double, int> book("AAPL");
MatchingEngine<double, int> engine(book);
OrderManager<double, int> manager;
TradeLogger<double, int> logger("trades.log");

// Create and match orders
auto order = manager.createOrder("AAPL", 150.50, 100, true);
auto trades = engine.matchOrder(std::move(order));

// Log trades
logger.logTrades(trades);
```

---

##  Debugging & Profiling

### Enable Debug Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

### Use Valgrind (Linux/macOS)
```bash
valgrind --leak-check=full ./bin/hft_app
```

### Use Instruments (macOS)
```bash
instruments -t "Time Profiler" ./bin/hft_app
```

### Use perf (Linux)
```bash
perf record ./bin/hft_app
perf report
```

---

##  References & Resources

- **C++ Reference:** https://en.cppreference.com/
- **CMake Documentation:** https://cmake.org/documentation/
- **High-Frequency Trading:** "Flash Boys" by Michael Lewis
- **Low-Latency Programming:** CppCon talks on YouTube

---

##  Deliverables Checklist

- [x] Modular source code (`.hpp`/`.cpp`)
- [x] CMake build system
- [x] Main simulation (`main.cpp`)
- [x] Latency benchmark (`test_latency.cpp`)
- [x] README with architecture & instructions
- [x] Smart pointers throughout
- [x] Memory pool allocator
- [x] Cache-line alignment
- [x] Template-based order book
- [x] RAII resource management
- [x] Compile-time type checks
- [x] Tick-to-trade latency measurement
- [x] Statistical latency analysis
- [x] Trade logging (CSV format)

---

##  Contact & Support

For questions or issues:
- Check the code comments for detailed explanations
- Review the assignment specification
- Test with different compiler optimization levels
- Profile with your system's tools

---

##  Conclusion

This HFT system demonstrates a **production-grade approach** to low-latency C++ development, incorporating:

 **Modern C++17** features
 **Performance optimization** techniques  
 **Memory safety** through smart pointers  
 **Resource management** via RAII  
 **Compile-time safety** with templates and static assertions  
 **Comprehensive benchmarking** infrastructure  

---

*
