// OptiWeave Counter Overhead Micro-Benchmark
// Tests the overhead of the optimized thread-local counter approach
// This directly measures the counter performance without template complexity

#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include <cstdint>
#include <atomic>

constexpr int WARMUP = 5;
constexpr int ITERATIONS = 20;
constexpr int ARRAY_SIZE = 10000;
constexpr int LOOP_COUNT = 10000;

using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;

// Use volatile to prevent optimization
volatile int64_t g_sink = 0;

// ============================================================================
// SIMULATE DIFFERENT INSTRUMENTATION APPROACHES
// ============================================================================

// Shared test data
std::vector<int> test_array;

void init_test_array() {
    test_array.resize(ARRAY_SIZE);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        test_array[i] = i % 100;
    }
}

// 1. No instrumentation (baseline)
void baseline_no_instrumentation() {
    int64_t sum = 0;
    for (int iter = 0; iter < LOOP_COUNT; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            sum += test_array[i];  // Real memory access
        }
    }
    g_sink = sum;
}

// 2. Atomic counter (old approach - HIGH overhead due to contention)
std::atomic<uint64_t> g_atomic_counter{0};

void with_atomic_counter() {
    int64_t sum = 0;
    for (int iter = 0; iter < LOOP_COUNT; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            g_atomic_counter.fetch_add(1, std::memory_order_relaxed);
            sum += test_array[i];
        }
    }
    g_sink = sum;
}

// 3. Thread-local counter (NEW optimized approach - LOW overhead)
thread_local uint64_t tl_counter = 0;

void with_thread_local_counter() {
    int64_t sum = 0;
    for (int iter = 0; iter < LOOP_COUNT; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            ++tl_counter;  // No atomic, no contention
            sum += test_array[i];
        }
    }
    g_sink = sum;
}

// 4. Thread-local counter with runtime check (standard mode)
bool g_stats_enabled = true;
thread_local uint64_t tl_counter_checked = 0;

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

void with_checked_counter() {
    int64_t sum = 0;
    for (int iter = 0; iter < LOOP_COUNT; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            if (LIKELY(g_stats_enabled)) {
                ++tl_counter_checked;
            }
            sum += test_array[i];
        }
    }
    g_sink = sum;
}

// 5. Sampled counter (every 256 operations using bitmask - branchless!)
thread_local uint64_t tl_counter_sampled = 0;
thread_local uint32_t sample_counter = 0;
constexpr uint32_t SAMPLE_MASK = 0xFF;  // Sample every 256 operations

void with_sampled_counter() {
    int64_t sum = 0;
    for (int iter = 0; iter < LOOP_COUNT; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            // Branchless sampling using bitmask
            uint32_t should_sample = ((++sample_counter) & SAMPLE_MASK) == 0;
            tl_counter_sampled += should_sample * 256;  // Branchless add
            sum += test_array[i];
        }
    }
    g_sink = sum;
}

// 6. Full instrumentation simulation (counter + source location recording)
struct SourceLocation {
    const char* file;
    int line;
    const char* func;
};

thread_local SourceLocation last_location;
thread_local uint64_t tl_counter_full = 0;

void with_full_instrumentation() {
    int64_t sum = 0;
    for (int iter = 0; iter < LOOP_COUNT; iter++) {
        for (int i = 0; i < ARRAY_SIZE; i++) {
            // Simulate full instrumentation:
            // 1. Increment counter
            ++tl_counter_full;
            // 2. Record source location (lightweight)
            last_location = {"test.cpp", 123, "test_func"};
            // 3. Access array
            sum += test_array[i];
        }
    }
    g_sink = sum;
}

// ============================================================================
// BENCHMARK INFRASTRUCTURE
// ============================================================================

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
        if (baseline.avg() < 0.001) return 0.0;  // Avoid division by zero
        return (avg() - baseline.avg()) / baseline.avg() * 100.0;
    }
};

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
    std::cout << "├─────────────────────────────────┼──────────┼──────────┼──────────┼──────────┤\n";
}

int main() {
    init_test_array();
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         OptiWeave Counter Overhead Micro-Benchmark                            ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Configuration:\n";
    std::cout << "  Array size:       " << ARRAY_SIZE << "\n";
    std::cout << "  Loop iterations:  " << LOOP_COUNT << "\n";
    std::cout << "  Total operations: " << (uint64_t)ARRAY_SIZE * LOOP_COUNT << " (" << (ARRAY_SIZE * LOOP_COUNT / 1000000) << "M)\n";
    std::cout << "  Warmup runs:      " << WARMUP << "\n";
    std::cout << "  Measured runs:    " << ITERATIONS << "\n\n";
    
    std::cout << "This benchmark compares different counter strategies:\n";
    std::cout << "  1. Baseline (no counting) - actual array access\n";
    std::cout << "  2. Atomic counter (old approach)\n";
    std::cout << "  3. Thread-local counter (optimized)\n";
    std::cout << "  4. Thread-local with runtime check (standard mode)\n";
    std::cout << "  5. Sampled counter (1/256, branchless)\n";
    std::cout << "  6. Full instrumentation (counter + source location)\n\n";
    
    std::cout << "Running benchmarks...\n\n";
    
    // Run benchmarks
    auto baseline = run_bench("Baseline", baseline_no_instrumentation);
    auto atomic = run_bench("Atomic Counter", with_atomic_counter);
    auto thread_local_ = run_bench("Thread-Local Counter", with_thread_local_counter);
    auto checked = run_bench("Thread-Local + Check", with_checked_counter);
    auto sampled = run_bench("Sampled (1/100)", with_sampled_counter);
    auto full = run_bench("Full Instrumentation", with_full_instrumentation);
    
    // Print results
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "┌─────────────────────────────────┬──────────┬──────────┬──────────┬──────────┐\n";
    std::cout << "│ Counter Strategy                │ Min (ms) │ Avg (ms) │ Max (ms) │ Overhead │\n";
    std::cout << "├─────────────────────────────────┼──────────┼──────────┼──────────┼──────────┤\n";
    
    auto print_row = [&baseline](const char* name, const BenchResult& r, bool show_overhead = true) {
        std::cout << "│ " << std::left << std::setw(31) << name << " │ "
                  << std::right << std::setw(8) << r.min_ms << " │ "
                  << std::setw(8) << r.avg() << " │ "
                  << std::setw(8) << r.max_ms << " │ ";
        if (show_overhead) {
            std::cout << std::setw(7) << r.overhead(baseline) << "% │\n";
        } else {
            std::cout << std::setw(8) << "-" << " │\n";
        }
    };
    
    print_row("1. Baseline (no counting)", baseline, false);
    print_separator();
    print_row("2. Atomic Counter (OLD)", atomic, true);
    print_separator();
    print_row("3. Thread-Local (OPTIMIZED)", thread_local_, true);
    print_separator();
    print_row("4. Thread-Local + Check", checked, true);
    print_separator();
    print_row("5. Sampled (1/256 branchless)", sampled, true);
    print_separator();
    print_row("6. Full Instrumentation", full, true);
    std::cout << "└─────────────────────────────────┴──────────┴──────────┴──────────┴──────────┘\n\n";
    
    // Analysis
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "OVERHEAD SUMMARY:\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";
    
    std::cout << "  Atomic Counter:            " << std::setw(7) << atomic.overhead(baseline) << "%\n";
    std::cout << "  Thread-Local Counter:      " << std::setw(7) << thread_local_.overhead(baseline) << "% (OPTIMIZED)\n";
    std::cout << "  Thread-Local + Check:      " << std::setw(7) << checked.overhead(baseline) << "%\n";
    std::cout << "  Sampled (1/256):           " << std::setw(7) << sampled.overhead(baseline) << "%\n";
    std::cout << "  Full Instrumentation:      " << std::setw(7) << full.overhead(baseline) << "%\n\n";
    
    double improvement = atomic.overhead(baseline) - thread_local_.overhead(baseline);
    std::cout << "IMPROVEMENT (Atomic → Thread-Local): " << improvement << " percentage points\n\n";
    
    // Evaluation against target
    double target_low = 8.0;
    double target_high = 17.0;
    
    std::cout << "TARGET EVALUATION (8-17% overhead goal):\n";
    
    auto evaluate = [=](const char* name, double overhead) {
        std::cout << "  " << std::left << std::setw(25) << name << ": ";
        if (overhead < 5.0) {
            std::cout << "✅ EXCELLENT (<5%)\n";
        } else if (overhead < target_low) {
            std::cout << "✅ VERY GOOD (<8%)\n";
        } else if (overhead <= target_high) {
            std::cout << "✅ GOOD (within 8-17% target)\n";
        } else if (overhead < 30.0) {
            std::cout << "⚠️  ACCEPTABLE (<30%)\n";
        } else {
            std::cout << "❌ HIGH (>" << overhead << "%)\n";
        }
    };
    
    evaluate("Thread-Local Counter", thread_local_.overhead(baseline));
    evaluate("Thread-Local + Check", checked.overhead(baseline));
    evaluate("Sampled (1/256)", sampled.overhead(baseline));
    evaluate("Full Instrumentation", full.overhead(baseline));
    
    std::cout << "\n";
    
    // Counter verification
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n";
    std::cout << "COUNTER VERIFICATION:\n";
    std::cout << "═══════════════════════════════════════════════════════════════════════════════\n\n";
    
    uint64_t expected = (uint64_t)ARRAY_SIZE * LOOP_COUNT * (WARMUP + ITERATIONS);
    std::cout << "  Expected operations:    " << expected << "\n";
    std::cout << "  Atomic counter:         " << g_atomic_counter.load() << "\n";
    std::cout << "  Thread-local counter:   " << tl_counter << "\n";
    std::cout << "  Thread-local (checked): " << tl_counter_checked << "\n";
    std::cout << "  Sampled counter:        " << tl_counter_sampled << " (expected: ~" << expected/256 * 256 << ")\n";
    std::cout << "  Full instr. counter:    " << tl_counter_full << "\n\n";
    
    return 0;
}
