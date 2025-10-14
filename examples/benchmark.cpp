#include <optiweave/prelude.hpp>
// Comprehensive benchmark for OptiWeave overhead measurement
#include <iostream>
#include <chrono>
#include <vector>
#include <cmath>

using namespace std;
using namespace std::chrono;

const int ITERATIONS = 10000000;
const int ARRAY_SIZE = 1000;

// Benchmark 1: Array access heavy workload
double benchmark_array_access() {
    int arr[ARRAY_SIZE];
    for (int i = 0; i < ARRAY_SIZE; i++) {
        optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/benchmark.cpp", 17, __FUNCTION__) = i;
    }

    auto start = high_resolution_clock::now();

    long long sum = 0;
    for (int iter = 0; iter < ITERATIONS / 1000; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            sum += optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/benchmark.cpp", 25, __FUNCTION__);
        }
    }

    auto end = high_resolution_clock::now();
    duration<double, milli> elapsed = end - start;

    // Prevent optimization
    if (sum == 0) cout << "";

    return elapsed.count();
}

// Benchmark 2: Arithmetic operations heavy workload
double benchmark_arithmetic() {
    auto start = high_resolution_clock::now();

    double result = 1.0;
    for (int i = 1; i < ITERATIONS / 10000; i++) {
        result = result * 1.0001 + 0.5 - 0.3;
    }

    auto end = high_resolution_clock::now();
    duration<double, milli> elapsed = end - start;

    // Prevent optimization
    if (result == 0) cout << "";

    return elapsed.count();
}

// Benchmark 3: Mixed operations (realistic workload)
double benchmark_mixed() {
    int arr[100];
    for (int i = 0; i < 100; i++) {
        optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/benchmark.cpp", 60, __FUNCTION__) = i;
    }

    auto start = high_resolution_clock::now();

    long long result = 0;
    for (int iter = 0; iter < ITERATIONS / 1000; iter++) {
        for (int i = 0; i < 99; i++) {
            result += optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/benchmark.cpp", 68, __FUNCTION__) * optiweave::__ow_subscript_impl(arr, i + 1, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/benchmark.cpp", 68, __FUNCTION__);
        }
    }

    auto end = high_resolution_clock::now();
    duration<double, milli> elapsed = end - start;

    // Prevent optimization
    if (result == 0) cout << "";

    return elapsed.count();
}

// Benchmark 4: Function call overhead
int compute(int a, int b) {
    return a * b + a - b;
}

double benchmark_function_calls() {
    auto start = high_resolution_clock::now();

    int result = 0;
    for (int i = 0; i < ITERATIONS / 10000; i++) {
        result += compute(i, i + 1);
    }

    auto end = high_resolution_clock::now();
    duration<double, milli> elapsed = end - start;

    // Prevent optimization
    if (result == 0) cout << "";

    return elapsed.count();
}

// Run each benchmark multiple times and take average
double run_benchmark(const char* name, double (*benchmark_fn)()) {
    const int RUNS = 5;
    double total = 0.0;

    cout << "Running " << name << "..." << endl;

    for (int i = 0; i < RUNS; i++) {
        double time = benchmark_fn();
        cout << "  Run " << (i + 1) << ": " << time << " ms" << endl;
        total += time;
    }

    double avg = total / RUNS;
    cout << "  Average: " << avg << " ms" << endl << endl;

    return avg;
}

int main() {
    cout << "OptiWeave Performance Benchmark" << endl;
    cout << "================================" << endl << endl;

    double array_time = run_benchmark("Array Access", benchmark_array_access);
    double arith_time = run_benchmark("Arithmetic Ops", benchmark_arithmetic);
    double mixed_time = run_benchmark("Mixed Operations", benchmark_mixed);
    double func_time = run_benchmark("Function Calls", benchmark_function_calls);

    cout << "Summary:" << endl;
    cout << "--------" << endl;
    cout << "Array Access:     " << array_time << " ms" << endl;
    cout << "Arithmetic Ops:   " << arith_time << " ms" << endl;
    cout << "Mixed Operations: " << mixed_time << " ms" << endl;
    cout << "Function Calls:   " << func_time << " ms" << endl;

    return 0;
}
