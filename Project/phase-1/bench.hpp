//  Created by Katherine Z on 9/13/25.
#pragma once
/*
 * bench.hpp
 * -----------------------------------------------------------------------------
 * Declarations for Part 2 benchmarking.
 *   - Timing helpers (std::chrono)
 *   - Repeated-run benchmarking (warmups + repeats)
 *   - Input bundle type for kernels (BenchmarkInput) and data generators
 * -----------------------------------------------------------------------------
 */

#include <vector>
#include <chrono>
#include <cstddef>  // std::size_t
#include <random>

//------------------------------------------------------------------------------
// Time and output data
//------------------------------------------------------------------------------
// Aggregated statistics of multiple runs (milliseconds)
struct BenchStats {
    double mean_ms;
    double stdev_ms;
};

// Compute mean and standard deviation from a list of samples (ms).
BenchStats bench_stats(const std::vector<double>& times_ms);

// Time a callable once and return elapsed milliseconds.
// Only the body of `fn()` is measured for accuracy.
template <typename Fn>
inline double time_once_ms(Fn&& fn) {
    using clock = std::chrono::steady_clock; // monotonic clock
    const auto t0 = clock::now();
    fn();                                    // region under test
    const auto t1 = clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// Run a benchmark with warmups and repeated timed runs.
//   - warmups: run `fn()` this many times without recording (cache warmup)
//   - repeats: run `fn()` this many times and record each elapsed time (ms)
//   - out_samples_ms (optional): if non-null, receives all per-run samples
//   Returns: mean & stdev across recorded runs.
template <typename Fn>
inline BenchStats run_benchmark(Fn&& fn, int warmups = 2, int repeats = 20,
                                std::vector<double>* out_samples_ms = nullptr) {
    // Warm-up (not timed)
    for (int i = 0; i < warmups; ++i) {
        fn();
    }

    // imed runs
    std::vector<double> samples;
    samples.reserve(repeats);
    for (int i = 0; i < repeats; ++i) {
        samples.push_back(time_once_ms(fn));
    }

    if (out_samples_ms) {
        *out_samples_ms = samples; // EN: return raw samples; ZH: 返回原始样本
    }
    return bench_stats(samples);    // EN: aggregate; ZH: 统计聚合
}

// checksum to prevent dead-code elimination in benchmarks.
// just sums all elements and returns the scalar result.
inline double checksum(const double* data, std::size_t n) {
    double s = 0.0;
    for (std::size_t i = 0; i < n; ++i) s += data[i];
    return s;
}


//------------------------------------------------------------------------------
// Input Data Generators
//------------------------------------------------------------------------------

struct BenchmarkInput {
    std::vector<double> r_matrixA; // n×n, row-major matrix A
    std::vector<double> c_matrixA; // n×n, col-major layout of A
    std::vector<double> vec;       // n-vector
    std::vector<double> matrixB;   // n×n, row-major matrix B
    std::vector<double> T_matrixB; // n×n, row-major B^T
};

// Generators
//   - make_random_matrix_row_major: create n×n row-major matrix with U[lo,hi]
//   - make_random_vector:           create length-n vector with U[lo,hi]
//   - make_benchmark_input:         build the full bundle for size n
// for non-square matrix
std::vector<double>make_random_matrix_row_major(int rows, int cols, std::mt19937& rng,
                                                  double lo = -1.0, double hi = 1.0);
// for square matrix
std::vector<double> make_random_matrix_row_major(int n, std::mt19937& rng,
                                                 double lo = -1.0,double hi = 1.0);

std::vector<double> make_random_vector(int n, std::mt19937& rng,
                                       double lo = -1.0, double hi = 1.0);

BenchmarkInput make_benchmark_input(int n, std::mt19937& rng);


