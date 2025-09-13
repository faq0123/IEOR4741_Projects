#include "lin_alg.h"
#include <chrono>
#include <vector>
#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
#include <memory>  // For aligned_alloc
#include <windows.h>  // For Windows UTF-8 output

struct BenchmarkResult {
    double avg_time_ms;
    double std_dev_ms;
};

BenchmarkResult benchmark(void (*func)(const double*, int, int, const double*, double*), 
                         int rows, int cols, int runs = 10) {
    std::vector<double> times;
    std::mt19937 gen(42);  // Fixed seed for reproducibility
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double* mat = static_cast<double*>(std::aligned_alloc(64, rows * cols * sizeof(double)));  // 64B aligned
    double* vec = static_cast<double*>(std::aligned_alloc(64, cols * sizeof(double)));
    double* res = static_cast<double*>(std::aligned_alloc(64, rows * sizeof(double)));
    if (!mat || !vec || !res) throw std::bad_alloc();

    for (int i = 0; i < rows * cols; ++i) mat[i] = dis(gen);
    for (int i = 0; i < cols; ++i) vec[i] = dis(gen);
    for (int i = 0; i < rows; ++i) res[i] = 0.0;

    for (int r = 0; r < runs; ++r) {
        auto start = std::chrono::high_resolution_clock::now();
        func(mat, rows, cols, vec, res);
        auto end = std::chrono::high_resolution_clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }

    double avg = 0.0, variance = 0.0;
    for (double t : times) avg += t;
    avg /= runs;
    for (double t : times) variance += (t - avg) * (t - avg);
    variance = std::sqrt(variance / (runs - 1));

    std::free(mat); std::free(vec); std::free(res);
    return {avg, variance};
}

BenchmarkResult benchmark_mm(void (*func)(const double*, int, int, const double*, int, int, double*), 
                            int rowsA, int colsA, int colsB, int runs = 10) {
    std::vector<double> times;
    std::mt19937 gen(42);
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double* A = static_cast<double*>(std::aligned_alloc(64, rowsA * colsA * sizeof(double)));
    double* B = static_cast<double*>(std::aligned_alloc(64, colsA * colsB * sizeof(double)));
    double* res = static_cast<double*>(std::aligned_alloc(64, rowsA * colsB * sizeof(double)));
    if (!A || !B || !res) throw std::bad_alloc();

    for (int i = 0; i < rowsA * colsA; ++i) A[i] = dis(gen);
    for (int i = 0; i < colsA * colsB; ++i) B[i] = dis(gen);
    for (int i = 0; i < rowsA * colsB; ++i) res[i] = 0.0;

    for (int r = 0; r < runs; ++r) {
        auto start = std::chrono::high_resolution_clock::now();
        func(A, rowsA, colsA, B, colsA, colsB, res);
        auto end = std::chrono::high_resolution_clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }

    double avg = 0.0, variance = 0.0;
    for (double t : times) avg += t;
    avg /= runs;
    for (double t : times) variance += (t - avg) * (t - avg);
    variance = std::sqrt(variance / (runs - 1));

    std::free(A); std::free(B); std::free(res);
    return {avg, variance};
}

int main() {
    SetConsoleOutputCP(65001);  // Fix Chinese output
    std::cout << "Running benchmark test...\n";
    std::cout << std::setw(10) << "Size" << std::setw(20) << "Function" 
              << std::setw(15) << "Avg Time (ms)" << std::setw(15) << "Std Dev (ms)" << std::endl;

    std::vector<int> sizes = {64, 512, 1024};  // Test sizes
    for (int size : sizes) {
        // MV benchmarks
        auto mv_row = benchmark(multiply_mv_row_major, size, size);
        auto mv_col = benchmark(multiply_mv_col_major, size, size);
        std::cout << std::setw(10) << size << std::setw(20) << "MV Row-Major" 
                  << std::setw(15) << mv_row.avg_time_ms << std::setw(15) << mv_row.std_dev_ms << std::endl;
        std::cout << std::setw(10) << size << std::setw(20) << "MV Col-Major" 
                  << std::setw(15) << mv_col.avg_time_ms << std::setw(15) << mv_col.std_dev_ms << std::endl;

        // MM benchmarks
        auto mm_naive = benchmark_mm(multiply_mm_naive, size, size, size);
        auto mm_trans = benchmark_mm(multiply_mm_transposed_b, size, size, size);
        auto mm_tiled = benchmark_mm(multiply_mm_tiled, size, size, size);
        std::cout << std::setw(10) << size << std::setw(20) << "MM Naive" 
                  << std::setw(15) << mm_naive.avg_time_ms << std::setw(15) << mm_naive.std_dev_ms << std::endl;
        std::cout << std::setw(10) << size << std::setw(20) << "MM Transposed B" 
                  << std::setw(15) << mm_trans.avg_time_ms << std::setw(15) << mm_trans.std_dev_ms << std::endl;
        std::cout << std::setw(10) << size << std::setw(20) << "MM Tiled" 
                  << std::setw(15) << mm_tiled.avg_time_ms << std::setw(15) << mm_tiled.std_dev_ms << std::endl;
    }

    std::cout << "Benchmark test finished.\n";
    return 0;
}