#include <optiweave/prelude.hpp>
#include <iostream>
#include <chrono>

// Test 1: Simple template class with array member
template<typename T, size_t N>
class FixedArray {
private:
    T data[N];

public:
    FixedArray() {
        for (size_t i = 0; i < N; i++) {
            (__optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 13, __FUNCTION__), data[i] = T{});
        }
    }

    T& operator[](size_t idx) {
        return optiweave::__ow_subscript_impl(data, idx, "/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 18, __FUNCTION__);
    }

    const T& operator[](size_t idx) const {
        return optiweave::__ow_subscript_impl(data, idx, "/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 22, __FUNCTION__);
    }

    size_t size() const { return N; }

    // Method that uses array subscripts internally
    T sum() const {
        T result = T{};
        for (size_t i = 0; i < N; i++) {
            result += optiweave::__ow_subscript_impl(data, i, "/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 31, __FUNCTION__);
        }
        return result;
    }
};

// Test 2: Template function with C-style array
template<typename T, size_t N>
T array_sum(const T (&arr)[N]) {
    T sum = T{};
    for (size_t i = 0; i < N; i++) {
        sum += optiweave::__ow_subscript_impl(arr, i, "/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 42, __FUNCTION__);
    }
    return sum;
}

// Test 3: Template function with pointer and size
template<typename T>
T pointer_sum(const T* arr, size_t size) {
    T sum = T{};
    for (size_t i = 0; i < size; i++) {
        sum += optiweave::__ow_subscript_impl(arr, i, "/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 52, __FUNCTION__);
    }
    return sum;
}

// Test 4: Non-template function with raw arrays
void test_raw_arrays() {
    int arr[100];
    double darr[100];

    // Initialize
    for (int i = 0; i < 100; i++) {
        (__optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 64, __FUNCTION__), arr[i] = i);
        (__optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 65, __FUNCTION__), darr[i] = i * 0.5);
    }

    // Sum
    int isum = 0;
    double dsum = 0.0;

    for (int i = 0; i < 100; i++) {
        isum += optiweave::__ow_subscript_impl(arr, i, "/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 73, __FUNCTION__);
        dsum += optiweave::__ow_subscript_impl(darr, i, "/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 74, __FUNCTION__);
    }

    std::cout << "Raw array int sum: " << isum << std::endl;
    std::cout << "Raw array double sum: " << dsum << std::endl;
}

// Test 5: Benchmark with raw arrays
void benchmark_raw_arrays(int iterations) {
    double data[10000];

    // Initialize
    for (int i = 0; i < 10000; i++) {
        (__optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 87, __FUNCTION__), data[i] = static_cast<double>(i) / 100.0);
    }

    auto start = std::chrono::high_resolution_clock::now();

    double result = 0.0;
    for (int iter = 0; iter < iterations; iter++) {
        // Read all elements
        for (int i = 0; i < 10000; i++) {
            result += optiweave::__ow_subscript_impl(data, i, "/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 96, __FUNCTION__);
        }

        // Write all elements
        for (int i = 0; i < 10000; i++) {
            (__optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/template_validation/test_templates_simple.cpp", 101, __FUNCTION__), data[i] = data[i] * 1.001);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Benchmark (" << iterations << " iterations):" << std::endl;
    std::cout << "  Total time: " << duration.count() / 1000.0 << " ms" << std::endl;
    std::cout << "  Time per iteration: " << duration.count() / iterations << " us" << std::endl;
    std::cout << "  Result: " << result << std::endl;
}

int main() {
    std::cout << "=== C++ Template Tests (Raw Arrays) ===" << std::endl;
    std::cout << std::endl;

    // Test raw arrays
    test_raw_arrays();

    // Test C-style array template
    int arr1[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    double arr2[5] = {1.1, 2.2, 3.3, 4.4, 5.5};

    std::cout << "Template array_sum (int): " << array_sum(arr1) << std::endl;
    std::cout << "Template array_sum (double): " << array_sum(arr2) << std::endl;

    // Test pointer-based template
    std::cout << "Template pointer_sum (int): " << pointer_sum(arr1, 10) << std::endl;
    std::cout << "Template pointer_sum (double): " << pointer_sum(arr2, 5) << std::endl;

    // Test template class
    FixedArray<int, 100> fixed_arr;
    for (size_t i = 0; i < fixed_arr.size(); i++) {
        fixed_arr[i] = i * 2;
    }
    std::cout << "FixedArray sum: " << fixed_arr.sum() << std::endl;

    std::cout << std::endl;

    // Benchmark
    benchmark_raw_arrays(1000);

    return 0;
}
