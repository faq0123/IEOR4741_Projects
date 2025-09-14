//
//  kernals.hpp
//  phase-1
//
//  Created by Katherine Z on 9/12/25.
//

#ifndef kernals_hpp
#define kernals_hpp

#include <stdio.h>

// Matrix-Vector Multiplication (Row-Major)
void multiply_mv_row_major(const double* matrix, int rows, int cols, const double* vector, double* result);

// Matrix-Vector Multiplication (Column-Major)
void multiply_mv_col_major(const double* matrix, int rows, int cols, const double* vector, double* result);

// Matrix-Matrix Multiplication (Naive)
void multiply_mm_naive(const double* matrixA, int rowsA, int colsA, const double* matrixB, int rowsB, int colsB, double* result);

// Matrix-Matrix Multiplication (Transposed B)
void multiply_mm_transposed_b(const double* matrixA, int rowsA, int colsA, const double* matrixB_transposed, int rowsB, int colsB, double* result);

// Optimized variant: loop-reordered (i-k-j) accumulation to improve cache reuse of A rows and B rows.
void multiply_mm_loop_reordered(const double* matrixA, int rowsA, int colsA,
								const double* matrixB, int rowsB, int colsB,
								double* result);

// Optimized variant: blocked / tiled multiplication.
// block_size controls the tile edge length (e.g., 32, 48, 64). Caller ensures block_size > 0.
void multiply_mm_tiled(const double* matrixA, int rowsA, int colsA,
					   const double* matrixB, int rowsB, int colsB,
					   double* result, int block_size);

#endif /* kernals_hpp */
