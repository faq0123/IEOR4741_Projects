#include "kernels.hpp"
#include <chrono>
#include <vector>
#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
#include <malloc.h>  // For _aligned_malloc on Windows/MinGW
#include <windows.h>

struct BenchmarkResult {
    double avg_time_ms;
    double std_dev_ms;
};

BenchmarkResult benchmark(void (*func)(const double*, int, int, const double*, double*), 
                         int rows, int cols, int runs = 10) {
    std::vector<double> times;
    std::mt19937 gen(42);
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double* mat = static_cast<double*>(_aligned_malloc(rows * cols * sizeof(double), 64));  // 64B aligned
    double* vec = static_cast<double*>(_aligned_malloc(cols * sizeof(double), 64));
    double* res = static_cast<double*>(_aligned_malloc(rows * sizeof(double), 64));
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

    _aligned_free(mat); _aligned_free(vec); _aligned_free(res);
    return {avg, variance};
}

BenchmarkResult benchmark_mm(void (*func)(const double*, int, int, const double*, int, int, double*), 
                            int rowsA, int colsA, int colsB, int runs = 10) {
    std::vector<double> times;
    std::mt19937 gen(42);
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double* A = static_cast<double*>(_aligned_malloc(rowsA * colsA * sizeof(double), 64));
    double* B = static_cast<double*>(_aligned_malloc(colsA * colsB * sizeof(double), 64));
    double* res = static_cast<double*>(_aligned_malloc(rowsA * colsB * sizeof(double), 64));
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

    _aligned_free(A); _aligned_free(B); _aligned_free(res);
    return {avg, variance};
}

BenchmarkResult benchmark_mm_tiled(void (*func)(const double*, int, int, const double*, int, int, double*, int), 
                                  int rowsA, int colsA, int colsB, int runs = 10) {
    std::vector<double> times;
    std::mt19937 gen(42);
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double* A = static_cast<double*>(_aligned_malloc(rowsA * colsA * sizeof(double), 64));
    double* B = static_cast<double*>(_aligned_malloc(colsA * colsB * sizeof(double), 64));
    double* res = static_cast<double*>(_aligned_malloc(rowsA * colsB * sizeof(double), 64));
    if (!A || !B || !res) throw std::bad_alloc();

    for (int i = 0; i < rowsA * colsA; ++i) A[i] = dis(gen);
    for (int i = 0; i < colsA * colsB; ++i) B[i] = dis(gen);
    for (int i = 0; i < rowsA * colsB; ++i) res[i] = 0.0;

    for (int r = 0; r < runs; ++r) {
        auto start = std::chrono::high_resolution_clock::now();
        func(A, rowsA, colsA, B, colsA, colsB, res, 64);  // Pass block_size = 64
        auto end = std::chrono::high_resolution_clock::now();
        times.push_back(std::chrono::duration<double, std::milli>(end - start).count());
    }

    double avg = 0.0, variance = 0.0;
    for (double t : times) avg += t;
    avg /= runs;
    for (double t : times) variance += (t - avg) * (t - avg);
    variance = std::sqrt(variance / (runs - 1));

    _aligned_free(A); _aligned_free(B); _aligned_free(res);
    return {avg, variance};
}

int main() {
    SetConsoleOutputCP(65001);
    std::cout << "Running benchmarks...\n";
    std::cout << std::setw(10) << "Size" << std::setw(20) << "Function" 
              << std::setw(15) << "Avg Time (ms)" << std::setw(15) << "Std Dev (ms)" << std::endl;

    std::vector<int> sizes = {64, 512, 1024};
    for (int size : sizes) {
        auto mv_row = benchmark(multiply_mv_row_major, size, size);
        auto mv_col = benchmark(multiply_mv_col_major, size, size);
        std::cout << std::setw(10) << size << std::setw(20) << "MV Row-Major" 
                  << std::setw(15) << mv_row.avg_time_ms << std::setw(15) << mv_row.std_dev_ms << std::endl;
        std::cout << std::setw(10) << size << std::setw(20) << "MV Col-Major" 
                  << std::setw(15) << mv_col.avg_time_ms << std::setw(15) << mv_col.std_dev_ms << std::endl;

        auto mm_naive = benchmark_mm(multiply_mm_naive, size, size, size);
        auto mm_trans = benchmark_mm(multiply_mm_transposed_b, size, size, size);
    auto mm_tiled = benchmark_mm_tiled(multiply_mm_tiled, size, size, size);
    auto mm_loop = benchmark_mm(multiply_mm_loop_reordered, size, size, size);
        std::cout << std::setw(10) << size << std::setw(20) << "MM Naive" 
                  << std::setw(15) << mm_naive.avg_time_ms << std::setw(15) << mm_naive.std_dev_ms << std::endl;
        std::cout << std::setw(10) << size << std::setw(20) << "MM Transposed B" 
                  << std::setw(15) << mm_trans.avg_time_ms << std::setw(15) << mm_trans.std_dev_ms << std::endl;
        std::cout << std::setw(10) << size << std::setw(20) << "MM Tiled" 
                  << std::setw(15) << mm_tiled.avg_time_ms << std::setw(15) << mm_tiled.std_dev_ms << std::endl;
        std::cout << std::setw(10) << size << std::setw(20) << "MM Loop Reordered" 
                  << std::setw(15) << mm_loop.avg_time_ms << std::setw(15) << mm_loop.std_dev_ms << std::endl;
    }

    std::cout << "Benchmarks completed!\n";
    return 0;
}
