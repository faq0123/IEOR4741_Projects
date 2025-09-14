//  baseline_main.cpp
//  phase-1
//
//  Golden-case + cross-impl tests
//

#include <iostream>
#include <cstdlib>
#include "kernels.hpp"
#include "linalg_utils.hpp"

int main() {
    using std::cout; using std::endl;

    // =========================================================================
    // utuls: print to check error
    // =========================================================================
    auto print_vec = [](const char* name, const double* a, int n) {
        std::cout << name << " = [";
        for (int i = 0; i < n; ++i) {
            std::cout << a[i] << (i + 1 == n ? "" : ", ");
        }
        std::cout << "]\n";
    };

    auto print_mat = [&](const char* name, const double* a, int rows, int cols) {
        std::cout << name << " = \n";
        for (int i = 0; i < rows; ++i) {
            std::cout << "  [";
            for (int j = 0; j < cols; ++j) {
                std::cout << a[i * cols + j] << (j + 1 == cols ? "" : ", ");
            }
            std::cout << "]\n";
        }
    };

    
    // =========================================================================
    // GOLDEN test to check calculation
    // =========================================================================

    // 1) GOLDEN: MV（row-major）
    //    A = [[1,2,3],[4,5,6]],  x = [7,8,9]
    //    result = A*x = [50, 122]
    {
        const int rows = 2, cols = 3;
        double A[rows * cols] = {1,2,3, 4,5,6};   // row-major
        double x[cols]        = {7,8,9};
        double y[rows]        = {0,0};
        const double y_expect[rows] = {50, 122};

        multiply_mv_row_major(A, rows, cols, x, y);
        bool ok = arrays_almost_equal(y, y_expect, rows);
        cout << "[GOLDEN MV row-major] " << (ok ? "PASS" : "FAIL") << endl;
        if (!ok) { print_vec("y_got", y, rows); print_vec("y_exp", y_expect, rows); }
    }

    
    // 2) GOLDEN: MV（column-major）same as above
    //    col0=[1,4], col1=[2,5], col2=[3,6]
    {
        const int rows = 2, cols = 3;
        double Acol[rows * cols] = {1,4, 2,5, 3,6}; // column-major layout
        double x[cols]           = {7,8,9};
        double y[rows]           = {0,0};
        const double y_expect[rows] = {50, 122};

        multiply_mv_col_major(Acol, rows, cols, x, y);
        bool ok = arrays_almost_equal(y, y_expect, rows);
        cout << "[GOLDEN MV col-major] " << (ok ? "PASS" : "FAIL") << endl;
        if (!ok) { print_vec("y_got", y, rows); print_vec("y_exp", y_expect, rows); }
    }

    // 3) GOLDEN: MM（naive）
    //    A = [[1,2,3],
    //         [4,5,6]]  (2x3)
    //    B = [[7, 8],
    //         [9,10],
    //         [11,12]]  (3x2)
    //    result = A*B = [[58, 64],
    //                    [139,154]]  (2x2)
    {
        const int rowsA = 2, colsA = 3;
        const int rowsB = 3, colsB = 2;
        double A[rowsA * colsA] = {1,2,3, 4,5,6};          // row-major
        double B[rowsB * colsB] = {7,8, 9,10, 11,12};      // row-major
        double C[rowsA * colsB] = {0};
        const double C_expect[rowsA * colsB] = {58,64, 139,154};

        multiply_mm_naive(A, rowsA, colsA, B, rowsB, colsB, C);
        bool ok = arrays_almost_equal(C, C_expect, rowsA * colsB);
        cout << "[GOLDEN MM naive] " << (ok ? "PASS" : "FAIL") << endl;
        if (!ok) { print_mat("C_got", C, rowsA, colsB); print_mat("C_exp", C_expect, rowsA, colsB); }
    }

    // 4) GOLDEN: MM（B^T）same as above
    //    B^T = [[7, 9,11],
    //           [8,10,12]]  (2x3)
    // =========================================================================
    {
        const int rowsA = 2, colsA = 3;
        const int rowsB = 3, colsB = 2;
        double A[rowsA * colsA]  = {1,2,3, 4,5,6};
        double BT[colsB * rowsB] = {7,9,11, 8,10,12}; // row-major for B^T (2x3)
        double C[rowsA * colsB]  = {0};
        const double C_expect[rowsA * colsB] = {58,64, 139,154};

        multiply_mm_transposed_b(A, rowsA, colsA, BT, rowsB, colsB, C);
        bool ok = arrays_almost_equal(C, C_expect, rowsA * colsB);
        cout << "[GOLDEN MM BT] " << (ok ? "PASS" : "FAIL") << endl;
        if (!ok) { print_mat("C_got", C, rowsA, colsB); print_mat("C_exp", C_expect, rowsA, colsB); }
    }

    // =========================================================================
    // Cross-Implementation Test
    // =========================================================================

    // ---------- test 1：MV（Compare row-major & col-major） ----------
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
        cout << "[Cross MV row-major] " << (ok ? "PASS" : "FAIL") << endl;

        delete[] A; delete[] x; delete[] y; delete[] Acol; delete[] y_ref;
    }

    // ---------- test 2：MV（Compare col-major & row-major） ----------
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
        cout << "[Cross MV col-major] " << (ok ? "PASS" : "FAIL") << endl;

        delete[] Arow; delete[] Acol; delete[] x; delete[] y; delete[] y_ref;
    }

    // ---------- test 3：MM（Compare naive & BT） ----------
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
        cout << "[Cross MM naive vs BT] " << (ok ? "PASS" : "FAIL") << endl;

        delete[] A; delete[] B; delete[] C; delete[] C_ref; delete[] BT;
    }

    cout << "All tests completed.\n";
    return 0;
}
