#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace optiweave {
namespace timing {

// High-resolution timer for measuring operation latency
class OperationTimer {
  using clock = std::chrono::high_resolution_clock;
  clock::time_point start_;

public:
  OperationTimer() : start_(clock::now()) {}

  // Get elapsed time in nanoseconds
  uint64_t elapsed_ns() const {
    auto end = clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_)
        .count();
  }
};

// Statistics for a single operation type
struct TimingStats {
  std::atomic<uint64_t> total_time_ns{0};
  std::atomic<uint64_t> count{0};
  std::atomic<uint64_t> min_ns{UINT64_MAX};
  std::atomic<uint64_t> max_ns{0};

  // Sample-based percentile tracking (to avoid storing all measurements)
  std::vector<uint64_t> samples;
  std::mutex samples_mutex;
  uint32_t sample_rate = 100; // Sample 1 out of every 100 operations

  // Helper methods
  void record(uint64_t duration_ns);
  double get_average_ns() const;
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
