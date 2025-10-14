#include <optiweave/prelude.hpp>
/**
 * @file test_memory_profiling.cpp
 * @brief Test memory profiling feature for OptiWeave
 *
 * This file demonstrates various memory allocation patterns that can be detected
 * by OptiWeave's memory profiler:
 * - new/new[] allocations
 * - delete/delete[] deallocations
 * - Potential memory leaks (allocations without deallocations)
 * - Balanced allocations and deallocations
 */

#include <iostream>
#include <vector>

// Function with balanced memory usage
void balanced_memory() {
    // Single object allocation
    int* ptr = new int(42);
    std::cout << "Allocated single int: " << *ptr << std::endl;
    delete ptr;

    // Array allocation
    double* arr = new double[10];
    for (int i = 0; i < 10; i++) {
        optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/build/../examples/test_memory_profiling.cpp", 26, __FUNCTION__) = i * 1.5;
    }
    delete[] arr;
}

// Function with potential memory leak
void potential_leak() {
    // Allocation without deallocation
    int* leaked = new int(100);
    std::cout << "Created potentially leaked memory: " << *leaked << std::endl;
    // Missing: delete leaked;
}

// Function with multiple allocations
void multiple_allocations() {
    // Multiple allocations
    int* a = new int(1);
    int* b = new int(2);
    int* c = new int(3);

    std::cout << "Sum: " << (*a + *b + *c) << std::endl;

    // Only deallocate some
    delete a;
    delete b;
    // Leak: c is not deleted
}

// Class with dynamic memory
class DynamicArray {
public:
    DynamicArray(size_t size) : size_(size) {
        data_ = new double[size];
        for (size_t i = 0; i < size; i++) {
            optiweave::__ow_subscript_impl(data_, i, "/Users/shreeshjha/Dev/Github/OptiWeave/build/../examples/test_memory_profiling.cpp", 60, __FUNCTION__) = 0.0;
        }
    }

    ~DynamicArray() {
        delete[] data_;
    }

    double& operator[](size_t index) {
        return optiweave::__ow_subscript_impl(data_, index, "/Users/shreeshjha/Dev/Github/OptiWeave/build/../examples/test_memory_profiling.cpp", 69, __FUNCTION__);
    }

private:
    double* data_;
    size_t size_;
};

// Function using RAII pattern (properly managed)
void raii_pattern() {
    DynamicArray arr(20);
    for (size_t i = 0; i < 20; i++) {
        arr[i] = i * 2.5;
    }
    std::cout << "RAII array[10]: " << arr[10] << std::endl;
    // No explicit delete needed - destructor handles it
}

// Function with mixed patterns
void mixed_patterns() {
    // STL containers (no tracking - they use internal allocators)
    std::vector<int> vec = {1, 2, 3, 4, 5};

    // Raw pointer allocation
    char* buffer = new char[256];
    optiweave::__ow_subscript_impl(buffer, 0, "/Users/shreeshjha/Dev/Github/OptiWeave/build/../examples/test_memory_profiling.cpp", 94, __FUNCTION__) = 'H';
    optiweave::__ow_subscript_impl(buffer, 1, "/Users/shreeshjha/Dev/Github/OptiWeave/build/../examples/test_memory_profiling.cpp", 95, __FUNCTION__) = 'i';
    optiweave::__ow_subscript_impl(buffer, 2, "/Users/shreeshjha/Dev/Github/OptiWeave/build/../examples/test_memory_profiling.cpp", 96, __FUNCTION__) = '\0';
    std::cout << "Buffer: " << buffer << std::endl;
    delete[] buffer;

    // Object allocation
    DynamicArray* dynamic = new DynamicArray(5);
    (*dynamic)[0] = 3.14;
    delete dynamic;
}

int main() {
    std::cout << "=== OptiWeave Memory Profiling Test ===" << std::endl;

    std::cout << "\n1. Testing balanced memory usage..." << std::endl;
    balanced_memory();

    std::cout << "\n2. Testing potential memory leak..." << std::endl;
    potential_leak();

    std::cout << "\n3. Testing multiple allocations..." << std::endl;
    multiple_allocations();

    std::cout << "\n4. Testing RAII pattern..." << std::endl;
    raii_pattern();

    std::cout << "\n5. Testing mixed patterns..." << std::endl;
    mixed_patterns();

    std::cout << "\n=== Test Complete ===" << std::endl;
    std::cout << "Expected memory profile:" << std::endl;
    std::cout << "  - Multiple new/new[] allocations" << std::endl;
    std::cout << "  - Several delete/delete[] deallocations" << std::endl;
    std::cout << "  - 2-3 potential memory leaks detected" << std::endl;

    return 0;
}
