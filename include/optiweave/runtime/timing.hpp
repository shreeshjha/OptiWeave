#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>
#include <array>

// Force inline macro for hot paths
#if defined(__GNUC__) || defined(__clang__)
  #define OPTIWEAVE_TIMING_FORCE_INLINE __attribute__((always_inline)) inline
  #define OPTIWEAVE_TIMING_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
  #define OPTIWEAVE_TIMING_FORCE_INLINE inline
  #define OPTIWEAVE_TIMING_UNLIKELY(x) (x)
#endif

// Use RDTSC for fast cycle counting on x86_64
#if defined(__x86_64__) || defined(_M_X64)
  #define OPTIWEAVE_TIMING_HAS_RDTSC 1
  OPTIWEAVE_TIMING_FORCE_INLINE uint64_t optiweave_timing_rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
  }
#else
  #define OPTIWEAVE_TIMING_HAS_RDTSC 0
#endif

namespace optiweave {
namespace timing {

// High-resolution timer for measuring operation latency
// OPTIMIZED: Uses RDTSC on x86_64 for ~10x faster timing
class OperationTimer {
#if OPTIWEAVE_TIMING_HAS_RDTSC
  uint64_t start_;
public:
  OperationTimer() : start_(optiweave_timing_rdtsc()) {}
  
  // Get elapsed cycles (convert to ns at reporting time)
  OPTIWEAVE_TIMING_FORCE_INLINE uint64_t elapsed_cycles() const {
    return optiweave_timing_rdtsc() - start_;
  }
  
  // Get elapsed time in nanoseconds (approximate, assuming ~3GHz)
  OPTIWEAVE_TIMING_FORCE_INLINE uint64_t elapsed_ns() const {
    return elapsed_cycles() / 3;  // Approximate conversion
  }
#else
  using clock = std::chrono::high_resolution_clock;
  clock::time_point start_;
public:
  OperationTimer() : start_(clock::now()) {}

  uint64_t elapsed_ns() const {
    auto end = clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_)
        .count();
  }
#endif
};

// Lock-free reservoir sampling buffer for timing samples
// OPTIMIZED: No mutex in hot path, uses atomic operations
class ReservoirSampler {
  static constexpr size_t MAX_SAMPLES = 8192;  // Power of 2 for fast modulo
  std::array<std::atomic<uint64_t>, MAX_SAMPLES> samples_{};
  std::atomic<uint64_t> sample_count_{0};
  
public:
  // Record a sample using lock-free reservoir sampling
  OPTIWEAVE_TIMING_FORCE_INLINE void record(uint64_t value) {
    uint64_t idx = sample_count_.fetch_add(1, std::memory_order_relaxed);
    
    if (idx < MAX_SAMPLES) {
      // First MAX_SAMPLES samples go directly into buffer
      samples_[idx].store(value, std::memory_order_relaxed);
    } else {
      // Reservoir sampling: replace random element
      // Use simple hash of idx as pseudo-random
      uint64_t r = (idx * 2654435761ULL) % (idx + 1);
      if (r < MAX_SAMPLES) {
        samples_[r].store(value, std::memory_order_relaxed);
      }
    }
  }
  
  // Get samples for analysis (not thread-safe, call after measurement)
  std::vector<uint64_t> get_samples() const {
    size_t count = std::min(sample_count_.load(), (uint64_t)MAX_SAMPLES);
    std::vector<uint64_t> result;
    result.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      result.push_back(samples_[i].load(std::memory_order_relaxed));
    }
    return result;
  }
  
  size_t count() const { return sample_count_.load(std::memory_order_relaxed); }
  void reset() { sample_count_.store(0, std::memory_order_relaxed); }
};

// Statistics for a single operation type
// OPTIMIZED: Lock-free recording, thread-local sample counter
struct TimingStats {
  std::atomic<uint64_t> total_time_ns{0};
  std::atomic<uint64_t> count{0};
  std::atomic<uint64_t> min_ns{UINT64_MAX};
  std::atomic<uint64_t> max_ns{0};

  // OPTIMIZED: Lock-free reservoir sampling
  ReservoirSampler sampler;
  uint32_t sample_rate = 100; // Sample 1 out of every 100 operations

  // Legacy compatibility (deprecated, use sampler instead)
  std::vector<uint64_t> samples;
  std::mutex samples_mutex;

  // OPTIMIZED: Thread-local sample counter (no rand())
  OPTIWEAVE_TIMING_FORCE_INLINE void record(uint64_t duration_ns) {
    // Always update basic stats (atomic, lock-free)
    total_time_ns.fetch_add(duration_ns, std::memory_order_relaxed);
    count.fetch_add(1, std::memory_order_relaxed);
    
    // Update min/max using CAS loops
    uint64_t current_min = min_ns.load(std::memory_order_relaxed);
    while (duration_ns < current_min && 
           !min_ns.compare_exchange_weak(current_min, duration_ns, std::memory_order_relaxed));
    
    uint64_t current_max = max_ns.load(std::memory_order_relaxed);
    while (duration_ns > current_max && 
           !max_ns.compare_exchange_weak(current_max, duration_ns, std::memory_order_relaxed));
    
    // OPTIMIZED: Use thread-local counter instead of rand()
    static thread_local uint32_t tl_sample_counter = 0;
    if (OPTIWEAVE_TIMING_UNLIKELY(++tl_sample_counter >= sample_rate)) {
      tl_sample_counter = 0;
      sampler.record(duration_ns);
    }
  }
  
  double get_average_ns() const {
    uint64_t c = count.load(std::memory_order_relaxed);
    return c > 0 ? static_cast<double>(total_time_ns.load()) / c : 0.0;
  }
};

// Global timing stats for each operation type
struct AllTimingStats {
  TimingStats array_subscript;
  TimingStats addition;
  TimingStats subtraction;
  TimingStats multiplication;
  TimingStats division;
  TimingStats modulo;
  TimingStats assignment;
  TimingStats add_assign;
  TimingStats sub_assign;
  TimingStats mul_assign;
  TimingStats div_assign;
  TimingStats mod_assign;
  TimingStats equal;
  TimingStats not_equal;
  TimingStats less_than;
  TimingStats greater_than;
  TimingStats less_equal;
  TimingStats greater_equal;
};

// Global timing stats instance
extern AllTimingStats g_timing_stats;

// Control flags
extern bool g_timing_enabled;
extern bool g_profile_enabled; // Full profiling with percentiles
extern std::string g_timing_csv_path;
extern std::string g_timing_json_path;

// RAII timer that records duration on destruction
// OPTIMIZED: Uses RDTSC on x86_64
class ScopedOperationTimer {
  OperationTimer timer_;
  TimingStats &stats_;

public:
  explicit ScopedOperationTimer(TimingStats &stats) : stats_(stats) {}

  ~ScopedOperationTimer() {
    if (g_timing_enabled) {
      uint64_t duration = timer_.elapsed_ns();
      stats_.record(duration);
    }
  }
};

// Initialize timing system
void initialize();

// Finalize and print timing statistics
void finalize();

// Print timing statistics to stdout
void print_timing_statistics();

// Export timing data
void export_timing_csv(const std::string &filename);
void export_timing_json(const std::string &filename);

// Calculate percentiles from samples
struct PercentileStats {
  uint64_t p50;
  uint64_t p90;
  uint64_t p95;
  uint64_t p99;
};

PercentileStats calculate_percentiles(std::vector<uint64_t> &samples);

// Generate histogram data
struct HistogramBin {
  uint64_t lower_bound_ns;
  uint64_t upper_bound_ns;
  uint64_t count;
};

std::vector<HistogramBin> generate_histogram(const std::vector<uint64_t> &samples,
                                              size_t num_bins = 10);

} // namespace timing
} // namespace optiweave
