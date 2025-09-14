//  Created by Katherine Z on 9/13/25.

// cache_locality_main.cpp
/*
 * Task 2 — Cache locality specific benchmarks (standalone main)
 *     - Experiment A: MV row-sweep (fix cols, grow rows) to show row-major vs col-major
 *     - Experiment C: Stride microbenchmark to show how larger strides hurt locality
 */

#include <iostream>
#include <vector>
#include <random>
#include <cstddef>
#include <stdio.h>

#include <algorithm>

#include "kernels.hpp"
#include "linalg_utils.hpp"
#include "bench.hpp"

using std::cout; using std::cerr; using std::endl;
using std::vector; using std::size_t; using std::mt19937;
using std::uniform_real_distribution;

// ==========================================================================
// Experiment A: MV row-sweep (fix cols, grow rows)
// Output: scenario,kernel,rows,cols,mean_ms,stdev_ms
// ==========================================================================
static void run_mv_rowsweep_and_report(const vector<int>& rows_list, int cols,
                                       int outer_trials, int inner_repeats) {
    const unsigned base_seed = 24680u;

    // CSV header
    cout << "scenario,kernel,rows,cols,mean_ms,stdev_ms\n";

    for (int rows : rows_list) {
        vector<double> mv_row_ms; mv_row_ms.reserve(outer_trials * inner_repeats);
        vector<double> mv_col_ms; mv_col_ms.reserve(outer_trials * inner_repeats);

        for (int trial = 0; trial < outer_trials; ++trial) {
            mt19937 rng_trial(base_seed
                              + 0x9E3779B9u * static_cast<unsigned>(trial)
                              + 17u * static_cast<unsigned>(rows));

            // Generate row-major A and x; then build Acol (column-major layout) once per trial
            vector<double> A   = make_random_matrix_row_major(rows, cols, rng_trial);
            vector<double> x   = make_random_vector(cols, rng_trial);
            vector<double> Acol(static_cast<size_t>(rows) * cols);
            row_to_col_major(A.data(), rows, cols, Acol.data());

            vector<double> y(static_cast<size_t>(rows));

            // Row-major MV
            for (int r = 0; r < inner_repeats; ++r) {
                double t = time_once_ms([&]{
                    multiply_mv_row_major(A.data(), rows, cols, x.data(), y.data());
                });
                mv_row_ms.push_back(t);
                volatile double sink = checksum(y.data(), static_cast<size_t>(rows)); (void)sink;
            }

            // Column-major MV (i-outer, j-inner on column-major storage => large stride on A)
            for (int r = 0; r < inner_repeats; ++r) {
                double t = time_once_ms([&]{
                    multiply_mv_col_major(Acol.data(), rows, cols, x.data(), y.data());
                });
                mv_col_ms.push_back(t);
                volatile double sink = checksum(y.data(), static_cast<size_t>(rows)); (void)sink;
            }
        }

        BenchStats s_row = bench_stats(mv_row_ms);
        BenchStats s_col = bench_stats(mv_col_ms);

        cout << "mv_rowsweep,mv_row," << rows << "," << cols << ","
             << s_row.mean_ms << "," << s_row.stdev_ms << "\n";
        cout << "mv_rowsweep,mv_col," << rows << "," << cols << ","
             << s_col.mean_ms << "," << s_col.stdev_ms << "\n";
    }
}

// ==========================================================================
// Experiment C: Stride microbenchmark (ns per element)
// Output: scenario,stride_doubles,mean_ns_per_elem,stdev_ns_per_elem
// ==========================================================================
static void run_stride_bench_and_report(size_t N, const vector<size_t>& strides,
                                        int outer_trials,int inner_repeats) {
    
    auto is_power_of_two = [](size_t x){ return x && ((x & (x-1)) == 0); };
    if (!is_power_of_two(N)) {
        cerr << "[Warn] N is not a power of two; consider N=1<<21.\n";
    }
    const size_t mask = N - 1; // valid if N is power-of-two

    const unsigned base_seed = 13579u;
    mt19937 rng(base_seed);
    uniform_real_distribution<double> dist(-1.0, 1.0);

    vector<double> a(N);
    for (double& v : a) v = dist(rng);

    // CSV header
    cout << "scenario,stride_doubles,mean_ns_per_elem,stdev_ns_per_elem\n";

    for (size_t s : strides) {
        vector<double> samples_ns_per_elem; samples_ns_per_elem.reserve(outer_trials * inner_repeats);

        for (int trial = 0; trial < outer_trials; ++trial) {
            // warmup
            volatile double warm_sink = 0.0;
            for (size_t i = 0; i < (1u<<20); ++i) warm_sink += a[(i*s) & mask];

            for (int r = 0; r < inner_repeats; ++r) {
                double last_sum = 0.0;
                double t_ms = time_once_ms([&]{
                    double sum = 0.0;
                    // Fix total number of touches to N; stride changes the jump distance
                    for (size_t i = 0; i < N; ++i) {
                        sum += a[(i * s) & mask];
                    }
                    last_sum = sum;
                });
                volatile double sink = last_sum; (void)sink;

                // ms -> ns per element
                double ns_per_elem = (t_ms * 1e6) / static_cast<double>(N);
                samples_ns_per_elem.push_back(ns_per_elem);
            }
        }

        BenchStats st = bench_stats(samples_ns_per_elem);
        // bench_stats returns ms, we already converted each sample to ns/elem, so ×1e6 keeps units consistent.
        cout << "stride_bench," << s << "," << st.mean_ms * 1e6 << "," << st.stdev_ms * 1e6 << "\n";
    }
}



int main() {
    // ------------------ Experiment A: MV row-sweep ------------------
    {
        // Choose a mid-size cols; rows cross L1/L2 to highlight cache effects.
        const int fixed_cols          = 256;
        const vector<int> rows_list   = {1024, 4096, 16384}; // y ≈ 8KB, 32KB, 128KB
        const int outer_trials        = 3;
        const int inner_repeats       = 10;
        run_mv_rowsweep_and_report(rows_list, fixed_cols, outer_trials, inner_repeats);
    }

    // ------------------ Experiment C: Stride microbenchmark ------------------
    {
        const size_t N                = (1u << 21);           // ~16 MiB
        const vector<size_t> strides  = {1, 2, 4, 8, 16, 32}; // doubles; 8 ≈ one 64B cache line
        const int outer_trials        = 3;
        const int inner_repeats       = 5;
        run_stride_bench_and_report(N, strides, outer_trials, inner_repeats);
    }

    return 0;
}
