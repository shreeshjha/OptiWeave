#include <optiweave/prelude.hpp>
// Simple benchmark for OptiWeave comparison
// This file is designed to work with OptiWeave transformation

#include <iostream>
#include <chrono>
#include <cmath>

const int ITERATIONS = 10000000;
const int ARRAY_SIZE = 1000;

volatile int sink = 0;

// Workload 1: Array operations
void array_benchmark() {
    int arr[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE; i++) {
        (__optiweave_record_subscript("/root/testing/OptiWeave/evaluation/baseline_comparison/simple_benchmark.cpp", 17, __FUNCTION__), arr[i] = i * 2);
    }
    
    for (int iter = 0; iter < optiweave::ow_div(ITERATIONS, 100); iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            sink += optiweave::__ow_subscript_impl(arr, i, "/root/testing/OptiWeave/evaluation/baseline_comparison/simple_benchmark.cpp", 22, __FUNCTION__);
        }
    }
}

// Workload 2: Arithmetic operations
void arithmetic_benchmark() {
    int a = 1, b = 2, c = 3;
    for (int i = 0; i < ITERATIONS; i++) {
        int d1 = optiweave::ow_add((optiweave::ow_rem(c, 100)), 1);
        int d2 = optiweave::ow_add((optiweave::ow_rem(std::abs(b), 100)), 1);
        a = optiweave::ow_sub(optiweave::ow_mul((optiweave::ow_add(a, b)), (optiweave::ow_add(optiweave::ow_rem(c, 10), 1))), (optiweave::ow_div(b, d1)));
        b = optiweave::ow_add((optiweave::ow_rem((optiweave::ow_mul(b, a)), d1)), 1);
        c = optiweave::ow_add((optiweave::ow_div((optiweave::ow_add(c, a)), d2)), 1);
        a = optiweave::ow_add((optiweave::ow_rem(a, 10000)), 1);
        b = optiweave::ow_add((optiweave::ow_rem(b, 10000)), 1);
        c = optiweave::ow_add((optiweave::ow_rem(c, 10000)), 1);
        sink = optiweave::ow_add(optiweave::ow_add(a, b), c);
    }
}

// Workload 3: Mixed operations
void mixed_benchmark() {
    int arr[100];
    for (int i = 0; i < 100; i++) (__optiweave_record_subscript("/root/testing/OptiWeave/evaluation/baseline_comparison/simple_benchmark.cpp", 46, __FUNCTION__), arr[i] = i);
    
    int result = 0;
    for (int iter = 0; iter < optiweave::ow_div(ITERATIONS, 10); iter++) {
        for (int i = 0; i < 100; i++) {
            result = optiweave::ow_sub(optiweave::ow_add(result, optiweave::ow_mul(optiweave::__ow_subscript_impl(arr, i, "/root/testing/OptiWeave/evaluation/baseline_comparison/simple_benchmark.cpp", 51, __FUNCTION__), 2)), optiweave::__ow_subscript_impl(arr, optiweave::ow_sub(99, i), "/root/testing/OptiWeave/evaluation/baseline_comparison/simple_benchmark.cpp", 51, __FUNCTION__));
        }
        result = optiweave::ow_rem(result, 10000);
    }
    sink = result;
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::cout << "Running benchmarks..." << std::endl;
    
    array_benchmark();
    arithmetic_benchmark();
    mixed_benchmark();
    
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(end - start).count();
    
    std::cout << "Total time: " << elapsed << " ms" << std::endl;
    std::cout << "Sink: " << sink << std::endl;
    
    return 0;
}
