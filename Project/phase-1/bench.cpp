//  Created by Katherine Z on 9/13/25.

/*
 * bench.cpp
 * -----------------------------------------------------------------------------
 * EN: Implementations for Part 2 benchmarking helpers:
 *     - bench_stats: compute mean and standard deviation (ms)
 *     - random data generators: matrix (row-major) and vector
 *     - make_benchmark_input: build the full input bundle for size n
 */

#include "bench.hpp"
#include "linalg_utils.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <vector>
using std::vector; using std::size_t;
using std::max; using std::sqrt; using std::swap;
using std::mt19937; using std::uniform_real_distribution;



// Compute mean and standard deviation from a list of elapsed times (ms).
//     If input is empty, returns {0, 0}.
BenchStats bench_stats(const vector<double>& times_ms) {
    const size_t n = times_ms.size();
    if (n == 0) return {0.0, 0.0};

    double sum = 0.0;
    double sum2 = 0.0;
    for (double x : times_ms) {
        sum  += x;
        sum2 += x * x;
    }
    const double inv_n = 1.0 / static_cast<double>(n);
    const double mean  = sum * inv_n;
    const double var   = max(0.0, sum2 * inv_n - mean * mean);
    return { mean, sqrt(var) };
}

//------------------------------------------------------------------------------
// Generate input data
//------------------------------------------------------------------------------

// Generate an n×n row-major matrix with elements ~ U[lo, hi].
//   - If n <= 0, returns an empty vector.
// for non-square matrix
vector<double> make_random_matrix_row_major(int rows, int cols,
                                              std::mt19937& rng,
                                              double lo, double hi) {
    vector<double> M;
    if (rows <= 0 || cols <= 0) return M;
    
    if (hi < lo) std::swap(lo, hi);
    uniform_real_distribution<double> dist(lo, hi);

    M.resize(static_cast<size_t>(rows) * static_cast<size_t>(cols));
    for (double& v : M) v = dist(rng);
    return M;
}

// reload for sqaure matrix
vector<double> make_random_matrix_row_major(int n, mt19937& rng,
                                            double lo, double hi) {
    return make_random_matrix_row_major(n, n, rng, lo, hi);
}


// Generate a length-n vector with elements ~ U[lo, hi].
//   - If n <= 0, returns an empty vector.
vector<double> make_random_vector(int n, mt19937& rng,
                                  double lo, double hi) {
    vector<double> x;
    if (n <= 0) return x;

    if (hi < lo) std::swap(lo, hi);
    uniform_real_distribution<double> dist(lo, hi);

    x.resize(static_cast<size_t>(n));
    for (double& v : x) v = dist(rng);
    return x;
}


// Build the full input bundle for size n, Returns an empty bundle if n <= 0.
//   - r_matrixA  (n×n row-major)
//   - c_matrixA  (A in col-major layout)
//   - vec        (length n)
//   - matrixB    (n×n row-major)
//   - T_matrixB  (B^T, row-major)
BenchmarkInput make_benchmark_input(int n, mt19937& rng) {
    BenchmarkInput in;
    if (n <= 0) return in;

    // Generate A, x, B with fixed RNG (passed by reference for reproducibility).
    in.r_matrixA = make_random_matrix_row_major(n, rng);
    in.vec       = make_random_vector(n, rng);
    in.matrixB   = make_random_matrix_row_major(n, rng);

    const size_t nn = static_cast<size_t>(n) * static_cast<size_t>(n);
    in.c_matrixA.resize(nn);
    in.T_matrixB.resize(nn);

    row_to_col_major(in.r_matrixA.data(), n, n, in.c_matrixA.data());  // col-major A
    transpose_row_major(in.matrixB.data(), n, n, in.T_matrixB.data()); // B^T

    return in;
}


