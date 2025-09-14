//
//  kernals.cpp
//  phase-1
//
//  Created by Katherine Z on 9/12/25.
//

#include "kernels.hpp"
#include <cstddef>
#include <iostream>
#include <random>
#include <cstring>
using namespace std;


void multiply_mv_row_major(const double* matrix, int rows, int cols,
                           const double* vector, double* result) {
    // REQUIRE len(vector) == cols, len(result) == rows
    // checking for null pointers
    if (!matrix || !vector || !result) {
        cerr << "[Error] Null pointer passed to multiply_mv_row_major.\n";
        return;
    }
    // checking for valid dimensions
    if (rows <= 0 || cols <= 0) {
        std::cerr << "[Error] Invalid matrix dimensions (rows=" << rows
                  << ", cols=" << cols << ").\n";
        return;
    }
    
    // compute result = A * vector
    for (int i = 0; i < rows; i++) {
        double sum = 0.0;
        for (int j = 0; j < cols; j++) {
            sum += matrix[i * cols + j] * vector[j];
        }
        result[i] = sum;
    }
}


void multiply_mv_col_major(const double* matrix, int rows, int cols,
                           const double* vector, double* result) {
    // REQUIRE len(vector) == cols, len(result) == rows
    // checking for null pointers
    if (!matrix || !vector || !result) {
        cerr << "[Error] Null pointer passed to multiply_mv_col_major.\n";
        return;
    }
    // checking for valid dimensions
    if (rows <= 0 || cols <= 0) {
        cerr << "[Error] Invalid matrix dimensions (rows=" << rows
                  << ", cols=" << cols << ").\n";
        return;
    }
    
    // compute result = A * vector
    for (int i = 0; i < rows; i++) {
        double sum = 0.0;
        for (int j = 0; j < cols; j++) {
            sum += matrix[j * rows + i] * vector[j];
        }
        result[i] = sum;
    }

//    // initialization
//    for (int i = 0; i < rows; i++) {
//        result[i] = 0.0;
//    }
//    // compute result = A * vector
//    for (int j = 0; j < cols; j++) {
//        const double xj = vector[j];
//        for (int i = 0; i < rows; i++) {
//            result[i] += matrix[j * rows + i] * xj;
//        }
//    }
}


void multiply_mm_naive(const double* matrixA, int rowsA, int colsA,
                       const double* matrixB, int rowsB, int colsB,
                       double* result) {
    // checking for null pointers
    if (!matrixA || !matrixB || !result) {
        cerr << "[Error] Null pointer passed to multiply_mm_naive.\n";
        return;
    }
    // checking for valid dimensions
    if (rowsA <= 0 || colsA <= 0 || rowsB <= 0 || colsB <= 0) {
        cerr << "[Error] Invalid matrix dimensions.\n";
        return;
    }
    // checking for compatible dimensions
    if (colsA != rowsB) {
        cerr << "[Error] Incompatible dimensions: A(" << rowsA << "x" << colsA
                  << ") * B(" << rowsB << "x" << colsB << ").\n";
        return;
    }
    
    // initial and clean result matrix
    size_t n = static_cast<size_t>(rowsA) * static_cast<size_t>(colsB);
    memset(result, 0, sizeof(double) * n);

    // C[i,j] = sum_k A[i,k] * B[k, j]
    for (int i = 0; i < rowsA; i++) {
        for (int j = 0; j < colsB; j++) {
            double sum = 0.0;
            for (int k = 0; k < colsA; k++) {
                sum += matrixA[i * colsA + k] * matrixB[k * colsB + j];
            }
            result[i * colsB + j] = sum;
        }
    }
    
    // i-k-j, cache-friendly
//    for (int i = 0; i < rowsA; i++) {
//        for (int k = 0; k < colsA; k++) {
//            double a = matrixA[i * colsA + k]; // A(i,k)
//            for (int j = 0; j < colsB; j++) {
//                result[i * colsB + j] += a * matrixB[k * colsB + j]; // += A(i,k) * B(k,j)
//            }
//        }
//    }
}


void multiply_mm_transposed_b(const double* matrixA, int rowsA, int colsA,
                              const double* matrixB_transposed, int rowsB, int colsB,
                              double* result) {
    // checking for null pointers
    if (!matrixA || !matrixB_transposed || !result) {
        cerr << "[Error] Null pointer passed to multiply_mm_transposed_b.\n";
        return;
    }
    // checking for valid dimensions
    if (rowsA <= 0 || colsA <= 0 || rowsB <= 0 || colsB <= 0) {
        cerr << "[Error] Invalid matrix dimensions.\n";
        return;
    }
    //checking for compatible dimensions
    if (colsA != rowsB) {
        cerr << "[Error] Incompatible dimensions: A(" << rowsA << "x" << colsA
                  << ") * B(" << rowsB << "x" << colsB << ").\n";
        return;
    }

    // initial and clean result matrix
    const size_t n = static_cast<size_t>(rowsA) * static_cast<size_t>(colsB);
    memset(result, 0, sizeof(double) * n);

    // C[i,j] = sum_k A[i,k] * BT[j,k]
    for (int i = 0; i < rowsA; ++i) {
        const double* Ai = matrixA + static_cast<size_t>(i) * colsA;      // start of i-th row in A
        for (int j = 0; j < colsB; ++j) {
            const double* BTj = matrixB_transposed + static_cast<size_t>(j) * rowsB; // start of j-th row in BT
            double sum = 0.0;
            for (int k = 0; k < colsA; ++k) {
                sum += Ai[k] * BTj[k];
            }
            result[static_cast<size_t>(i) * colsB + j] = sum;
        }
    }
}
