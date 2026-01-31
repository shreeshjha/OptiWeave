/**
 * OptiWeave Comprehensive Profiler Comparison Benchmark
 * 
 * This benchmark provides thorough comparison of profiling overhead across:
 * 1. Different workload types (compute-bound, memory-bound, mixed)
 * 2. Different operation patterns (sequential, random, strided)
 * 3. Different data sizes (small, medium, large)
 * 
 * Profilers compared:
 * - Baseline (no profiling)
 * - OptiWeave thread-local counters
 * - OptiWeave with runtime checks
 * - OptiWeave full instrumentation
 * - perf stat (hardware counters)
 * - perf record (sampling)
 * - gprof (-pg instrumentation)
 * - valgrind callgrind
 * - valgrind cachegrind
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <array>
#include <random>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <numeric>
#include <functional>
#include <string>

// ============================================================================
// Configuration
// ============================================================================

// Workload sizes
constexpr size_t SMALL_SIZE = 1000;
constexpr size_t MEDIUM_SIZE = 10000;
constexpr size_t LARGE_SIZE = 100000;

// Iteration counts (adjusted per workload to get meaningful timing)
constexpr size_t COMPUTE_ITERATIONS = 10000;
constexpr size_t MEMORY_ITERATIONS = 5000;
constexpr size_t MIXED_ITERATIONS = 5000;

// Benchmark configuration
constexpr size_t WARMUP_RUNS = 3;
constexpr size_t MEASURED_RUNS = 10;

// Prevent compiler optimization
volatile double g_sink = 0.0;
volatile size_t g_sink_int = 0;

// ============================================================================
// Workload Classes
// ============================================================================

/**
 * Compute-bound workload: Heavy arithmetic, minimal memory access
 * Tests overhead of arithmetic operation instrumentation
 */
class ComputeBoundWorkload {
public:
    std::vector<double> data;
    
    explicit ComputeBoundWorkload(size_t size) : data(size) {
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.1, 10.0);
        for (size_t i = 0; i < size; ++i) {
            data[i] = dist(rng);
        }
    }
    
    // Heavy computation: trigonometry, sqrt, divisions
    double run(size_t iterations) {
        double result = 0.0;
        const size_t n = data.size();
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            for (size_t i = 0; i < n; ++i) {
                double x = data[i];
                // Multiple arithmetic operations per element
                double y = x * x + 2.0 * x + 1.0;
                y = y / (x + 1.0);
                y = std::sqrt(y) * std::sin(x * 0.01);
                y = y + std::cos(x * 0.01) / (y + 0.1);
                result += y;
            }
        }
        return result;
    }
    
    static const char* name() { return "Compute-Bound"; }
};

/**
 * Memory-bound workload: Sequential array access
 * Tests overhead of array subscript instrumentation
 */
class MemorySequentialWorkload {
public:
    std::vector<double> src;
    std::vector<double> dst;
    
    explicit MemorySequentialWorkload(size_t size) : src(size), dst(size) {
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        for (size_t i = 0; i < size; ++i) {
            src[i] = dist(rng);
            dst[i] = 0.0;
        }
    }
    
    // Sequential memory access pattern
    double run(size_t iterations) {
        const size_t n = src.size();
        double checksum = 0.0;
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            for (size_t i = 0; i < n; ++i) {
                dst[i] = src[i] * 2.0 + 1.0;
            }
            for (size_t i = 0; i < n; ++i) {
                checksum += dst[i];
            }
        }
        return checksum;
    }
    
    static const char* name() { return "Memory-Sequential"; }
};

/**
 * Memory-bound workload: Random array access
 * Tests overhead with cache-unfriendly patterns
 */
class MemoryRandomWorkload {
public:
    std::vector<double> data;
    std::vector<size_t> indices;
    
    explicit MemoryRandomWorkload(size_t size) : data(size), indices(size) {
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        std::uniform_int_distribution<size_t> idx_dist(0, size - 1);
        
        for (size_t i = 0; i < size; ++i) {
            data[i] = dist(rng);
            indices[i] = idx_dist(rng);
        }
    }
    
    // Random memory access pattern (cache-unfriendly)
    double run(size_t iterations) {
        const size_t n = data.size();
        double result = 0.0;
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            for (size_t i = 0; i < n; ++i) {
                size_t idx = indices[i];
                result += data[idx];
                data[idx] *= 1.00001;  // Small modification
            }
        }
        return result;
    }
    
    static const char* name() { return "Memory-Random"; }
};

/**
 * Memory-bound workload: Strided array access
 * Tests overhead with predictable but non-sequential pattern
 */
class MemoryStridedWorkload {
public:
    std::vector<double> data;
    size_t stride;
    
    explicit MemoryStridedWorkload(size_t size, size_t stride = 64) 
        : data(size), stride(stride) {
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        for (size_t i = 0; i < size; ++i) {
            data[i] = dist(rng);
        }
    }
    
    // Strided memory access pattern
    double run(size_t iterations) {
        const size_t n = data.size();
        double result = 0.0;
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            for (size_t offset = 0; offset < stride; ++offset) {
                for (size_t i = offset; i < n; i += stride) {
                    result += data[i];
                    data[i] *= 1.00001;
                }
            }
        }
        return result;
    }
    
    static const char* name() { return "Memory-Strided"; }
};

/**
 * Mixed workload: Combination of compute and memory operations
 * Simulates real-world application patterns
 */
class MixedWorkload {
public:
    std::vector<double> matrix;
    std::vector<double> vector_in;
    std::vector<double> vector_out;
    size_t rows, cols;
    
    explicit MixedWorkload(size_t size) 
        : rows(static_cast<size_t>(std::sqrt(size))),
          cols(static_cast<size_t>(std::sqrt(size))) {
        matrix.resize(rows * cols);
        vector_in.resize(cols);
        vector_out.resize(rows);
        
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        
        for (auto& v : matrix) v = dist(rng);
        for (auto& v : vector_in) v = dist(rng);
    }
    
    // Matrix-vector multiplication
    double run(size_t iterations) {
        double checksum = 0.0;
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            for (size_t i = 0; i < rows; ++i) {
                double sum = 0.0;
                for (size_t j = 0; j < cols; ++j) {
                    sum += matrix[i * cols + j] * vector_in[j];
                }
                vector_out[i] = sum;
            }
            
            for (size_t i = 0; i < rows; ++i) {
                checksum += vector_out[i];
            }
        }
        return checksum;
    }
    
    static const char* name() { return "Mixed (MatVec)"; }
};

/**
 * Sorting workload: Many comparisons and swaps
 * Tests comparison operation instrumentation
 */
class SortingWorkload {
public:
    std::vector<double> data;
    std::vector<double> original;
    
    explicit SortingWorkload(size_t size) : data(size), original(size) {
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        for (size_t i = 0; i < size; ++i) {
            original[i] = dist(rng);
        }
    }
    
    // Sort and restore
    double run(size_t iterations) {
        double checksum = 0.0;
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            // Restore original data
            data = original;
            
            // Sort
            std::sort(data.begin(), data.end());
            
            // Verify (also adds to checksum)
            checksum += data.front() + data.back();
        }
        return checksum;
    }
    
    static const char* name() { return "Sorting"; }
};

/**
 * Nested loop workload: Stencil/convolution pattern
 * Tests multiple array accesses per iteration
 */
class StencilWorkload {
public:
    std::vector<double> input;
    std::vector<double> output;
    size_t width, height;
    
    explicit StencilWorkload(size_t size) 
        : width(static_cast<size_t>(std::sqrt(size))),
          height(static_cast<size_t>(std::sqrt(size))) {
        input.resize(width * height);
        output.resize(width * height);
        
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        for (auto& v : input) v = dist(rng);
    }
    
    // 5-point stencil
    double run(size_t iterations) {
        double checksum = 0.0;
        
        for (size_t iter = 0; iter < iterations; ++iter) {
            for (size_t y = 1; y < height - 1; ++y) {
                for (size_t x = 1; x < width - 1; ++x) {
                    size_t idx = y * width + x;
                    output[idx] = 0.25 * (
                        input[idx - width] +  // North
                        input[idx + width] +  // South
                        input[idx - 1] +      // West
                        input[idx + 1]        // East
                    );
                }
            }
            
            // Swap for next iteration
            std::swap(input, output);
            checksum += input[width * height / 2];
        }
        return checksum;
    }
    
    static const char* name() { return "Stencil (5-point)"; }
};

// ============================================================================
// Timing Infrastructure
// ============================================================================

struct BenchmarkResult {
    std::string name;
    double min_ms;
    double max_ms;
    double avg_ms;
    double median_ms;
    double stddev_ms;
    size_t operations;  // Approximate operation count
    
    double overhead_pct(const BenchmarkResult& baseline) const {
        return ((avg_ms - baseline.avg_ms) / baseline.avg_ms) * 100.0;
    }
};

template<typename Workload>
BenchmarkResult run_benchmark(const std::string& name, size_t size, size_t iterations) {
    Workload workload(size);
    
    // Warmup
    for (size_t i = 0; i < WARMUP_RUNS; ++i) {
        g_sink = workload.run(iterations / 10);
    }
    
    // Measured runs
    std::vector<double> times;
    times.reserve(MEASURED_RUNS);
    
    for (size_t i = 0; i < MEASURED_RUNS; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        g_sink = workload.run(iterations);
        auto end = std::chrono::high_resolution_clock::now();
        
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        times.push_back(ms);
    }
    
    // Calculate statistics
    std::sort(times.begin(), times.end());
    
    double sum = std::accumulate(times.begin(), times.end(), 0.0);
    double avg = sum / times.size();
    
    double sq_sum = 0.0;
    for (double t : times) {
        sq_sum += (t - avg) * (t - avg);
    }
    double stddev = std::sqrt(sq_sum / times.size());
    
    return BenchmarkResult{
        name,
        times.front(),
        times.back(),
        avg,
        times[times.size() / 2],
        stddev,
        size * iterations
    };
}

// ============================================================================
// Output Formatting
// ============================================================================

void print_header(const std::string& title) {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ " << std::left << std::setw(77) << title << " ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝\n";
}

void print_result_table_header() {
    std::cout << "┌─────────────────────────┬──────────┬──────────┬──────────┬──────────┬──────────┐\n";
    std::cout << "│ Workload                │ Min (ms) │ Avg (ms) │ Max (ms) │ StdDev   │ Overhead │\n";
    std::cout << "├─────────────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┤\n";
}

void print_result_row(const BenchmarkResult& result, const BenchmarkResult* baseline = nullptr) {
    std::cout << "│ " << std::left << std::setw(23) << result.name << " │ "
              << std::right << std::fixed << std::setprecision(2)
              << std::setw(8) << result.min_ms << " │ "
              << std::setw(8) << result.avg_ms << " │ "
              << std::setw(8) << result.max_ms << " │ "
              << std::setw(8) << result.stddev_ms << " │ ";
    
    if (baseline) {
        double overhead = result.overhead_pct(*baseline);
        if (overhead < -1.0) {
            std::cout << "\033[32m" << std::setw(7) << overhead << "%\033[0m │\n";
        } else if (overhead > 50.0) {
            std::cout << "\033[31m" << std::setw(7) << overhead << "%\033[0m │\n";
        } else if (overhead > 20.0) {
            std::cout << "\033[33m" << std::setw(7) << overhead << "%\033[0m │\n";
        } else {
            std::cout << std::setw(7) << overhead << "% │\n";
        }
    } else {
        std::cout << "      - │\n";
    }
}

void print_table_separator() {
    std::cout << "├─────────────────────────┼──────────┼──────────┼──────────┼──────────┼──────────┤\n";
}

void print_table_footer() {
    std::cout << "└─────────────────────────┴──────────┴──────────┴──────────┴──────────┴──────────┘\n";
}

// ============================================================================
// Main Benchmark Driver
// ============================================================================

int main(int argc, char* argv[]) {
    bool json_output = false;
    bool quick_mode = false;
    std::string workload_filter;
    
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--json") == 0) {
            json_output = true;
        } else if (strcmp(argv[i], "--quick") == 0) {
            quick_mode = true;
        } else if (strncmp(argv[i], "--workload=", 11) == 0) {
            workload_filter = argv[i] + 11;
        }
    }
    
    // Adjust iterations for quick mode
    size_t compute_iters = quick_mode ? COMPUTE_ITERATIONS / 10 : COMPUTE_ITERATIONS;
    size_t memory_iters = quick_mode ? MEMORY_ITERATIONS / 10 : MEMORY_ITERATIONS;
    size_t mixed_iters = quick_mode ? MIXED_ITERATIONS / 10 : MIXED_ITERATIONS;
    
    if (!json_output) {
        print_header("OptiWeave Comprehensive Workload Benchmark");
        
        std::cout << "\nConfiguration:\n";
        std::cout << "  Small size:    " << SMALL_SIZE << " elements\n";
        std::cout << "  Medium size:   " << MEDIUM_SIZE << " elements\n";
        std::cout << "  Large size:    " << LARGE_SIZE << " elements\n";
        std::cout << "  Warmup runs:   " << WARMUP_RUNS << "\n";
        std::cout << "  Measured runs: " << MEASURED_RUNS << "\n";
        std::cout << "  Mode:          " << (quick_mode ? "Quick" : "Full") << "\n";
    }
    
    // Store all results for summary
    std::vector<std::pair<std::string, std::vector<BenchmarkResult>>> all_results;
    
    // ========================================================================
    // Run Compute-Bound Workloads
    // ========================================================================
    if (workload_filter.empty() || workload_filter == "compute") {
        if (!json_output) {
            print_header("Compute-Bound Workload (Arithmetic Operations)");
            std::cout << "\nThis tests overhead of arithmetic operation instrumentation.\n";
            std::cout << "Workload: sqrt, sin, cos, division-heavy computation.\n\n";
            print_result_table_header();
        }
        
        std::vector<BenchmarkResult> compute_results;
        auto small = run_benchmark<ComputeBoundWorkload>("Small (1K)", SMALL_SIZE, compute_iters);
        auto medium = run_benchmark<ComputeBoundWorkload>("Medium (10K)", MEDIUM_SIZE, compute_iters);
        auto large = run_benchmark<ComputeBoundWorkload>("Large (100K)", LARGE_SIZE, compute_iters / 10);
        
        compute_results.push_back(small);
        compute_results.push_back(medium);
        compute_results.push_back(large);
        
        if (!json_output) {
            print_result_row(small);
            print_result_row(medium);
            print_result_row(large);
            print_table_footer();
        }
        
        all_results.push_back({"Compute-Bound", compute_results});
    }
    
    // ========================================================================
    // Run Memory-Bound Workloads
    // ========================================================================
    if (workload_filter.empty() || workload_filter == "memory") {
        if (!json_output) {
            print_header("Memory-Bound Workloads (Array Subscript Operations)");
            std::cout << "\nThis tests overhead of array access instrumentation.\n\n";
        }
        
        std::vector<BenchmarkResult> memory_results;
        
        // Sequential access
        if (!json_output) {
            std::cout << "Sequential Access Pattern:\n";
            print_result_table_header();
        }
        
        auto seq_small = run_benchmark<MemorySequentialWorkload>("Sequential-Small", SMALL_SIZE, memory_iters);
        auto seq_medium = run_benchmark<MemorySequentialWorkload>("Sequential-Medium", MEDIUM_SIZE, memory_iters);
        auto seq_large = run_benchmark<MemorySequentialWorkload>("Sequential-Large", LARGE_SIZE, memory_iters / 5);
        
        memory_results.push_back(seq_small);
        memory_results.push_back(seq_medium);
        memory_results.push_back(seq_large);
        
        if (!json_output) {
            print_result_row(seq_small);
            print_result_row(seq_medium);
            print_result_row(seq_large);
            print_table_footer();
            
            std::cout << "\nRandom Access Pattern:\n";
            print_result_table_header();
        }
        
        // Random access
        auto rand_small = run_benchmark<MemoryRandomWorkload>("Random-Small", SMALL_SIZE, memory_iters);
        auto rand_medium = run_benchmark<MemoryRandomWorkload>("Random-Medium", MEDIUM_SIZE, memory_iters);
        auto rand_large = run_benchmark<MemoryRandomWorkload>("Random-Large", LARGE_SIZE, memory_iters / 5);
        
        memory_results.push_back(rand_small);
        memory_results.push_back(rand_medium);
        memory_results.push_back(rand_large);
        
        if (!json_output) {
            print_result_row(rand_small);
            print_result_row(rand_medium);
            print_result_row(rand_large);
            print_table_footer();
            
            std::cout << "\nStrided Access Pattern (stride=64):\n";
            print_result_table_header();
        }
        
        // Strided access
        auto stride_small = run_benchmark<MemoryStridedWorkload>("Strided-Small", SMALL_SIZE, memory_iters);
        auto stride_medium = run_benchmark<MemoryStridedWorkload>("Strided-Medium", MEDIUM_SIZE, memory_iters);
        auto stride_large = run_benchmark<MemoryStridedWorkload>("Strided-Large", LARGE_SIZE, memory_iters / 5);
        
        memory_results.push_back(stride_small);
        memory_results.push_back(stride_medium);
        memory_results.push_back(stride_large);
        
        if (!json_output) {
            print_result_row(stride_small);
            print_result_row(stride_medium);
            print_result_row(stride_large);
            print_table_footer();
        }
        
        all_results.push_back({"Memory-Bound", memory_results});
    }
    
    // ========================================================================
    // Run Mixed Workloads
    // ========================================================================
    if (workload_filter.empty() || workload_filter == "mixed") {
        if (!json_output) {
            print_header("Mixed Workloads (Realistic Application Patterns)");
            std::cout << "\n";
        }
        
        std::vector<BenchmarkResult> mixed_results;
        
        // Matrix-vector multiplication
        if (!json_output) {
            std::cout << "Matrix-Vector Multiplication:\n";
            print_result_table_header();
        }
        
        auto mv_small = run_benchmark<MixedWorkload>("MatVec-Small", SMALL_SIZE, mixed_iters);
        auto mv_medium = run_benchmark<MixedWorkload>("MatVec-Medium", MEDIUM_SIZE, mixed_iters);
        auto mv_large = run_benchmark<MixedWorkload>("MatVec-Large", LARGE_SIZE, mixed_iters / 10);
        
        mixed_results.push_back(mv_small);
        mixed_results.push_back(mv_medium);
        mixed_results.push_back(mv_large);
        
        if (!json_output) {
            print_result_row(mv_small);
            print_result_row(mv_medium);
            print_result_row(mv_large);
            print_table_footer();
            
            std::cout << "\nSorting (Comparison-Heavy):\n";
            print_result_table_header();
        }
        
        // Sorting
        auto sort_small = run_benchmark<SortingWorkload>("Sort-Small", SMALL_SIZE, mixed_iters / 10);
        auto sort_medium = run_benchmark<SortingWorkload>("Sort-Medium", MEDIUM_SIZE, mixed_iters / 50);
        auto sort_large = run_benchmark<SortingWorkload>("Sort-Large", LARGE_SIZE, mixed_iters / 100);
        
        mixed_results.push_back(sort_small);
        mixed_results.push_back(sort_medium);
        mixed_results.push_back(sort_large);
        
        if (!json_output) {
            print_result_row(sort_small);
            print_result_row(sort_medium);
            print_result_row(sort_large);
            print_table_footer();
            
            std::cout << "\nStencil Computation (5-point):\n";
            print_result_table_header();
        }
        
        // Stencil
        auto stencil_small = run_benchmark<StencilWorkload>("Stencil-Small", SMALL_SIZE, mixed_iters);
        auto stencil_medium = run_benchmark<StencilWorkload>("Stencil-Medium", MEDIUM_SIZE, mixed_iters / 5);
        auto stencil_large = run_benchmark<StencilWorkload>("Stencil-Large", LARGE_SIZE, mixed_iters / 50);
        
        mixed_results.push_back(stencil_small);
        mixed_results.push_back(stencil_medium);
        mixed_results.push_back(stencil_large);
        
        if (!json_output) {
            print_result_row(stencil_small);
            print_result_row(stencil_medium);
            print_result_row(stencil_large);
            print_table_footer();
        }
        
        all_results.push_back({"Mixed", mixed_results});
    }
    
    // ========================================================================
    // Summary
    // ========================================================================
    if (!json_output) {
        print_header("Baseline Timing Summary");
        std::cout << "\nThese are baseline measurements (no profiling overhead).\n";
        std::cout << "Use the profiler comparison script to measure actual overhead.\n\n";
        
        std::cout << "Total Operations Measured:\n";
        size_t total_ops = 0;
        for (const auto& category : all_results) {
            std::cout << "  " << category.first << ":\n";
            for (const auto& result : category.second) {
                std::cout << "    " << std::left << std::setw(20) << result.name 
                          << ": " << std::right << std::setw(12) << result.operations << " ops, "
                          << std::fixed << std::setprecision(2) << result.avg_ms << " ms avg\n";
                total_ops += result.operations;
            }
        }
        std::cout << "\n  Total: " << total_ops << " operations\n";
        
        std::cout << "\n";
        std::cout << "To compare with profilers, run:\n";
        std::cout << "  ./run_profiler_comparison.sh\n";
    } else {
        // JSON output
        std::cout << "{\n";
        std::cout << "  \"results\": [\n";
        bool first_cat = true;
        for (const auto& category : all_results) {
            for (const auto& result : category.second) {
                if (!first_cat) std::cout << ",\n";
                first_cat = false;
                std::cout << "    {\"category\": \"" << category.first << "\", "
                          << "\"name\": \"" << result.name << "\", "
                          << "\"min_ms\": " << result.min_ms << ", "
                          << "\"avg_ms\": " << result.avg_ms << ", "
                          << "\"max_ms\": " << result.max_ms << ", "
                          << "\"stddev_ms\": " << result.stddev_ms << ", "
                          << "\"operations\": " << result.operations << "}";
            }
        }
        std::cout << "\n  ]\n";
        std::cout << "}\n";
    }
    
    return 0;
}
