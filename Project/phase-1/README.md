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
  │   #  optimized: multiply_mm_loop_reordered / multiply_mm_tiled
  ├── linalg_utils.hpp / linalg_utils.cpp
  │   # Helpers: transpose_row_major, row_to_col_major, fillers, checksum, etc.
  ├── baseline_main.cpp
      # Simple golden test and cross-impl cases for each function to verify its correctness.
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
  ├── profile_mm.cpp
    # Profiling harness (Tasks 5 & 6): compares naive, transposed-B, loop-reordered, and tiled variants; reports GFLOP/s.
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
- **`part2 - task3&4 - locality`** 
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

# Part 3 – alignment
clang++ -std=gnu++14 -O3 align_inline.cpp -o bench

# Part 4 – inline
clang++ -std=gnu++14 -O3 align_inline.cpp -o bench_O3
clang++ -std=gnu++14 -O0 align_inline.cpp -o bench_O0

# Part 5 – profiling / optimization harness (MM only)
clang++ -std=c++17 -O3 -DNDEBUG kernels.cpp linalg_utils.cpp profile_mm.cpp -o profile_mm


```

Run and optionally save CSV:
```bash
./part1_tests
./bench_main         > bench.csv
./cache_locality     > locality.csv
./bench
./bench --unaligned
./bench_O3
./bench_O0
./profile_mm -n 512 -r 5 -b 64        # size=512, 5 repeats, block=64
./profile_mm -n 1024 -r 3 -b 64       # larger size
./profile_mm -n 1024 -r 3 -b 128      # compare block size
```

## Profiling & Optimization (Tasks 5 & 6)

The file `profile_mm.cpp` provides a focused harness to compare matrix–matrix multiplication variants:

Implemented kernels:
* `multiply_mm_naive` – baseline triple loop (i,j,k) with row-major A/B.
* `multiply_mm_transposed_b` – consumes pre-transposed B (improves B access locality).
* `multiply_mm_loop_reordered` – (i,k,j) ordering to reuse `A[i,k]` while streaming a row of B.
* `multiply_mm_tiled` – cache blocking with a configurable square tile size (default 64) to maximize L1/L2 reuse.

CLI arguments:
* `-n <size>` : square matrix dimension (default 512).
* `-r <repeats>` : number of timed runs per kernel (default 5).
* `-b <block>` : tile size for the tiled kernel (default 64).

Output columns:
```
kernel,n,repeats,mean_ms,stdev_ms,GFLOP/s (approx)
```
GFLOP/s is computed as 2·n^3 / time, treating one multiply+add as two floating‑point ops.

Suggested block-size sweep (edit a simple shell loop or run manually):
```
for b in 16 32 48 64 96 128; do ./profile_mm -n 1024 -r 3 -b $b; done
```
Look for the plateau—very small tiles underutilize bandwidth (high loop overhead); overly large tiles overflow caches and degrade.

Interpreting results:
* Expect `transposed_b` > `naive` due to contiguous access of both A row & B^T row.
* `loop_reordered` often narrows the gap with transposed form when B is not transposed, by improving B row streaming.
* `tiled` should win at larger sizes once n exceeds last-level cache working-set of naive versions, provided a reasonable tile size (≈ 32–128) is chosen.

Add profiling tool output (e.g., Visual Studio CPU Usage screenshots) highlighting that the hottest function corresponds to memory-bound inner loops; relate cache miss reductions (qualitatively) when moving from naive → transposed → tiled.


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



### 2. Row- vs. Column-Major Order
Row-major stores rows contiguously; column-major stores columns contiguously.

* In our matrix-vector multiply, row-major allowed each inner loop to read sequential memory, which matched cache lines and ran roughly twice as fast as a column-major version.  
* In matrix-matrix multiply, the naive `i-j-k` order hit B’s columns with big strides, causing extra cache misses. Transposing B first made those accesses contiguous and cut runtime by about **20 %** in early benchmarks. A later loop-reordered variant (`i-k-j`) plus tiling further improved locality without an explicit transpose (see Profiling & Optimization section).



### 3. CPU Caches and Locality
Modern CPUs have small, fast **L1** caches per core, larger but slower **L2**, and a shared **L3**. Data moves in 64-byte cache lines.

* **Temporal locality** means reusing the same data soon after it’s fetched.  
* **Spatial locality** means touching nearby data that came in with the same cache line.  

We tried to exploit both. **Loop-reordering (i-k-j)** keeps `A[i][k]` in a register while scanning across `j`, and **tiling (e.g., 64×64 blocks)** reuses sub-matrices so they stay in cache. Profiling (see `profile_mm.cpp`) showed noticeably fewer cache misses and up to **30–40 %** speedups over naive for larger matrices.



### 4. Memory Alignment
Alignment places data on addresses that match cache-line or hardware word boundaries (e.g., 64 bytes). Misaligned data can span two lines and require extra loads.  

We used **64-byte–aligned allocations** for our matrices; it avoided split loads and gave a small but repeatable performance bump—about **5–15 %** on the larger 1024×1024 runs (variation depends on CPU prefetcher behavior). Alignment also supports potential future SIMD.



### 5. Compiler Optimizations
Inlining removes function call overhead and may expose vectorization.At -O3, compiler already inlined and vectorized aggressively.-O3 was ~10× faster than -O0.



### 6. Profiling and Bottlenecks

In our first-pass implementations the main bottleneck was the memory hierarchy, not the floating-point arithmetic. Profiling with both the general benchmark (`bench_main.cpp`) and targeted harness (`profile_mm.cpp`) showed that access patterns dominated runtime: for matrix–vector, the row-major version (contiguous reads of `A[i,*]`) consistently beat the column-major version, and the gap grew with rows (e.g., ~14% → ~36% from 1,024 to 16,384 rows). That pointed to poor spatial locality and more cache/TLB misses in the column-major walk. For matrix–matrix, the naive kernel suffered because it reads `B[* ,j]` with a large stride in the inner loop; timing confirmed it was slower than the variant using transposed B, which turns both inner streams into contiguous reads and yielded about 1.4–1.5× speedups at the same sizes. Adding loop-reordering and later tiling reduced remaining cache misses and improved effective bandwidth.

These results guided us to focus on data layout and loop order rather than micro-tuning arithmetic: 
* prefer loop nests that read contiguous rows
* precompute Bᵀ OR use loop-reordering to avoid strided access
* apply cache blocking (tiling) once matrices exceed lower cache levels
* make small locality-friendly tweaks (e.g., hoist reused scalars) and maintain alignment
More aggressive techniques (explicit SIMD intrinsics, parallelization) are left for potential future work; the data-access changes already provided the largest gains.

### 7. Teamwork Reflection
We first worked together to complete the initial implementation. After that, each member focused on a different part of the analysis and optimization. In the end, we gathered our results and discussed the findings as a group.

The main benefit of this approach was efficiency, since we could work in parallel and learn from each other’s insights. The main challenges were consolidating the code into one and coordinating with teammates to compare and explain our individual results.

---

## Acknowledgements
Assignment spec from IEOR 4741. All additional code and benchmarking scripts by the team listed above.
