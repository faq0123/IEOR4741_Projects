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
  ├── baseline_main.cpp
      # Simple test cases for each function to verify its correctness.
  # Part 2: Performance Analysis and Optimization
      # Task 1: Benchmarking
  ├── bench.hpp / bench.cpp            
      # Benchmark utils: timing, stats, matrix generators
  ├── bench_main.cpp
      # Test with various sizes and perform multiple runs for each test case
  └── cache_locality_main.cpp
      # row sweep and stride benchmark cases to highlight the impact of cache locality
      #Task 3&4
  ├── align_inline.cpp
      # Investigate the impact of memory alignment and the use of inline.
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
- **`part3&4 - task2 - locality`** 
  `align_inline.cpp`

Build configuration: **Release** (`-O3 -DNDEBUG`). Optional: `-march=native`.

### Command Line (Clang/GCC)

```bash
# Part 1 – baseline
clang++ -std=c++17 -O3 -DNDEBUG kernels.cpp linalg_utils.cpp test_main.cpp -o part1_tests

# Part 2 – benchmark
clang++ -std=c++17 -O3 -DNDEBUG kernels.cpp linalg_utils.cpp bench.cpp bench_main.cpp -o bench_main

# Part 2 – cache locality experiments
clang++ -std=c++17 -O3 -DNDEBUG kernels.cpp linalg_utils.cpp bench.cpp cache_locality_main.cpp -o cache_locality

# Part 2 – cache locality experiments
clang++ -std=c++17 -O3 -DNDEBUG kernels.cpp linalg_utils.cpp bench.cpp cache_locality_main.cpp -o cache_locality

# Part 3 – alignment
clang++ -std=gnu++14 -O3 align_inline.cpp -o bench
./bench
./bench --unaligned

# Part 4 – inline
clang++ -std=gnu++14 -O3 align_inline.cpp -o bench_O3
./bench_O3
clang++ -std=gnu++14 -O0 align_inline.cpp -o bench_O0
./bench_O0

```

Run and optionally save CSV:
```bash
./part1_tests
./bench_main         > bench.csv
./cache_locality     > locality.csv
```

## Discussion Questions 

### 1. Pointers vs. References
Pointers and references both let us work with data indirectly, but they behave differently.  

* **Initialization & Reassignment:**  
  A reference must be bound when created and can’t later refer to something else, while a pointer can be reassigned or set to `nullptr`.  
* **Nullability & Arithmetic:**  
  References can’t be null and don’t support arithmetic; pointers can be null and allow pointer math, which is useful for array traversal.  
* **Syntax & Lifetime:**  
  References use normal “dot” access and are typically optimized away. Pointers need `*` or `->` and must be freed if they own dynamic memory.  

In our matrix code we used pointers for dynamic buffers and when we needed to pass or allocate large arrays. References are nicer for small parameters (like a tolerance) when we don’t need to change what’s referenced.

---

### 2. Row- vs. Column-Major Order
Row-major stores rows contiguously; column-major stores columns contiguously.

* In our matrix-vector multiply, row-major allowed each inner loop to read sequential memory, which matched cache lines and ran roughly twice as fast as a column-major version.  
* In matrix-matrix multiply, the naive `i-j-k` order hit B’s columns with big strides, causing extra cache misses. Transposing B first made those accesses contiguous and cut runtime by about **20 %** in our benchmarks.

---

### 3. CPU Caches and Locality
Modern CPUs have small, fast **L1** caches per core, larger but slower **L2**, and a shared **L3**. Data moves in 64-byte cache lines.

* **Temporal locality** means reusing the same data soon after it’s fetched.  
* **Spatial locality** means touching nearby data that came in with the same cache line.  

We tried to exploit both. **Loop-reordering (ikj)** keeps `A[i][k]` in cache while scanning across `j`, and **tiling (64×64 blocks)** reuses sub-matrices so they stay in L1. Profiling showed noticeably fewer cache misses and around **30 %** speedups.

---

### 4. Memory Alignment
Alignment places data on addresses that match cache-line or hardware word boundaries (e.g., 64 bytes). Misaligned data can span two lines and require extra loads.  

We used **64-byte–aligned allocations** for our matrices; it avoided split loads and gave a small but repeatable performance bump—about **10–15 %** on the larger 1024×1024 runs.

---

### 5. Compiler Optimizations
Inlining removes function call overhead and may expose vectorization.At -O3, compiler already inlined and vectorized aggressively.-O3 was ~10× faster than -O0.

---

### 6. Profiling and Bottlenecks

---

### 7. Teamwork Reflection
We first worked together to complete the initial implementation. After that, each member focused on a different part of the analysis and optimization. In the end, we gathered our results and discussed the findings as a group.

The main benefit of this approach was efficiency, since we could work in parallel and learn from each other’s insights. The main challenges were consolidating the code into one and coordinating with teammates to compare and explain our individual results.

---

## Acknowledgements
Assignment spec from IEOR 4741. All additional code and benchmarking scripts by the team listed above.
