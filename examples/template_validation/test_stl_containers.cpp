/*
 * Comprehensive STL Container Template Test
 * Tests OptiWeave with various C++ standard library containers
 */

#include <iostream>
#include <vector>
#include <array>
#include <deque>
#include <string>
#include <algorithm>

// Test 1: std::vector<T>
template <typename T>
T vector_sum(const std::vector<T>& vec) {
    T sum = T{};
    for (size_t i = 0; i < vec.size(); i++) {
        sum += vec[i];
    }
    return sum;
}

// Test 2: std::array<T, N>
template <typename T, size_t N>
T array_sum(const std::array<T, N>& arr) {
    T sum = T{};
    for (size_t i = 0; i < N; i++) {
        sum += arr[i];
    }
    return sum;
}

// Test 3: std::deque<T>
template <typename T>
T deque_sum(const std::deque<T>& dq) {
    T sum = T{};
    for (size_t i = 0; i < dq.size(); i++) {
        sum += dq[i];
    }
    return sum;
}

// Test 4: std::string (uses operator[])
size_t count_vowels(const std::string& str) {
    size_t count = 0;
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
            c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U') {
            count++;
        }
    }
    return count;
}

// Test 5: Nested templates
template <typename T>
void nested_vector_access(std::vector<std::vector<T>>& matrix) {
    for (size_t i = 0; i < matrix.size(); i++) {
        for (size_t j = 0; j < matrix[i].size(); j++) {
            matrix[i][j] = static_cast<T>(i * 10 + j);
        }
    }
}

// Test 6: Template with multiple containers
template <typename T>
struct ContainerPair {
    std::vector<T> vec;
    std::array<T, 5> arr;

    T get_total() {
        T sum = T{};
        for (size_t i = 0; i < vec.size(); i++) {
            sum += vec[i];
        }
        for (size_t i = 0; i < 5; i++) {
            sum += arr[i];
        }
        return sum;
    }
};

int main() {
    std::cout << "STL Container Template Validation\n";
    std::cout << "==================================\n\n";

    // Test 1: std::vector
    std::cout << "Test 1: std::vector<int>\n";
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int vec_sum_result = vector_sum(vec);
    std::cout << "  Sum: " << vec_sum_result << " (expected: 55)\n";
    std::cout << "  " << (vec_sum_result == 55 ? "✓ PASS" : "✗ FAIL") << "\n\n";

    // Test 2: std::array
    std::cout << "Test 2: std::array<double, 5>\n";
    std::array<double, 5> arr = {1.5, 2.5, 3.5, 4.5, 5.5};
    double arr_sum_result = array_sum(arr);
    std::cout << "  Sum: " << arr_sum_result << " (expected: 17.5)\n";
    std::cout << "  " << (arr_sum_result == 17.5 ? "✓ PASS" : "✗ FAIL") << "\n\n";

    // Test 3: std::deque
    std::cout << "Test 3: std::deque<int>\n";
    std::deque<int> dq = {10, 20, 30, 40, 50};
    int dq_sum_result = deque_sum(dq);
    std::cout << "  Sum: " << dq_sum_result << " (expected: 150)\n";
    std::cout << "  " << (dq_sum_result == 150 ? "✓ PASS" : "✗ FAIL") << "\n\n";

    // Test 4: std::string
    std::cout << "Test 4: std::string vowel counting\n";
    std::string str = "Hello World! This is a template test.";
    size_t vowel_count = count_vowels(str);
    std::cout << "  Vowels: " << vowel_count << " (expected: 11)\n";
    std::cout << "  " << (vowel_count == 11 ? "✓ PASS" : "✗ FAIL") << "\n\n";

    // Test 5: Nested vectors
    std::cout << "Test 5: std::vector<std::vector<int>>\n";
    std::vector<std::vector<int>> matrix(3, std::vector<int>(4));
    nested_vector_access(matrix);
    bool nested_pass = (matrix[0][0] == 0 && matrix[1][2] == 12 && matrix[2][3] == 23);
    std::cout << "  matrix[1][2]: " << matrix[1][2] << " (expected: 12)\n";
    std::cout << "  " << (nested_pass ? "✓ PASS" : "✗ FAIL") << "\n\n";

    // Test 6: Template with multiple container types
    std::cout << "Test 6: Template struct with mixed containers\n";
    ContainerPair<int> pair;
    pair.vec = {1, 2, 3};
    pair.arr = {10, 20, 30, 40, 50};
    int total = pair.get_total();
    std::cout << "  Total: " << total << " (expected: 156)\n";
    std::cout << "  " << (total == 156 ? "✓ PASS" : "✗ FAIL") << "\n\n";

    // Test 7: const containers
    std::cout << "Test 7: const std::vector<int>\n";
    const std::vector<int> const_vec = {5, 10, 15, 20};
    int const_sum = 0;
    for (size_t i = 0; i < const_vec.size(); i++) {
        const_sum += const_vec[i];
    }
    std::cout << "  Sum: " << const_sum << " (expected: 50)\n";
    std::cout << "  " << (const_sum == 50 ? "✓ PASS" : "✗ FAIL") << "\n\n";

    std::cout << "==================================\n";
    std::cout << "All tests completed!\n";

    return 0;
}
