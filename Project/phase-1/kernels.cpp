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

// Loop-reordered (i-k-j) matrix multiplication.
// Reorders loops to stream through B rows contiguously while accumulating into C row segments.
void multiply_mm_loop_reordered(const double* matrixA, int rowsA, int colsA,
                                const double* matrixB, int rowsB, int colsB,
                                double* result) {
    if (!matrixA || !matrixB || !result) {
        std::cerr << "[Error] Null pointer passed to multiply_mm_loop_reordered.\n";
        return;
    }
    if (rowsA <= 0 || colsA <= 0 || rowsB <= 0 || colsB <= 0) {
        std::cerr << "[Error] Invalid matrix dimensions.\n";
        return;
    }
    if (colsA != rowsB) {
        std::cerr << "[Error] Incompatible dimensions: A(" << rowsA << "x" << colsA
                  << ") * B(" << rowsB << "x" << colsB << ").\n";
        return;
    }

    const size_t nC = static_cast<size_t>(rowsA) * static_cast<size_t>(colsB);
    memset(result, 0, sizeof(double) * nC);

    // i-k-j ordering: keep A(i,k) in a register, walk across row segment of C and row of B
    for (int i = 0; i < rowsA; ++i) {
        const double* Ai = matrixA + static_cast<size_t>(i) * colsA;
        double* Ci = result + static_cast<size_t>(i) * colsB;
        for (int k = 0; k < colsA; ++k) {
            const double a = Ai[k];         // A(i,k)
            const double* Bk = matrixB + static_cast<size_t>(k) * colsB; // row k of B
            for (int j = 0; j < colsB; ++j) {
                Ci[j] += a * Bk[j];
            }
        }
    }
}

// Blocked / tiled matrix multiplication (row-major matrices)
// Splits the i, k, j loops into tiles of size block_size to increase cache reuse.
void multiply_mm_tiled(const double* matrixA, int rowsA, int colsA,
                       const double* matrixB, int rowsB, int colsB,
                       double* result, int block_size) {
    if (!matrixA || !matrixB || !result) {
        std::cerr << "[Error] Null pointer passed to multiply_mm_tiled.\n";
        return;
    }
    if (rowsA <= 0 || colsA <= 0 || rowsB <= 0 || colsB <= 0 || block_size <= 0) {
        std::cerr << "[Error] Invalid matrix dimensions or block size.\n";
        return;
    }
    if (colsA != rowsB) {
        std::cerr << "[Error] Incompatible dimensions: A(" << rowsA << "x" << colsA
                  << ") * B(" << rowsB << "x" << colsB << ").\n";
        return;
    }

    const size_t nC = static_cast<size_t>(rowsA) * static_cast<size_t>(colsB);
    memset(result, 0, sizeof(double) * nC);

    // Tile loops: I,K,J outer; inner micro-kernel updates a block of C
    for (int ii = 0; ii < rowsA; ii += block_size) {
        int i_max = std::min(ii + block_size, rowsA);
        for (int kk = 0; kk < colsA; kk += block_size) {
            int k_max = std::min(kk + block_size, colsA);
            for (int jj = 0; jj < colsB; jj += block_size) {
                int j_max = std::min(jj + block_size, colsB);
                // Compute C block (ii:i_max, jj:j_max)
                for (int i = ii; i < i_max; ++i) {
                    const double* Ai = matrixA + static_cast<size_t>(i) * colsA;
                    double* Ci = result + static_cast<size_t>(i) * colsB;
                    for (int k = kk; k < k_max; ++k) {
                        const double a = Ai[k];
                        const double* Bk = matrixB + static_cast<size_t>(k) * colsB;
                        for (int j = jj; j < j_max; ++j) {
                            Ci[j] += a * Bk[j];
                        }
                    }
                }
            }
        }
    }
}
