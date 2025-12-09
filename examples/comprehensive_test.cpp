#include <optiweave/prelude.hpp>
// Comprehensive test program for OptiWeave
// Tests: array access, loops, functions, data structures

#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

// Simple function with array operations
int sum_array(int* arr, int size) {
    int total = 0;
    for (int i = 0; i < size; i++) {
        total += optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 13, __FUNCTION__);
    }
    return total;
}

// Matrix multiplication (simple version)
void matrix_multiply(double** A, double** B, double** C, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(C, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 22, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 22, __FUNCTION__) = 0;
            for (int k = 0; k < N; k++) {
                optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(C, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 24, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 24, __FUNCTION__) += optiweave::ow_mul(optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(A, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 24, __FUNCTION__), k, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 24, __FUNCTION__), optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(B, k, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 24, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 24, __FUNCTION__));
            }
        }
    }
}

// Bubble sort (intentionally inefficient for profiling)
void bubble_sort(int arr[], int n) {
    for (int i = 0; i < optiweave::ow_sub(n, 1); i++) {
        for (int j = 0; j < optiweave::ow_sub(optiweave::ow_sub(n, i), 1); j++) {
            if (optiweave::__ow_subscript_impl(arr, j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 34, __FUNCTION__) > optiweave::__ow_subscript_impl(arr, optiweave::ow_add(j, 1), "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 34, __FUNCTION__)) {
                int temp = optiweave::__ow_subscript_impl(arr, j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 35, __FUNCTION__);
                optiweave::__ow_subscript_impl(arr, j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 36, __FUNCTION__) = optiweave::__ow_subscript_impl(arr, optiweave::ow_add(j, 1), "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 36, __FUNCTION__);
                optiweave::__ow_subscript_impl(arr, optiweave::ow_add(j, 1), "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 37, __FUNCTION__) = temp;
            }
        }
    }
}

// Vector operations
int process_vector(std::vector<int>& vec) {
    int sum = 0;
    for (size_t i = 0; i < vec.size(); i++) {
        vec[i] = optiweave::ow_add(optiweave::ow_mul(vec[i], 2), 1);
        sum += vec[i];
    }
    return sum;
}

// Nested array access
int find_max_in_matrix(int matrix[10][10]) {
    int max_val = optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, 0, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 55, __FUNCTION__), 0, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 55, __FUNCTION__);
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 58, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 58, __FUNCTION__) > max_val) {
                max_val = optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 59, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 59, __FUNCTION__);
            }
        }
    }
    return max_val;
}

int main() {
    std::cout << "OptiWeave Comprehensive Test Program\n";
    std::cout << "====================================\n\n";

    auto start_total = std::chrono::high_resolution_clock::now();

    // Test 1: Simple array operations
    std::cout << "Test 1: Simple array operations...\n";
    int arr1[1000];
    for (int i = 0; i < 1000; i++) {
        optiweave::__ow_subscript_impl(arr1, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 76, __FUNCTION__) = i;
    }
    int sum1 = sum_array(arr1, 1000);
    std::cout << "  Sum: " << sum1 << "\n";

    // Test 2: Bubble sort
    std::cout << "Test 2: Bubble sort (500 elements)...\n";
    int arr2[500];
    for (int i = 0; i < 500; i++) {
        optiweave::__ow_subscript_impl(arr2, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 85, __FUNCTION__) = optiweave::ow_sub(500, i);
    }
    bubble_sort(arr2, 500);
    std::cout << "  First element: " << optiweave::__ow_subscript_impl(arr2, 0, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 88, __FUNCTION__) << ", Last: " << optiweave::__ow_subscript_impl(arr2, 499, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 88, __FUNCTION__) << "\n";

    // Test 3: Vector operations
    std::cout << "Test 3: Vector operations...\n";
    std::vector<int> vec(1000);
    for (int i = 0; i < 1000; i++) {
        vec[i] = i;
    }
    int vec_sum = process_vector(vec);
    std::cout << "  Vector sum: " << vec_sum << "\n";

    // Test 4: 2D array operations
    std::cout << "Test 4: 2D array operations...\n";
    int matrix[10][10];
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 104, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 104, __FUNCTION__) = optiweave::ow_add(optiweave::ow_mul(i, 10), j);
        }
    }
    int max_val = find_max_in_matrix(matrix);
    std::cout << "  Max value: " << max_val << "\n";

    // Test 5: Matrix multiplication (small 20x20)
    std::cout << "Test 5: Matrix multiplication (20x20)...\n";
    const int N = 20;
    double** A = new double*[N];
    double** B = new double*[N];
    double** C = new double*[N];

    for (int i = 0; i < N; i++) {
        optiweave::__ow_subscript_impl(A, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 118, __FUNCTION__) = new double[N];
        optiweave::__ow_subscript_impl(B, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 119, __FUNCTION__) = new double[N];
        optiweave::__ow_subscript_impl(C, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 120, __FUNCTION__) = new double[N];
        for (int j = 0; j < N; j++) {
            optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(A, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 122, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 122, __FUNCTION__) = optiweave::ow_add(i, j);
            optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(B, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 123, __FUNCTION__), j, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 123, __FUNCTION__) = optiweave::ow_sub(i, j);
        }
    }

    matrix_multiply(A, B, C, N);
    std::cout << "  Result C[0][0]: " << optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(C, 0, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 128, __FUNCTION__), 0, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 128, __FUNCTION__) << "\n";

    // Cleanup
    for (int i = 0; i < N; i++) {
        delete[] optiweave::__ow_subscript_impl(A, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 132, __FUNCTION__);
        delete[] optiweave::__ow_subscript_impl(B, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 133, __FUNCTION__);
        delete[] optiweave::__ow_subscript_impl(C, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/comprehensive_test.cpp", 134, __FUNCTION__);
    }
    delete[] A;
    delete[] B;
    delete[] C;

    auto end_total = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_total - start_total);

    std::cout << "\nTotal execution time: " << duration.count() << " ms\n";
    std::cout << "All tests completed successfully!\n";

    return 0;
}
