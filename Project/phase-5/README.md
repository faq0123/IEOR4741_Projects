# Phase 5 — Order Book Benchmark

## Overview
This phase implements and compares two versions of an order book system in C++:
- **BaselineOrderBook** — straightforward reference version.  
- **OptimizedOrderBook** — improved version with reduced memory copies, pre-reserved hash tables, and better cache locality.

## Files
| File | Description |
|------|--------------|
| `order_book.cpp` | Main C++ source file containing both order book implementations, unit tests, and benchmarks. |
| `Performance_Analysis.pdf` | Report summarizing execution time comparisons, optimization effectiveness, and latency breakdowns. |
| `Unit_Stress_Test.pdf` | Report showing unit test and stress test outputs verifying accuracy and stability. |

## How to Run
```bash
g++ -O3 -std=c++11 order_book.cpp -o orderbook
./orderbook
```

This will execute:
- All unit tests  
- Stress tests up to 100,000 orders  
- Benchmark results printed in console

## Visualization
A benchmark chart of execution time vs. order volume is included in the performance report  
(`Performance_Analysis.pdf`).

## Notes
- The optimized version shows up to **2× speedup** for small workloads.  
- At large scales, both versions converge due to `std::map` insertion cost.  
- All tests passed successfully; no crashes or memory leaks observed.
