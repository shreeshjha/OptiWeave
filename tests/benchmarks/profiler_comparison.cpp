/**
 * OptiWeave Profiler Comparison Benchmark
 * 
 * Compares runtime overhead of:
 * 1. Baseline (no profiling)
 * 2. OptiWeave (thread-local counters)
 * 3. perf stat
 * 4. perf record
 * 5. gprof (-pg compiled)
 * 6. valgrind --tool=callgrind
 * 7. valgrind --tool=cachegrind
 * 
 * This file is compiled multiple times with different flags to test each scenario.
 */

#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <numeric>

// Configuration
constexpr size_t ARRAY_SIZE = 10000;
constexpr size_t ITERATIONS = 5000;
constexpr size_t WARMUP_RUNS = 3;
constexpr size_t MEASURED_RUNS = 10;

// Prevent compiler from optimizing away results
volatile double g_sink = 0;

// Simulate realistic workload: matrix-like operations with array accesses
class Workload {
public:
    std::vector<double> data;
    std::vector<double> weights;
    std::vector<double> results;
    
    Workload(size_t size) : data(size), weights(size), results(size) {
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        
        for (size_t i = 0; i < size; ++i) {
            data[i] = dist(rng);
            weights[i] = dist(rng);
            results[i] = 0.0;
        }
    }
    
    // Simulate array-heavy computation (what OptiWeave instruments)
    void compute_weighted_sum() {
        for (size_t i = 0; i < data.size(); ++i) {
            results[i] = data[i] * weights[i];
        }
    }
    
    // Simulate nested loop access patterns
    void compute_convolution_like() {
        const size_t window = 5;
        for (size_t i = window; i < data.size() - window; ++i) {
            double sum = 0.0;
            for (size_t j = 0; j < window; ++j) {
                sum += data[i - j] * weights[j];
                sum += data[i + j] * weights[j];
            }
            results[i] = sum / (2.0 * window);
        }
    }
    
    // Simulate random access patterns
    void compute_random_access(const std::vector<size_t>& indices) {
        for (size_t idx : indices) {
            if (idx < data.size()) {
                results[idx] = data[idx] * weights[idx] + 1.0;
            }
        }
    }
    
    // Simulate sorting (many comparisons and swaps)
    void sort_data() {
        std::sort(data.begin(), data.end());
    }
    
    // Accumulate results to prevent optimization
    double accumulate() const {
        return std::accumulate(results.begin(), results.end(), 0.0);
    }
};

// Generate random indices for random access test
std::vector<size_t> generate_random_indices(size_t count, size_t max_idx) {
    std::vector<size_t> indices(count);
    std::mt19937 rng(123);
    std::uniform_int_distribution<size_t> dist(0, max_idx - 1);
    
    for (size_t i = 0; i < count; ++i) {
        indices[i] = dist(rng);
    }
    return indices;
}

// Run the full workload
double run_workload(size_t iterations) {
    Workload workload(ARRAY_SIZE);
    auto random_indices = generate_random_indices(ARRAY_SIZE, ARRAY_SIZE);
    
    double total = 0.0;
    
    for (size_t iter = 0; iter < iterations; ++iter) {
        workload.compute_weighted_sum();
        workload.compute_convolution_like();
        workload.compute_random_access(random_indices);
        
        // Occasionally sort to add variety
        if (iter % 100 == 0) {
            workload.sort_data();
        }
        
        total += workload.accumulate();
    }
    
    return total;
}

// Measure execution time
double measure_time_ms(size_t iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    double result = run_workload(iterations);
    g_sink = result;  // Prevent optimization
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    return duration.count() / 1000.0;  // Return milliseconds
}

int main(int argc, char* argv[]) {
    // Check for "quick" mode (for valgrind which is slow)
    size_t iterations = ITERATIONS;
    size_t warmup = WARMUP_RUNS;
    size_t runs = MEASURED_RUNS;
    
    bool quick_mode = false;
    bool json_output = false;
    
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--quick") == 0) {
            quick_mode = true;
            iterations = 100;  // Reduced for slow tools
            warmup = 1;
            runs = 3;
        } else if (strcmp(argv[i], "--json") == 0) {
            json_output = true;
        }
    }
    
    if (!json_output) {
        std::cout << "OptiWeave Profiler Comparison Benchmark\n";
        std::cout << "========================================\n\n";
        std::cout << "Configuration:\n";
        std::cout << "  Array size:    " << ARRAY_SIZE << "\n";
        std::cout << "  Iterations:    " << iterations << "\n";
        std::cout << "  Warmup runs:   " << warmup << "\n";
        std::cout << "  Measured runs: " << runs << "\n";
        std::cout << "\nRunning benchmark...\n\n";
    }
    
    // Warmup
    for (size_t i = 0; i < warmup; ++i) {
        measure_time_ms(iterations / 10);
    }
    
    // Measured runs
    std::vector<double> times;
    times.reserve(runs);
    
    for (size_t i = 0; i < runs; ++i) {
        times.push_back(measure_time_ms(iterations));
    }
    
    // Calculate statistics
    std::sort(times.begin(), times.end());
    double min_time = times.front();
    double max_time = times.back();
    double avg_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    double median_time = times[times.size() / 2];
    
    if (json_output) {
        // JSON output for script parsing
        std::cout << "{\"min\":" << min_time 
                  << ",\"max\":" << max_time 
                  << ",\"avg\":" << avg_time 
                  << ",\"median\":" << median_time 
                  << ",\"iterations\":" << iterations
                  << "}\n";
    } else {
        std::cout << "Results:\n";
        std::cout << "  Min:    " << min_time << " ms\n";
        std::cout << "  Max:    " << max_time << " ms\n";
        std::cout << "  Avg:    " << avg_time << " ms\n";
        std::cout << "  Median: " << median_time << " ms\n";
    }
    
    return 0;
}
