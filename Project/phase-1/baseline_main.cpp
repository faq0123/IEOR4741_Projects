//  main.cpp
//  phase-1
//
//  Created by Katherine Z on 9/12/25.
//

#include <iostream>
#include <cstdlib>
#include "kernels.hpp"
#include "linalg_utils.hpp"

int main() {
    using std::cout; using std::endl;

    // ---------- test 1：MV（row-major） ----------
    {
        int rows = 3, cols = 4;
        double* A = new double[rows * cols];
        double* x = new double[cols];
        double* y = new double[rows];

        // A = [2,3,4,...]；x = all 1
        fill_sequential(A, rows * cols, 2.0);
        fill_constant(x, cols, 1.0);

        multiply_mv_row_major(A, rows, cols, x, y);

        double* Acol = new double[rows * cols];
        double* y_ref = new double[rows];
        row_to_col_major(A, rows, cols, Acol);
        multiply_mv_col_major(Acol, rows, cols, x, y_ref);

        bool ok = arrays_almost_equal(y, y_ref, rows);
        cout << "[Test MV row-major] " << (ok ? "PASS" : "FAIL") << endl;

        delete[] A; delete[] x; delete[] y; delete[] Acol; delete[] y_ref;
    }

    // ---------- test 2：MV（col-major） ----------
    {
        int rows = 5, cols = 3;
        double* Arow = new double[rows * cols];
        double* Acol = new double[rows * cols];
        double* x = new double[cols];
        double* y = new double[rows];
        double* y_ref = new double[rows];

        fill_sequential(Arow, rows * cols, 1.0);
        row_to_col_major(Arow, rows, cols, Acol);
        fill_sequential(x, cols, 1.0); // x = [1,2,3]

        multiply_mv_col_major(Acol, rows, cols, x, y);      // col-major
        multiply_mv_row_major(Arow, rows, cols, x, y_ref);  // row-major as ref

        bool ok = arrays_almost_equal(y, y_ref, rows);
        cout << "[Test MV col-major] " << (ok ? "PASS" : "FAIL") << endl;

        delete[] Arow; delete[] Acol; delete[] x; delete[] y; delete[] y_ref;
    }

    // ---------- test 3：MM（native） ----------
    {
        int rowsA = 3, colsA = 4;
        int rowsB = 4, colsB = 2;

        double* A = new double[rowsA * colsA];
        double* B = new double[rowsB * colsB];
        double* C = new double[rowsA * colsB];
        double* C_ref = new double[rowsA * colsB];

        fill_sequential(A, rowsA * colsA, 2.0); // A = 2,3,4,...
        fill_constant(B, rowsB * colsB, 1.0);   // B = all 1s

        multiply_mm_naive(A, rowsA, colsA, B, rowsB, colsB, C);

        // use B^T as ref
        double* BT = new double[colsB * rowsB];
        transpose_row_major(B, rowsB, colsB, BT);
        multiply_mm_transposed_b(A, rowsA, colsA, BT, rowsB, colsB, C_ref);

        bool ok = arrays_almost_equal(C, C_ref, rowsA * colsB);
        cout << "[Test MM naive vs BT] " << (ok ? "PASS" : "FAIL") << endl;

        delete[] A; delete[] B; delete[] C; delete[] C_ref; delete[] BT;
    }

    return 0;
}

