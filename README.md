# Phase 1 — High-Performance Linear Algebra Kernels

## Overview
To implement and optimize fundamental linear algebra operations (matrix-vector multiplication 
and matrix-matrix multiplication) in C++, focusing on performance considerations such as cache locality, 
memory alignment, and the impact of compiler optimizations like inlining. Teams will analyze the performance 
of their implementations using benchmarking and profiling tools.


## Team
- **Aiqian Feng** (af3539)
- **Yiming Peng** (yp2721)
- **Yijuan Wang** (yw4443)
- **Qingyao Zhu** (qz2549)


## Repository Layout
```bash
/project-1
  # Part 1: Baseline Implementations
  ├── kernels.hpp / kernels.cpp      
  │   #  multiply_mv_row_major / multiply_mv_col_major
  │   #  multiply_mm_naive / multiply_mm_transposed_b
  ├── linalg_utils.hpp / linalg_utils.cpp
  │   # Helpers: transpose_row_major, row_to_col_major, fillers, checksum, etc.
  ├── test_main.cpp
      # Simple test cases for each function to verify its correctness.
  # Part 2: Performance Analysis and Optimization
      # Task 1: Benchmarking
  ├── bench.hpp / bench.cpp            
      # Benchmark utils: timing, stats, matrix generators
  ├── bench_main.cpp
      # Test with various sizes and perform multiple runs for each test case
  └── cache_locality_main.cpp
      # row sweep and stride benchmark cases to highlight the impact of cache locality
```


## Build Instruction

### Xcode
Suggested targets and their *Compile Sources*:
- **`part1 - baseline`**  
  `kernels.cpp`, `linalg_utils.cpp`, `baseline_main.cpp`
- **`part2 - task1 - benchmark`**  
  `kernels.cpp`, `linalg_utils.cpp`, `bench.cpp`, `bench_main.cpp`
- **`part2 - task2 - locality`**  
  `kernels.cpp`, `linalg_utils.cpp`, `bench.cpp`, `cache_locality_main.cpp`

Build configuration: **Release** (`-O3 -DNDEBUG`). Optional: `-march=native`.

### Command Line (Clang/GCC)

```bash
# Part 1 – baseline
clang++ -std=c++17 -O3 -DNDEBUG kernels.cpp linalg_utils.cpp test_main.cpp -o part1_tests

# Part 2 – benchmark
clang++ -std=c++17 -O3 -DNDEBUG kernels.cpp linalg_utils.cpp bench.cpp bench_main.cpp -o bench_main

# Part 2 – cache locality experiments
clang++ -std=c++17 -O3 -DNDEBUG kernels.cpp linalg_utils.cpp bench.cpp cache_locality_main.cpp -o cache_locality
```

Run and optionally save CSV:
```bash
./part1_tests
./bench_main         > bench.csv
./cache_locality     > locality.csv
```

## Discussion Questions 

## Acknowledgements
Assignment spec from IEOR 4741. All additional code and benchmarking scripts by the team listed above.
