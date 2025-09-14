//
//  linalg_utils.cpp
//  phase-1
//
//  Created by Katherine Z on 9/13/25.
//

#include "linalg_utils.hpp"
#include <cmath>
#include <algorithm>
using namespace std;

void transpose_row_major(const double* B, int rowsB, int colsB, double* BT) {
    for (int i = 0; i < rowsB; ++i) {
        for (int j = 0; j < colsB; ++j) {
            BT[j * rowsB + i] = B[i * colsB + j];
        }
    }
}

void row_to_col_major(const double* Arow, int rows, int cols, double* Acol) {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            Acol[j * rows + i] = Arow[i * cols + j];
        }
    }
}

void fill_sequential(double* data, int n, double start) {
    for (int i = 0; i < n; ++i) data[i] = start + i;
}

void fill_constant(double* data, int n, double value) {
    for (int i = 0; i < n; ++i) data[i] = value;
}

bool arrays_almost_equal(const double* a, const double* b, std::size_t n, double eps) {
    for (size_t i = 0; i < n; ++i) {
        double x = a[i], y = b[i];
        double denom = 1.0 + max(fabs(x), fabs(y));
        if (fabs(x - y) > eps * denom) return false;
    }
    return true;
}
