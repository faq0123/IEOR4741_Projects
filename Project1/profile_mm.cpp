#include "lin_alg.h"
#include <iostream>
#include <random>
#include <windows.h>
#include <malloc.h>  // For _aligned_malloc

void run_profile(void (*func)(const double*, int, int, const double*, int, int, double*), 
                 const char* func_name, int rowsA, int colsA, int colsB) {
    std::mt19937 gen(42);
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double* A = static_cast<double*>(_aligned_malloc(rowsA * colsA * sizeof(double), 64));
    double* B = static_cast<double*>(_aligned_malloc(colsA * colsB * sizeof(double), 64));
    double* res = static_cast<double*>(_aligned_malloc(rowsA * colsB * sizeof(double), 64));
    if (!A || !B || !res) {
        std::cerr << "Allocation failed!" << std::endl;
        return;
    }

    for (int i = 0; i < rowsA * colsA; ++i) A[i] = dis(gen);
    for (int i = 0; i < colsA * colsB; ++i) B[i] = dis(gen);
    for (int i = 0; i < rowsA * colsB; ++i) res[i] = 0.0;

    func(A, rowsA, colsA, B, colsA, colsB, res);  // Run to generate gmon.out

    std::cout << "Profiling " << func_name << " done for size " << rowsA << "x" << colsB << std::endl;

    _aligned_free(A); _aligned_free(B); _aligned_free(res);
}

int main() {
    SetConsoleOutputCP(65001);
    std::cout << "Running performance analysis...\n";

    // Profile for 512x512 matrices
    run_profile(multiply_mm_naive, "MM Naive", 512, 512, 512);
    system("mv gmon.out gmon_naive.out");
    
    run_profile(multiply_mm_transposed_b, "MM Transposed B", 512, 512, 512);
    system("mv gmon.out gmon_trans.out");

    std::cout << "Performance analysis completed!\n";
    return 0;
}