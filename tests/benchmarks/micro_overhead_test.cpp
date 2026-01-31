// OptiWeave Micro-Overhead Test
// Measures the actual overhead of our optimized instrumentation functions
// This test directly includes the prelude and measures the overhead

#include <iostream>
#include <chrono>
#include <vector>
#include <cmath>
#include <iomanip>

// Enable stats for testing (must be before prelude include)
#define OPTIWEAVE_ENABLE_STATS 1

// Include the optimized prelude
#include "prelude.hpp"

constexpr int WARMUP = 5;
constexpr int ITERATIONS = 20;
constexpr int ARRAY_SIZE = 1000;
constexpr int LOOP_COUNT = 100000;

using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;

struct BenchResult {
    double min_ms = 1e9;
    double max_ms = 0;
    double sum_ms = 0;
    int count = 0;
    
    void record(double ms) {
        min_ms = std::min(min_ms, ms);
        max_ms = std::max(max_ms, ms);
        sum_ms += ms;
        count++;
    }
    
    double avg() const { return sum_ms / count; }
    double overhead(const BenchResult& baseline) const {
        return (avg() - baseline.avg()) / baseline.avg() * 100.0;
    }
};

// Prevent compiler from optimizing away
volatile int g_sink = 0;

// ============================================================================
// BASELINE FUNCTIONS (no instrumentation)
// ============================================================================

void array_access_baseline(int* arr, int size) {
    int sum = 0;
    for (int iter = 0; iter < LOOP_COUNT; iter++) {
        for (int i = 0; i < size; i++) {
            sum += arr[i];
        }
    }
    g_sink = sum;
}

void arithmetic_baseline(int n) {
    int result = 0;
    for (int i = 0; i < n * LOOP_COUNT; i++) {
        result = (result + i) * 2 - 1;
    }
    g_sink = result;
}

void mixed_baseline(int* arr, int size) {
    int result = 0;
    for (int iter = 0; iter < LOOP_COUNT / 10; iter++) {
        for (int i = 0; i < size; i++) {
            result = result + arr[i] * 2;
        }
    }
    g_sink = result;
}

// ============================================================================
// INSTRUMENTED FUNCTIONS (using OptiWeave templates)
// ============================================================================

void array_access_instrumented(int* arr, int size) {
    int sum = 0;
    for (int iter = 0; iter < LOOP_COUNT; iter++) {
        for (int i = 0; i < size; i++) {
            // Using the OptiWeave subscript macro (which calls __ow_subscript_impl)
            sum += ow_subscript(arr, i);
        }
    }
    g_sink = sum;
}

void arithmetic_instrumented(int n) {
    int result = 0;
    for (int i = 0; i < n * LOOP_COUNT; i++) {
        // Using OptiWeave primop templates for arithmetic
        result = optiweave::ow_sub(optiweave::ow_mul(optiweave::ow_add(result, i), 2), 1);
    }
    g_sink = result;
}

void mixed_instrumented(int* arr, int size) {
    int result = 0;
    for (int iter = 0; iter < LOOP_COUNT / 10; iter++) {
        for (int i = 0; i < size; i++) {
            result = optiweave::ow_add(result, optiweave::ow_mul(ow_subscript(arr, i), 2));
        }
    }
    g_sink = result;
}

// ============================================================================
// BENCHMARK RUNNER
// ============================================================================

template<typename Func>
BenchResult run_bench(const char* name, Func&& func) {
    BenchResult result;
    
    // Warmup
    for (int i = 0; i < WARMUP; i++) {
        func();
    }
    
    // Measured runs
    for (int i = 0; i < ITERATIONS; i++) {
        auto start = Clock::now();
        func();
        auto end = Clock::now();
        result.record(Duration(end - start).count());
    }
    
    return result;
}

void print_separator() {
    std::cout << "├────────────────────────────┼──────────┼──────────┼──────────┼──────────┤\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║      OptiWeave Optimized Instrumentation Overhead Test                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Configuration:\n";
    std::cout << "  Array size:      " << ARRAY_SIZE << "\n";
    std::cout << "  Loop iterations: " << LOOP_COUNT << "\n";
    std::cout << "  Warmup runs:     " << WARMUP << "\n";
    std::cout << "  Measured runs:   " << ITERATIONS << "\n\n";
    
    // Initialize test array
    std::vector<int> arr(ARRAY_SIZE);
    for (int i = 0; i < ARRAY_SIZE; i++) arr[i] = i;
    
    std::cout << "Running benchmarks...\n\n";
    
    // Run benchmarks
    auto array_base = run_bench("Array Baseline", [&]() { array_access_baseline(arr.data(), ARRAY_SIZE); });
    auto array_inst = run_bench("Array Instrumented", [&]() { array_access_instrumented(arr.data(), ARRAY_SIZE); });
    
    auto arith_base = run_bench("Arithmetic Baseline", [&]() { arithmetic_baseline(100); });
    auto arith_inst = run_bench("Arithmetic Instrumented", [&]() { arithmetic_instrumented(100); });
    
    auto mixed_base = run_bench("Mixed Baseline", [&]() { mixed_baseline(arr.data(), ARRAY_SIZE); });
    auto mixed_inst = run_bench("Mixed Instrumented", [&]() { mixed_instrumented(arr.data(), ARRAY_SIZE); });
    
    // Print results
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "┌────────────────────────────┬──────────┬──────────┬──────────┬──────────┐\n";
    std::cout << "│ Benchmark                  │ Min (ms) │ Avg (ms) │ Max (ms) │ Overhead │\n";
    std::cout << "├────────────────────────────┼──────────┼──────────┼──────────┼──────────┤\n";
    
    auto print_row = [](const char* name, const BenchResult& r, const BenchResult* baseline = nullptr) {
        std::cout << "│ " << std::left << std::setw(26) << name << " │ "
                  << std::right << std::setw(8) << r.min_ms << " │ "
                  << std::setw(8) << r.avg() << " │ "
                  << std::setw(8) << r.max_ms << " │ ";
        if (baseline) {
            std::cout << std::setw(7) << r.overhead(*baseline) << "% │\n";
        } else {
            std::cout << std::setw(8) << "-" << " │\n";
        }
    };
    
    print_row("Array (baseline)", array_base);
    print_row("Array (instrumented)", array_inst, &array_base);
    print_separator();
    print_row("Arithmetic (baseline)", arith_base);
    print_row("Arithmetic (instrumented)", arith_inst, &arith_base);
    print_separator();
    print_row("Mixed (baseline)", mixed_base);
    print_row("Mixed (instrumented)", mixed_inst, &mixed_base);
    std::cout << "└────────────────────────────┴──────────┴──────────┴──────────┴──────────┘\n\n";
    
    // Calculate average overhead
    double avg_overhead = (array_inst.overhead(array_base) + 
                          arith_inst.overhead(arith_base) + 
                          mixed_inst.overhead(mixed_base)) / 3.0;
    
    std::cout << "Average Overhead: " << avg_overhead << "%\n\n";
    
    // Evaluation
    if (avg_overhead < 5.0) {
        std::cout << "✅ EXCELLENT - Very low overhead (<5%)\n";
    } else if (avg_overhead < 10.0) {
        std::cout << "✅ GOOD - Low overhead (<10%)\n";
    } else if (avg_overhead < 17.0) {
        std::cout << "⚠️  ACCEPTABLE - Within target range (8-17%)\n";
    } else if (avg_overhead < 30.0) {
        std::cout << "⚠️  MODERATE - Above target but usable (<30%)\n";
    } else {
        std::cout << "❌ HIGH - Needs optimization (>" << avg_overhead << "%)\n";
    }
    
    std::cout << "\n";
    
    return 0;
}
