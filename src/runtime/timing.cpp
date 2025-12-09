#include <optiweave/runtime/timing.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace optiweave {
namespace timing {

// Global timing stats instance
AllTimingStats g_timing_stats;

// Control flags
#ifdef OPTIWEAVE_ENABLE_TIMING
bool g_timing_enabled = true;  // Default to enabled when compiled with timing support
bool g_profile_enabled = false;
#else
bool g_timing_enabled = false;
bool g_profile_enabled = false;
#endif
std::string g_timing_csv_path;
std::string g_timing_json_path;

// Atomic min/max helpers
static void atomic_min(std::atomic<uint64_t> &atomic_val, uint64_t new_val) {
  uint64_t current = atomic_val.load(std::memory_order_relaxed);
  while (new_val < current &&
         !atomic_val.compare_exchange_weak(current, new_val,
                                          std::memory_order_relaxed)) {
  }
}

static void atomic_max(std::atomic<uint64_t> &atomic_val, uint64_t new_val) {
  uint64_t current = atomic_val.load(std::memory_order_relaxed);
  while (new_val > current &&
         !atomic_val.compare_exchange_weak(current, new_val,
                                          std::memory_order_relaxed)) {
  }
}

// TimingStats methods
void TimingStats::record(uint64_t duration_ns) {
  total_time_ns.fetch_add(duration_ns, std::memory_order_relaxed);
  count.fetch_add(1, std::memory_order_relaxed);

  // Update min/max
  atomic_min(min_ns, duration_ns);
  atomic_max(max_ns, duration_ns);

  // Sample for percentiles (1 out of sample_rate operations)
  if (g_profile_enabled && (rand() % sample_rate == 0)) {
    std::lock_guard<std::mutex> lock(samples_mutex);
    samples.push_back(duration_ns);
  }
}

double TimingStats::get_average_ns() const {
  uint64_t c = count.load();
  if (c == 0)
    return 0.0;
  return static_cast<double>(total_time_ns.load()) / c;
}

void initialize() {
  // Check environment variables
  const char *env_timing = std::getenv("OPTIWEAVE_TIMING");
  const char *env_profile = std::getenv("OPTIWEAVE_PROFILE");
  const char *env_csv = std::getenv("OPTIWEAVE_TIMING_CSV");
  const char *env_json = std::getenv("OPTIWEAVE_TIMING_JSON");

  if (env_timing && std::string(env_timing) == "1") {
    g_timing_enabled = true;
  }

  if (env_profile && std::string(env_profile) == "1") {
    g_timing_enabled = true;
    g_profile_enabled = true;
  }

  if (env_csv) {
    g_timing_csv_path = env_csv;
    g_timing_enabled = true;
  }

  if (env_json) {
    g_timing_json_path = env_json;
    g_timing_enabled = true;
  }

  if (g_timing_enabled) {
    std::atexit(finalize);
  }
}

void finalize() {
  if (!g_timing_enabled) {
    return;
  }

  // Print to stdout
  print_timing_statistics();

  // Export if requested
  if (!g_timing_csv_path.empty()) {
    export_timing_csv(g_timing_csv_path);
    std::cout << "\nTiming CSV report saved to: " << g_timing_csv_path << "\n";
  }

  if (!g_timing_json_path.empty()) {
    export_timing_json(g_timing_json_path);
    std::cout << "Timing JSON report saved to: " << g_timing_json_path << "\n";
  }
}

// Helper to format time in human-readable units
static std::string format_time(uint64_t ns) {
  std::ostringstream oss;
  if (ns >= 1000000000) { // >= 1s
    oss << std::fixed << std::setprecision(2) << (ns / 1000000000.0) << "s";
  } else if (ns >= 1000000) { // >= 1ms
    oss << std::fixed << std::setprecision(2) << (ns / 1000000.0) << "ms";
  } else if (ns >= 1000) { // >= 1μs
    oss << std::fixed << std::setprecision(2) << (ns / 1000.0) << "μs";
  } else {
    oss << ns << "ns";
  }
  return oss.str();
}

// Helper struct for sorting
struct OperationTiming {
  std::string name;
  uint64_t count;
  uint64_t total_time_ns;
  double avg_ns;
  uint64_t min_ns;
  uint64_t max_ns;
  PercentileStats percentiles;
  double percentage;
};

void print_timing_statistics() {
  if (!g_timing_enabled) {
    return;
  }

  std::cout << "\n";
  std::cout << "╔══════════════════════════════════════════════════╗\n";
  std::cout << "║       OptiWeave Performance Profile              ║\n";
  std::cout << "╚══════════════════════════════════════════════════╝\n";
  std::cout << "\n";

  // Calculate total time across all operations
  uint64_t total_time_ns = 0;
  total_time_ns += g_timing_stats.array_subscript.total_time_ns.load();
  total_time_ns += g_timing_stats.addition.total_time_ns.load();
  total_time_ns += g_timing_stats.subtraction.total_time_ns.load();
  total_time_ns += g_timing_stats.multiplication.total_time_ns.load();
  total_time_ns += g_timing_stats.division.total_time_ns.load();
  total_time_ns += g_timing_stats.modulo.total_time_ns.load();
  total_time_ns += g_timing_stats.assignment.total_time_ns.load();
  total_time_ns += g_timing_stats.add_assign.total_time_ns.load();
  total_time_ns += g_timing_stats.sub_assign.total_time_ns.load();
  total_time_ns += g_timing_stats.mul_assign.total_time_ns.load();
  total_time_ns += g_timing_stats.div_assign.total_time_ns.load();
  total_time_ns += g_timing_stats.mod_assign.total_time_ns.load();
  total_time_ns += g_timing_stats.equal.total_time_ns.load();
  total_time_ns += g_timing_stats.not_equal.total_time_ns.load();
  total_time_ns += g_timing_stats.less_than.total_time_ns.load();
  total_time_ns += g_timing_stats.greater_than.total_time_ns.load();
  total_time_ns += g_timing_stats.less_equal.total_time_ns.load();
  total_time_ns += g_timing_stats.greater_equal.total_time_ns.load();

  if (total_time_ns == 0) {
    std::cout << "No timing data collected.\n";
    return;
  }

  std::cout << "Total Time: " << format_time(total_time_ns) << "\n\n";

  // Collect all operations with timing data
  std::vector<OperationTiming> ops;

  auto add_op = [&](const std::string &name, TimingStats &stats) {
    uint64_t count = stats.count.load();
    if (count > 0) {
      OperationTiming op;
      op.name = name;
      op.count = count;
      op.total_time_ns = stats.total_time_ns.load();
      op.avg_ns = stats.get_average_ns();
      op.min_ns = stats.min_ns.load();
      op.max_ns = stats.max_ns.load();
      op.percentage = (op.total_time_ns * 100.0) / total_time_ns;

      // Calculate percentiles if profiling enabled
      if (g_profile_enabled && !stats.samples.empty()) {
        std::lock_guard<std::mutex> lock(stats.samples_mutex);
        auto samples_copy = stats.samples;
        op.percentiles = calculate_percentiles(samples_copy);
      }

      ops.push_back(op);
    }
  };

  add_op("Array Access", g_timing_stats.array_subscript);
  add_op("Addition", g_timing_stats.addition);
  add_op("Subtraction", g_timing_stats.subtraction);
  add_op("Multiplication", g_timing_stats.multiplication);
  add_op("Division", g_timing_stats.division);
  add_op("Modulo", g_timing_stats.modulo);
  add_op("Assignment", g_timing_stats.assignment);
  add_op("Add-Assign (+=)", g_timing_stats.add_assign);
  add_op("Sub-Assign (-=)", g_timing_stats.sub_assign);
  add_op("Mul-Assign (*=)", g_timing_stats.mul_assign);
  add_op("Div-Assign (/=)", g_timing_stats.div_assign);
  add_op("Mod-Assign (%=)", g_timing_stats.mod_assign);
  add_op("Equal (==)", g_timing_stats.equal);
  add_op("Not Equal (!=)", g_timing_stats.not_equal);
  add_op("Less Than (<)", g_timing_stats.less_than);
  add_op("Greater Than (>)", g_timing_stats.greater_than);
  add_op("Less/Equal (<=)", g_timing_stats.less_equal);
  add_op("Greater/Equal (>=)", g_timing_stats.greater_equal);

  // Sort by total time (descending)
  std::sort(ops.begin(), ops.end(),
            [](const OperationTiming &a, const OperationTiming &b) {
              return a.total_time_ns > b.total_time_ns;
            });

  // Print table header
  std::cout << "Time Breakdown by Operation:\n";
  if (g_profile_enabled) {
    std::cout << "┌─────────────────┬─────────┬──────────┬─────────┬─────────┬─────────┬─────────┐\n";
    std::cout << "│ Operation       │ Count   │ Total    │ Avg     │ Min     │ Max     │ p95     │\n";
    std::cout << "├─────────────────┼─────────┼──────────┼─────────┼─────────┼─────────┼─────────┤\n";
  } else {
    std::cout << "┌─────────────────┬─────────┬──────────┬─────────┬─────────┬─────────┐\n";
    std::cout << "│ Operation       │ Count   │ Total    │ Avg     │ Min     │ Max     │\n";
    std::cout << "├─────────────────┼─────────┼──────────┼─────────┼─────────┼─────────┤\n";
  }

  // Print table rows
  for (const auto &op : ops) {
    std::cout << "│ " << std::left << std::setw(15) << op.name << " │ "
              << std::right << std::setw(7) << op.count << " │ "
              << std::setw(8) << format_time(op.total_time_ns) << " │ "
              << std::setw(7) << format_time(static_cast<uint64_t>(op.avg_ns)) << " │ "
              << std::setw(7) << format_time(op.min_ns) << " │ "
              << std::setw(7) << format_time(op.max_ns) << " │";

    if (g_profile_enabled) {
      std::cout << " " << std::setw(7) << format_time(op.percentiles.p95) << " │";
    }

    std::cout << "\n";
  }

  // Print table footer
  if (g_profile_enabled) {
    std::cout << "└─────────────────┴─────────┴──────────┴─────────┴─────────┴─────────┴─────────┘\n";
  } else {
    std::cout << "└─────────────────┴─────────┴──────────┴─────────┴─────────┴─────────┘\n";
  }

  // Time distribution visualization
  std::cout << "\nTime Distribution:\n";
  for (const auto &op : ops) {
    int bar_length = static_cast<int>(op.percentage * 0.5); // Scale to fit
    std::cout << "  " << std::left << std::setw(18) << op.name << " ";
    for (int i = 0; i < bar_length; ++i) {
      std::cout << "█";
    }
    std::cout << " " << std::fixed << std::setprecision(1) << op.percentage
              << "%  (" << format_time(op.total_time_ns) << ")\n";
  }

  // Key insights
  if (!ops.empty()) {
    std::cout << "\nKey Insights:\n";
    const auto &dominant = ops[0];
    std::cout << "• " << dominant.name << " is the bottleneck (" << std::fixed
              << std::setprecision(1) << dominant.percentage << "% of time)\n";

    // Check for slow operations
    if (dominant.avg_ns > 100) { // > 100ns average
      std::cout << "• Average " << dominant.name << " time ("
                << format_time(static_cast<uint64_t>(dominant.avg_ns))
                << ") suggests possible cache misses\n";
    }
  }

  std::cout << "\n";
}

PercentileStats calculate_percentiles(std::vector<uint64_t> &samples) {
  if (samples.empty()) {
    return {0, 0, 0, 0};
  }

  std::sort(samples.begin(), samples.end());

  PercentileStats stats;
  size_t n = samples.size();

  stats.p50 = samples[n * 50 / 100];
  stats.p90 = samples[n * 90 / 100];
  stats.p95 = samples[n * 95 / 100];
  stats.p99 = samples[n * 99 / 100];

  return stats;
}

std::vector<HistogramBin> generate_histogram(const std::vector<uint64_t> &samples,
                                              size_t num_bins) {
  std::vector<HistogramBin> histogram;
  if (samples.empty() || num_bins == 0) {
    return histogram;
  }

  auto min_val = *std::min_element(samples.begin(), samples.end());
  auto max_val = *std::max_element(samples.begin(), samples.end());

  uint64_t range = max_val - min_val;
  uint64_t bin_width = range / num_bins + 1;

  // Initialize bins
  for (size_t i = 0; i < num_bins; ++i) {
    HistogramBin bin;
    bin.lower_bound_ns = min_val + i * bin_width;
    bin.upper_bound_ns = min_val + (i + 1) * bin_width;
    bin.count = 0;
    histogram.push_back(bin);
  }

  // Fill bins
  for (uint64_t sample : samples) {
    size_t bin_idx = std::min(static_cast<size_t>((sample - min_val) / bin_width), num_bins - 1);
    histogram[bin_idx].count++;
  }

  return histogram;
}

void export_timing_csv(const std::string &filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open timing CSV file: " << filename << "\n";
    return;
  }

  // Write header
  file << "Operation,Count,TotalTimeNs,AvgNs,MinNs,MaxNs\n";

  auto write_op = [&](const std::string &name, TimingStats &stats) {
    uint64_t count = stats.count.load();
    if (count > 0) {
      file << name << "," << count << "," << stats.total_time_ns.load() << ","
           << static_cast<uint64_t>(stats.get_average_ns()) << ","
           << stats.min_ns.load() << "," << stats.max_ns.load() << "\n";
    }
  };

  write_op("ArrayAccess", g_timing_stats.array_subscript);
  write_op("Addition", g_timing_stats.addition);
  write_op("Subtraction", g_timing_stats.subtraction);
  write_op("Multiplication", g_timing_stats.multiplication);
  write_op("Division", g_timing_stats.division);
  write_op("Modulo", g_timing_stats.modulo);
  write_op("Assignment", g_timing_stats.assignment);
  write_op("AddAssign", g_timing_stats.add_assign);
  write_op("SubAssign", g_timing_stats.sub_assign);
  write_op("MulAssign", g_timing_stats.mul_assign);
  write_op("DivAssign", g_timing_stats.div_assign);
  write_op("ModAssign", g_timing_stats.mod_assign);
  write_op("Equal", g_timing_stats.equal);
  write_op("NotEqual", g_timing_stats.not_equal);
  write_op("LessThan", g_timing_stats.less_than);
  write_op("GreaterThan", g_timing_stats.greater_than);
  write_op("LessEqual", g_timing_stats.less_equal);
  write_op("GreaterEqual", g_timing_stats.greater_equal);

  file.close();
}

void export_timing_json(const std::string &filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open timing JSON file: " << filename << "\n";
    return;
  }

  file << "{\n";
  file << "  \"operations\": {\n";

  bool first = true;
  auto write_op = [&](const std::string &name, TimingStats &stats) {
    uint64_t count = stats.count.load();
    if (count > 0) {
      if (!first)
        file << ",\n";
      first = false;

      file << "    \"" << name << "\": {\n";
      file << "      \"count\": " << count << ",\n";
      file << "      \"total_time_ns\": " << stats.total_time_ns.load() << ",\n";
      file << "      \"avg_ns\": " << static_cast<uint64_t>(stats.get_average_ns())
           << ",\n";
      file << "      \"min_ns\": " << stats.min_ns.load() << ",\n";
      file << "      \"max_ns\": " << stats.max_ns.load() << "\n";
      file << "    }";
    }
  };

  write_op("array_access", g_timing_stats.array_subscript);
  write_op("addition", g_timing_stats.addition);
  write_op("subtraction", g_timing_stats.subtraction);
  write_op("multiplication", g_timing_stats.multiplication);
  write_op("division", g_timing_stats.division);
  write_op("modulo", g_timing_stats.modulo);
  write_op("assignment", g_timing_stats.assignment);
  write_op("add_assign", g_timing_stats.add_assign);
  write_op("sub_assign", g_timing_stats.sub_assign);
  write_op("mul_assign", g_timing_stats.mul_assign);
  write_op("div_assign", g_timing_stats.div_assign);
  write_op("mod_assign", g_timing_stats.mod_assign);
  write_op("equal", g_timing_stats.equal);
  write_op("not_equal", g_timing_stats.not_equal);
  write_op("less_than", g_timing_stats.less_than);
  write_op("greater_than", g_timing_stats.greater_than);
  write_op("less_equal", g_timing_stats.less_equal);
  write_op("greater_equal", g_timing_stats.greater_equal);

  file << "\n  }\n";
  file << "}\n";

  file.close();
}

} // namespace timing
} // namespace optiweave
