#include "lin_alg.h"
#include <iostream>
#include <random>
#include <windows.h>

void run_profile(void (*func)(const double*, int, int, const double*, int, int, double*), 
                 const char* func_name, int rowsA, int colsA, int colsB) {
    std::mt19937 gen(42);
    std::uniform_real_distribution<> dis(0.0, 1.0);

    double* A = static_cast<double*>(std::aligned_alloc(64, rowsA * colsA * sizeof(double)));
    double* B = static_cast<double*>(std::aligned_alloc(64, colsA * colsB * sizeof(double)));
    double* res = static_cast<double*>(std::aligned_alloc(64, rowsA * colsB * sizeof(double)));
    if (!A || !B || !res) {
        std::cerr << "Allocation failed!" << std::endl;
        return;
    }

    for (int i = 0; i < rowsA * colsA; ++i) A[i] = dis(gen);
    for (int i = 0; i < colsA * colsB; ++i) B[i] = dis(gen);
    for (int i = 0; i < rowsA * colsB; ++i) res[i] = 0.0;

    func(A, rowsA, colsA, B, colsA, colsB, res);  // Run to generate gmon.out

    std::cout << "Profiling " << func_name << " done for size " << rowsA << "x" << colsB << std::endl;

    std::free(A); std::free(B); std::free(res);
}

int main() {
    SetConsoleOutputCP(65001);  // Fix Chinese output
    std::cout << "Starting profiling...\n";

    // Profile for 512x512 matrices
    run_profile(multiply_mm_naive, "MM Naive", 512, 512, 512);
    // Rename gmon.out to avoid overwrite
    system("mv gmon.out gmon_naive.out");
    
    run_profile(multiply_mm_transposed_b, "MM Transposed B", 512, 512, 512);
    system("mv gmon.out gmon_trans.out");

    std::cout << "Profiling finished.\n";
    return 0;
}