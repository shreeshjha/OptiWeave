#include <optiweave/prelude.hpp>
// OptiWeave Overhead Benchmark
// Measures the performance overhead of instrumentation

#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include <cmath>

// Configuration
const int WARMUP_ITERATIONS = 3;
const int BENCHMARK_ITERATIONS = 10;
const int INNER_ITERATIONS = 1000000;

struct BenchmarkResult {
    std::string name;
    double min_ms;
    double max_ms;
    double avg_ms;
    double median_ms;
    double stddev_ms;

    double overhead_pct(const BenchmarkResult& baseline) const {
        return optiweave::ow_mul((optiweave::ow_div((optiweave::ow_sub(avg_ms, baseline.avg_ms)), baseline.avg_ms)), 100.0);
    }
};

class Timer {
    std::chrono::high_resolution_clock::time_point start_;
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }

    void reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }
};

BenchmarkResult run_benchmark(const std::string& name, auto&& func) {
    std::vector<double> times;
    times.reserve(BENCHMARK_ITERATIONS);

    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        func();
    }

    // Actual benchmark
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        Timer timer;
        func();
        times.push_back(timer.elapsed_ms());
    }

    // Calculate statistics
    std::sort(times.begin(), times.end());

    double sum = 0.0;
    for (double t : times) sum += t;
    double avg = optiweave::ow_div(sum, times.size());

    double variance = 0.0;
    for (double t : times) {
        double diff = optiweave::ow_sub(t, avg);
        variance += optiweave::ow_mul(diff, diff);
    }
    double stddev = std::sqrt(optiweave::ow_div(variance, times.size()));

    return BenchmarkResult{
        name,
        times.front(),
        times.back(),
        avg,
        times[optiweave::ow_div(times.size(), 2)],
        stddev
    };
}

// Benchmark 1: Array Access
void benchmark_array_baseline() {
    int arr[100];
    for (int i = 0; i < 100; i++) optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/benchmarks/benchmark_overhead_original.cpp", 86, __FUNCTION__) = i;

    volatile int sum = 0;
    for (int iter = 0; iter < INNER_ITERATIONS; iter++) {
        for (int i = 0; i < 100; i++) {
            sum += optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/benchmarks/benchmark_overhead_original.cpp", 91, __FUNCTION__);
        }
    }
}

void benchmark_array_instrumented() {
    int arr[100];
    for (int i = 0; i < 100; i++) optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/benchmarks/benchmark_overhead_original.cpp", 98, __FUNCTION__) = i;

    volatile int sum = 0;
    for (int iter = 0; iter < INNER_ITERATIONS; iter++) {
        for (int i = 0; i < 100; i++) {
            sum += optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/benchmarks/benchmark_overhead_original.cpp", 103, __FUNCTION__);  // This will be instrumented
        }
    }
}

// Benchmark 2: Arithmetic Operations
void benchmark_arithmetic_baseline() {
    volatile int result = 0;
    for (int i = 0; i < INNER_ITERATIONS; i++) {
        result = optiweave::ow_sub(optiweave::ow_mul((optiweave::ow_add(result, i)), 2), 1);
    }
}

void benchmark_arithmetic_instrumented() {
    volatile int result = 0;
    for (int i = 0; i < INNER_ITERATIONS; i++) {
        result = optiweave::ow_sub(optiweave::ow_mul((optiweave::ow_add(result, i)), 2), 1);  // Will be instrumented
    }
}

// Benchmark 3: Mixed Operations
void benchmark_mixed_baseline() {
    int arr[50];
    for (int i = 0; i < 50; i++) optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/benchmarks/benchmark_overhead_original.cpp", 126, __FUNCTION__) = i;

    volatile int result = 0;
    for (int iter = 0; iter < optiweave::ow_div(INNER_ITERATIONS, 10); iter++) {
        for (int i = 0; i < 50; i++) {
            result = optiweave::ow_add(result, optiweave::ow_mul(optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/benchmarks/benchmark_overhead_original.cpp", 131, __FUNCTION__), 2));
        }
    }
}

void benchmark_mixed_instrumented() {
    int arr[50];
    for (int i = 0; i < 50; i++) optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/benchmarks/benchmark_overhead_original.cpp", 138, __FUNCTION__) = i;

    volatile int result = 0;
    for (int iter = 0; iter < optiweave::ow_div(INNER_ITERATIONS, 10); iter++) {
        for (int i = 0; i < 50; i++) {
            result = optiweave::ow_add(result, optiweave::ow_mul(optiweave::__ow_subscript_impl(arr, i, "/Users/shreeshjha/Dev/Github/OptiWeave/tests/benchmarks/benchmark_overhead_original.cpp", 143, __FUNCTION__), 2));  // Will be instrumented
        }
    }
}

// Benchmark 4: Function Calls
int compute_baseline(int a, int b, int c) {
    return optiweave::ow_sub(optiweave::ow_mul((optiweave::ow_add(a, b)), c), (optiweave::ow_div(a, (optiweave::ow_add(b, 1)))));
}

int compute_instrumented(int a, int b, int c) {
    return optiweave::ow_sub(optiweave::ow_mul((optiweave::ow_add(a, b)), c), (optiweave::ow_div(a, (optiweave::ow_add(b, 1)))));  // Will be instrumented
}

void benchmark_function_baseline() {
    volatile int result = 0;
    for (int i = 0; i < INNER_ITERATIONS; i++) {
        result = compute_baseline(i, optiweave::ow_add(i, 1), optiweave::ow_add(i, 2));
    }
}

void benchmark_function_instrumented() {
    volatile int result = 0;
    for (int i = 0; i < INNER_ITERATIONS; i++) {
        result = compute_instrumented(i, optiweave::ow_add(i, 1), optiweave::ow_add(i, 2));
    }
}

void print_results(const std::vector<std::pair<BenchmarkResult, BenchmarkResult>>& results) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    OptiWeave Overhead Benchmark Results                       ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Configuration:\n";
    std::cout << "  Warmup iterations: " << WARMUP_ITERATIONS << "\n";
    std::cout << "  Benchmark iterations: " << BENCHMARK_ITERATIONS << "\n";
    std::cout << "  Inner loop iterations: " << INNER_ITERATIONS << "\n\n";

    std::cout << std::fixed << std::setprecision(3);

    // Detailed table
    std::cout << "┌────────────────────────────┬──────────┬──────────┬──────────┬──────────┬──────────┐\n";
    std::cout << "│ Benchmark                  │ Min (ms) │ Max (ms) │ Avg (ms) │ Median   │ Overhead │\n";
    std::cout << "├────────────────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┤\n";

    for (const auto& [baseline, instrumented] : results) {
        // Baseline
        std::cout << "│ " << std::left << std::setw(26) << baseline.name
                  << " │ " << std::right << std::setw(8) << baseline.min_ms
                  << " │ " << std::setw(8) << baseline.max_ms
                  << " │ " << std::setw(8) << baseline.avg_ms
                  << " │ " << std::setw(8) << baseline.median_ms
                  << " │ " << std::setw(8) << "-" << " │\n";

        // Instrumented
        double overhead = instrumented.overhead_pct(baseline);
        std::cout << "│ " << std::left << std::setw(26) << instrumented.name
                  << " │ " << std::right << std::setw(8) << instrumented.min_ms
                  << " │ " << std::setw(8) << instrumented.max_ms
                  << " │ " << std::setw(8) << instrumented.avg_ms
                  << " │ " << std::setw(8) << instrumented.median_ms
                  << " │ " << std::setw(7) << overhead << "% │\n";

        std::cout << "├────────────────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┤\n";
    }
    std::cout << "└────────────────────────────┴──────────┴──────────┴──────────┴──────────┴──────────┘\n\n";

    // Summary
    std::cout << "Summary:\n";
    double total_overhead = 0.0;
    for (const auto& [baseline, instrumented] : results) {
        double overhead = instrumented.overhead_pct(baseline);
        total_overhead += overhead;

        std::cout << "  " << baseline.name << ": ";
        if (overhead < 1.0) {
            std::cout << "✓ Excellent (< 1% overhead)\n";
        } else if (overhead < 5.0) {
            std::cout << "✓ Good (< 5% overhead)\n";
        } else if (overhead < 10.0) {
            std::cout << "⚠ Acceptable (< 10% overhead)\n";
        } else {
            std::cout << "✗ High (> 10% overhead)\n";
        }
    }

    double avg_overhead = optiweave::ow_div(total_overhead, results.size());
    std::cout << "\nAverage overhead: " << avg_overhead << "%\n";

    if (avg_overhead < 1.0) {
        std::cout << "✓ EXCELLENT - Minimal performance impact\n";
    } else if (avg_overhead < 5.0) {
        std::cout << "✓ GOOD - Acceptable for production profiling\n";
    } else if (avg_overhead < 10.0) {
        std::cout << "⚠ ACCEPTABLE - Consider for development only\n";
    } else {
        std::cout << "✗ HIGH - Optimization needed\n";
    }

    std::cout << "\n";
}

int main() {
    std::cout << "OptiWeave Overhead Benchmark\n";
    std::cout << "==============================\n\n";

    std::cout << "NOTE: This binary measures BOTH baseline and instrumented functions.\n";
    std::cout << "Baseline functions are named *_baseline, instrumented are *_instrumented.\n\n";

    std::cout << "Running benchmarks...\n";

    std::vector<std::pair<BenchmarkResult, BenchmarkResult>> results;

    // Array benchmark
    std::cout << "  [1/4] Array access benchmark...\n";
    auto array_baseline = run_benchmark("Array (baseline)", benchmark_array_baseline);
    auto array_instrumented = run_benchmark("Array (instrumented)", benchmark_array_instrumented);
    results.push_back({array_baseline, array_instrumented});

    // Arithmetic benchmark
    std::cout << "  [2/4] Arithmetic operations benchmark...\n";
    auto arith_baseline = run_benchmark("Arithmetic (baseline)", benchmark_arithmetic_baseline);
    auto arith_instrumented = run_benchmark("Arithmetic (instrumented)", benchmark_arithmetic_instrumented);
    results.push_back({arith_baseline, arith_instrumented});

    // Mixed benchmark
    std::cout << "  [3/4] Mixed operations benchmark...\n";
    auto mixed_baseline = run_benchmark("Mixed (baseline)", benchmark_mixed_baseline);
    auto mixed_instrumented = run_benchmark("Mixed (instrumented)", benchmark_mixed_instrumented);
    results.push_back({mixed_baseline, mixed_instrumented});

    // Function benchmark
    std::cout << "  [4/4] Function calls benchmark...\n";
    auto func_baseline = run_benchmark("Function (baseline)", benchmark_function_baseline);
    auto func_instrumented = run_benchmark("Function (instrumented)", benchmark_function_instrumented);
    results.push_back({func_baseline, func_instrumented});

    print_results(results);

    return 0;
}
