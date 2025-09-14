#include <iostream>
#include <vector>
#include <chrono>
#include <random>

const int SIZE = 4096;

// Basic function to access matrix elements
int getElement(const std::vector<std::vector<int>>& matrix, int row, int col) {
    return matrix[row][col];
}

// Basic function to add two integers
int add(int a, int b) {
    return a + b;
}

// Unoptimized summation function
long long sumMatrixBasic(const std::vector<std::vector<int>>& matrix) {
    long long sum = 0;
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            sum = add(sum, getElement(matrix, i, j));
            // sum += matrix[i][j]; // Direct access for clarity
        }
    }
    return sum;
}

// ============================================================
// Optimization ideas applied:
//  1) Use direct access instead of helper functions
//  2) Use a raw pointer to iterate inside each row
//  3) Avoid extra index lookups and function-call overhead
//  4) Keep the accumulation in a 64-bit variable to prevent overflow
// ============================================================


long long sumMatrixOptimized(const std::vector<std::vector<int>>& matrix) {
    long long total = 0;   // 64-bit accumulator, big enough for all sums

    // Loop over all rows of the matrix
    for (int i = 0; i < SIZE; ++i) {

        // --------------------------------------------------------
        // Take a raw pointer to the beginning of the current row.
        // 'matrix[i].data()' gives direct access to the row's
        // internal contiguous array, avoiding matrix[i][j] overhead.
        // --------------------------------------------------------
        const int* row = matrix[i].data();

        // --------------------------------------------------------
        // Create a pointer to one-past-the-last element in this row.
        // This lets us iterate with pointer arithmetic:
        //   while (row < end) { ...; row++; }
        // which is faster than repeatedly checking an index.
        // --------------------------------------------------------
        const int* end = row + SIZE;

        // --------------------------------------------------------
        // Traverse the entire row using a pointer:
        //   *row   -> value at current position
        //   row++  -> advance pointer to the next element
        // This avoids:
        //   - an inner loop counter variable
        //   - array index calculations (row[j])
        // --------------------------------------------------------
        while (row < end) {
            total += *row++;   // add current value and move pointer forward
        }
    }

    // Return the accumulated sum
    return total;
}



int main() {
    // Generate a large random matrix
    std::vector<std::vector<int>> matrix(SIZE, std::vector<int>(SIZE));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(-100, 100);
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            matrix[i][j] = distrib(gen);
        }
    }

    auto start = std::chrono::high_resolution_clock::now();
    long long sum = sumMatrixBasic(matrix);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Basic Sum: " << sum << std::endl;
    std::cout << "Basic Time: " << duration.count() << " milliseconds" << std::endl;

    // Students will implement their optimized version here
    auto start_optimized = std::chrono::high_resolution_clock::now();
    long long optimized_sum = sumMatrixOptimized(matrix);
    auto end_optimized = std::chrono::high_resolution_clock::now();
    auto duration_optimized = std::chrono::duration_cast<std::chrono::milliseconds>(end_optimized - start_optimized);

    std::cout << "Optimized Sum: " << optimized_sum << std::endl;
    std::cout << "Optimized Time: " << duration_optimized.count() << " milliseconds" << std::endl;

    return 0;
}
