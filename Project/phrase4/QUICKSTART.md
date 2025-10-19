# Quick Start Guide - HFT Trading System
## Get Running in 5 Minutes

---

##  Quick Commands

```bash
# 1. Navigate to project
cd phrase4

# 2. Build (Release mode)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

# 3. Run main simulation
cd ..
./bin/hft_app

# 4. Run latency benchmarks
./bin/test_latency
```

---

##  What You'll See

### Main Simulation Output

```
╔════════════════════════════════════════════════════════════╗
║    High-Frequency Trading System - Phase 4 Project        ║
║                  IEOR E4741                                ║
╚════════════════════════════════════════════════════════════╝

*** Running Basic HFT Simulation ***
Number of ticks: 10000

============================================================
Tick-to-Trade Latency Analysis (nanoseconds)
============================================================
Sample Size:     10000
Min:             394 ns
Max:             7881743 ns
Mean:            3822.71 ns
Std Dev:         91503.40 ns
Median (P50):    856 ns
P95:             2941 ns
P99:             12801 ns
============================================================

=== Trade Summary ===
Total Trades: 8453
Total Volume: 641380 shares
Total Value: $94528525.19
Average Price: $147.38
===================
```

### Files Generated

```
trades_basic.log        - Basic simulation trades
trades_aggressive.log   - Aggressive matching trades
trades_stress.log       - 100K tick stress test trades
```

---

##  Exploring the Code

### Key Files to Review

1. **`include/Order.hpp`** - Template-based order structure
   - Shows compile-time type checking
   - Smart pointer usage

2. **`include/OrderBook.hpp`** - Limit order book with memory pool
   - Custom allocator implementation
   - Cache-optimized design

3. **`include/MatchingEngine.hpp`** - Core matching logic
   - Price-time priority algorithm
   - Partial fill handling

4. **`src/main.cpp`** - Full system integration
   - Shows all components working together
   - Latency measurement examples

---

##  Run Different Tests

### Modify Test Parameters

Edit `src/main.cpp`:

```cpp
// Change simulation size
runBasicSimulation(10000);  // Try 1000, 50000, etc.

// Change stress test size
for (int i = 0; i < 100000; ++i) {  // Try 10000, 500000
```

### Run Specific Benchmarks

Edit `test/test_latency.cpp`:

```cpp
testBasicLatency(10000);     // Adjust iteration count
testHighLoadLatency(50000);  // Test with more load
```

---

##  Interpreting Results

### Good Latency Numbers

| Metric | Target | Your System |
|--------|--------|-------------|
| P50 | < 1 μs |  Check output |
| P99 | < 10 μs |  Check output |
| Mean | < 5 μs |  Check output |

### What Affects Latency?

- **Order Book Depth**: More orders = higher latency (O(log n))
- **System Load**: Background processes increase variance
- **Compiler Optimization**: Use Release build (-O3)
- **Hardware**: Faster CPU = lower latency

---

##  Troubleshooting

### Build Fails

```bash
# Try:
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make VERBOSE=1
```

### High Latency Numbers

1. Make sure you're using Release build:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

2. Close other applications to reduce system load

3. Check if running on battery power (may throttle CPU)

### Missing Dependencies

```bash
# macOS
xcode-select --install

# Linux (Ubuntu/Debian)
sudo apt-get install build-essential cmake

# Check C++ compiler
c++ --version  # Should be C++17 compatible
```

---

##  Next Steps

1. **Read the README.md** - Full architecture explanation
2. **Review ARCHITECTURE.md** - System design diagrams
3. **Study PERFORMANCE_REPORT.md** - Detailed benchmarks
4. **Modify the code** - Experiment with different configurations

---

##  Common Experiments

### 1. Change Order Book Container

In `OrderBook.hpp`, try replacing `std::multimap` with other containers and compare performance.

### 2. Adjust Memory Pool Size

```cpp
explicit OrderBook(const std::string& sym) 
    : symbol(sym), memory_pool(1024) {}  // Try 512, 2048, etc.
```

### 3. Disable Cache Alignment

Remove `alignas(64)` from `MarketData.hpp` and measure impact:

```cpp
struct MarketData {  // Remove alignas(64)
    // ...
};
```

### 4. Batch Size Tuning

In `main.cpp`:

```cpp
TradeLoggerType trade_logger("trades_basic.log", true, 100);  
// Try different batch sizes: 10, 100, 1000, 10000
```

---

##  Pro Tips

### Get Better Performance

```bash
# Use maximum optimization
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"

# Run with high priority (macOS)
sudo nice -n -20 ./bin/hft_app

# Run with real-time priority (Linux, requires sudo)
sudo chrt -f 99 ./bin/hft_app
```

### Profile the Code

```bash
# macOS
instruments -t "Time Profiler" ./bin/hft_app

# Linux
perf record ./bin/hft_app
perf report

# Valgrind (check for memory leaks)
valgrind --leak-check=full ./bin/hft_app
```

---

##  Learning Objectives Checklist

After exploring this project, you should understand:

- [ ] Template metaprogramming in C++
- [ ] Smart pointers (unique_ptr, shared_ptr) usage
- [ ] RAII pattern for resource management
- [ ] Memory pool allocation strategies
- [ ] Cache-line alignment benefits
- [ ] High-resolution timing in C++
- [ ] STL container performance characteristics
- [ ] Compile-time vs runtime checks
- [ ] Move semantics and perfect forwarding
- [ ] CMake build system basics

---

##  Getting Help

### Debugging Tips

1. **Enable debug symbols**:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Debug
   ```

2. **Add debug output**:
   ```cpp
   std::cout << "Debug: order_id=" << order->id << std::endl;
   ```

3. **Use assertions**:
   ```cpp
   assert(order != nullptr && "Order must not be null");
   ```

### Code Understanding

Start with these files in order:
1. `Timer.hpp` - Simplest component
2. `Order.hpp` - Template basics
3. `MarketData.hpp` - Data structures
4. `OrderBook.hpp` - Container usage
5. `MatchingEngine.hpp` - Core algorithm
6. `main.cpp` - Full integration

---

##  Happy Coding!

This HFT system demonstrates modern C++ best practices for high-performance applications. Experiment, measure, and learn!

**Key Takeaway**: Modern C++ allows you to write **safe, fast, and maintainable** code without sacrificing performance.

---

*IEOR E4741 - Columbia University - Phase 4 Project*
