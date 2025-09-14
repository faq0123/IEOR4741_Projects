//
//  linalg_utils.hpp
//  phase-1
//
//  Created by Katherine Z on 9/13/25.

#ifndef linalg_utils_hpp
#define linalg_utils_hpp
#pragma once
#include <stdio.h>
#include <cstddef>

// BT = B^T
void transpose_row_major(const double* B, int rowsB, int colsB, double* BT);

void row_to_col_major(const double* Arow, int rows, int cols, double* Acol);

void fill_sequential(double* data, int n, double start = 1.0);

void fill_constant(double* data, int n, double value);

bool arrays_almost_equal(const double* a, const double* b, std::size_t n, double eps = 1e-12);


#endif /* linalg_utils_hpp */
