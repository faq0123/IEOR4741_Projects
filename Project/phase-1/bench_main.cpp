//  Created by Katherine Z on 9/13/25.


// bench_main.cpp
/*
 * Part 2 benchmark driver.
 *   - Sizes: {64, 256, 1024} as small/medium/large examples
 *   - For each size:
 *         outer_trials = 5: regenerate inputs (different data each trial)
 *         inner_repeats = 20: repeat timing on the same inputs
 *      Collect all 5×20 samples per kernel and report mean/stdev (CSV).
 */

#include <stdio.h>
#include <iostream>
#include <vector>
#include <random>
#include <cstddef>

#include "kernels.hpp"
#include "linalg_utils.hpp"
#include "bench.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::size_t;
using std::mt19937;
using std::uniform_real_distribution;


//------------------------------------------------------------------------------
// Run outer×inner benchmark for a given size n and append per-run samples.
// For each kernel, this fills its sample vector with 5×20 single-run times.
static void run_for_size_append_samples(int n, int outer_trials, int inner_repeats,
                                        vector<double>& mv_row_ms, vector<double>& mv_col_ms,
                                        vector<double>& mm_naive_ms, vector<double>& mm_bt_ms) {
    
    const unsigned base_seed = 12345u;

    for (int trial = 0; trial < outer_trials; ++trial) {
        mt19937 rng_trial(base_seed + 0x9E3779B9u * static_cast<unsigned>(trial));

        // Generate one full input bundle
        BenchmarkInput in = make_benchmark_input(n, rng_trial);

        vector<double> y(static_cast<size_t>(n));
        vector<double> C(static_cast<size_t>(n) * n);

        // func 1: MV row-major
        for (int r = 0; r < inner_repeats; ++r) {
            // Time only the kernel call
            double t = time_once_ms([&]{
                multiply_mv_row_major(in.r_matrixA.data(), n, n,
                                      in.vec.data(), y.data());
            });
            mv_row_ms.push_back(t);

            // here do checksum outside the timing window to prevent DCE
            volatile double sink = checksum(y.data(), static_cast<size_t>(n));
            (void)sink;
        }

        // func 2: MV col-major
        for (int r = 0; r < inner_repeats; ++r) {
            double t = time_once_ms([&]{
                multiply_mv_col_major(in.c_matrixA.data(), n, n,
                                      in.vec.data(), y.data());
            });
            mv_col_ms.push_back(t);
            volatile double sink = checksum(y.data(), static_cast<size_t>(n));
            (void)sink;
        }

        // func 3: MM naive (row-major)
        for (int r = 0; r < inner_repeats; ++r) {
            double t = time_once_ms([&]{
                multiply_mm_naive(in.r_matrixA.data(), n, n,
                                  in.matrixB.data(), n, n,
                                  C.data());
            });
            mm_naive_ms.push_back(t);
            volatile double sink = checksum(C.data(), static_cast<size_t>(n) * n);
            (void)sink;
        }

        // func 4: MM with transposed B (row-major)
        for (int r = 0; r < inner_repeats; ++r) {
            double t = time_once_ms([&]{
                multiply_mm_transposed_b(in.r_matrixA.data(), n, n,
                                         in.T_matrixB.data(), n, n,
                                         C.data());
            });
            mm_bt_ms.push_back(t);
            volatile double sink = checksum(C.data(), static_cast<size_t>(n) * n);
            (void)sink;
        }
    }
}


int main() {
    // Config — sizes and repetition counts
    const int sizes[] = {64, 128, 256};    // small / medium / large
    const int outer_trials  = 5;           // outer: regenerate inputs (different)
    const int inner_repeats = 20;          // inner:repeat on same inputs

    cout << "kernel,size,mean_ms,stdev_ms\n"; // CSV header

    for (int n : sizes) {
        vector<double> mv_row_ms;   mv_row_ms.reserve(outer_trials * inner_repeats);
        vector<double> mv_col_ms;   mv_col_ms.reserve(outer_trials * inner_repeats);
        vector<double> mm_naive_ms; mm_naive_ms.reserve(outer_trials * inner_repeats);
        vector<double> mm_bt_ms;    mm_bt_ms.reserve(outer_trials * inner_repeats);

        // Run the benchmark and collect samples (5×20 per kernel).
        run_for_size_append_samples(n, outer_trials, inner_repeats, mv_row_ms, mv_col_ms,
                                    mm_naive_ms, mm_bt_ms);

        // Aggregate mean&stdev over all samples per kernel.
        BenchStats s_mv_row = bench_stats(mv_row_ms);
        BenchStats s_mv_col = bench_stats(mv_col_ms);
        BenchStats s_mm_na  = bench_stats(mm_naive_ms);
        BenchStats s_mm_bt  = bench_stats(mm_bt_ms);

        // Output
        cout << "mv_row,"  << n << "," << s_mv_row.mean_ms << "," << s_mv_row.stdev_ms << "\n";
        cout << "mv_col,"  << n << "," << s_mv_col.mean_ms << "," << s_mv_col.stdev_ms << "\n";
        cout << "mm_naive,"<< n << "," << s_mm_na.mean_ms  << "," << s_mm_na.stdev_ms  << "\n";
        cout << "mm_bt,"   << n << "," << s_mm_bt.mean_ms  << "," << s_mm_bt.stdev_ms  << "\n";
    }

    return 0;
}

