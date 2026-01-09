#include <vector>
#include <iostream>
#include <chrono>
#include <numeric>

// Test 1: Basic std::vector subscript operations
void test_vector_basic() {
    std::vector<int> vec(1000);

    // Initialize
    for (size_t i = 0; i < vec.size(); i++) {
        vec[i] = i * 2;
    }

    // Read and sum
    int sum = 0;
    for (size_t i = 0; i < vec.size(); i++) {
        sum += vec[i];
    }

    std::cout << "Vector basic test sum: " << sum << std::endl;
}

// Test 2: Nested vector (2D array simulation)
void test_vector_2d() {
    std::vector<std::vector<int>> matrix(100, std::vector<int>(100));

    // Initialize 2D matrix
    for (size_t i = 0; i < matrix.size(); i++) {
        for (size_t j = 0; j < matrix[i].size(); j++) {
            matrix[i][j] = i * 100 + j;
        }
    }

    // Compute sum of diagonal
    int diagonal_sum = 0;
    for (size_t i = 0; i < matrix.size(); i++) {
        diagonal_sum += matrix[i][i];
    }

    std::cout << "2D vector diagonal sum: " << diagonal_sum << std::endl;
}

// Test 3: Template function with array subscripts
template<typename T>
T array_sum(const std::vector<T>& arr) {
    T sum = 0;
    for (size_t i = 0; i < arr.size(); i++) {
        sum += arr[i];
    }
    return sum;
}

// Test 4: Performance benchmark
void benchmark_vector_operations(int iterations) {
    std::vector<double> data(10000);

    // Initialize with some data
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<double>(i) / 100.0;
    }

    auto start = std::chrono::high_resolution_clock::now();

    double result = 0.0;
    for (int iter = 0; iter < iterations; iter++) {
        // Read all elements
        for (size_t i = 0; i < data.size(); i++) {
            result += data[i];
        }

        // Write all elements
        for (size_t i = 0; i < data.size(); i++) {
            data[i] = data[i] * 1.001;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Benchmark (" << iterations << " iterations):" << std::endl;
    std::cout << "  Total time: " << duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "  Time per iteration: " << duration.count() / iterations << " us" << std::endl;
    std::cout << "  Result (anti-optimization): " << result << std::endl;
}

int main() {
    std::cout << "=== C++ Template Validation Tests ===" << std::endl;
    std::cout << std::endl;

    // Run tests
    test_vector_basic();
    test_vector_2d();

    // Test template instantiation
    std::vector<int> int_vec = {1, 2, 3, 4, 5};
    std::vector<double> double_vec = {1.1, 2.2, 3.3, 4.4, 5.5};

    std::cout << "Template function int sum: " << array_sum(int_vec) << std::endl;
    std::cout << "Template function double sum: " << array_sum(double_vec) << std::endl;

    std::cout << std::endl;

    // Run benchmark
    benchmark_vector_operations(1000);

    return 0;
}
