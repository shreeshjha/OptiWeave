#include <optiweave/runtime/statistics.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace optiweave {
namespace statistics {

// Global counter instance (for final aggregation)
OperationCounters g_counters;

// Thread-local counter instance (fast path - no atomic contention)
thread_local ThreadLocalCounters tl_counters;

// Timing
std::chrono::high_resolution_clock::time_point g_start_time;

// Control flags
#ifdef OPTIWEAVE_ENABLE_STATS
bool g_stats_enabled = true;  // Default to enabled when compiled with stats support
#else
bool g_stats_enabled = false;
#endif
std::string g_stats_csv_path;
std::string g_stats_json_path;

// Helper struct for sorting operations by count
struct OperationInfo {
  std::string name;
  uint64_t count;
  double percentage;
  double ops_per_second;
};

// Flush thread-local counters to global atomics
// This should be called before reading the global counters
void flush_thread_local_counters() {
  // Flush all thread-local counters to global atomics using relaxed ordering
  // (sufficient since we only need eventual consistency at finalize time)
  if (tl_counters.array_subscript > 0) {
    g_counters.array_subscript.fetch_add(tl_counters.array_subscript, std::memory_order_relaxed);
    tl_counters.array_subscript = 0;
  }
  if (tl_counters.addition > 0) {
    g_counters.addition.fetch_add(tl_counters.addition, std::memory_order_relaxed);
    tl_counters.addition = 0;
  }
  if (tl_counters.subtraction > 0) {
    g_counters.subtraction.fetch_add(tl_counters.subtraction, std::memory_order_relaxed);
    tl_counters.subtraction = 0;
  }
  if (tl_counters.multiplication > 0) {
    g_counters.multiplication.fetch_add(tl_counters.multiplication, std::memory_order_relaxed);
    tl_counters.multiplication = 0;
  }
  if (tl_counters.division > 0) {
    g_counters.division.fetch_add(tl_counters.division, std::memory_order_relaxed);
    tl_counters.division = 0;
  }
  if (tl_counters.modulo > 0) {
    g_counters.modulo.fetch_add(tl_counters.modulo, std::memory_order_relaxed);
    tl_counters.modulo = 0;
  }
  if (tl_counters.assignment > 0) {
    g_counters.assignment.fetch_add(tl_counters.assignment, std::memory_order_relaxed);
    tl_counters.assignment = 0;
  }
  if (tl_counters.add_assign > 0) {
    g_counters.add_assign.fetch_add(tl_counters.add_assign, std::memory_order_relaxed);
    tl_counters.add_assign = 0;
  }
  if (tl_counters.sub_assign > 0) {
    g_counters.sub_assign.fetch_add(tl_counters.sub_assign, std::memory_order_relaxed);
    tl_counters.sub_assign = 0;
  }
  if (tl_counters.mul_assign > 0) {
    g_counters.mul_assign.fetch_add(tl_counters.mul_assign, std::memory_order_relaxed);
    tl_counters.mul_assign = 0;
  }
  if (tl_counters.div_assign > 0) {
    g_counters.div_assign.fetch_add(tl_counters.div_assign, std::memory_order_relaxed);
    tl_counters.div_assign = 0;
  }
  if (tl_counters.mod_assign > 0) {
    g_counters.mod_assign.fetch_add(tl_counters.mod_assign, std::memory_order_relaxed);
    tl_counters.mod_assign = 0;
  }
  if (tl_counters.equal > 0) {
    g_counters.equal.fetch_add(tl_counters.equal, std::memory_order_relaxed);
    tl_counters.equal = 0;
  }
  if (tl_counters.not_equal > 0) {
    g_counters.not_equal.fetch_add(tl_counters.not_equal, std::memory_order_relaxed);
    tl_counters.not_equal = 0;
  }
  if (tl_counters.less_than > 0) {
    g_counters.less_than.fetch_add(tl_counters.less_than, std::memory_order_relaxed);
    tl_counters.less_than = 0;
  }
  if (tl_counters.greater_than > 0) {
    g_counters.greater_than.fetch_add(tl_counters.greater_than, std::memory_order_relaxed);
    tl_counters.greater_than = 0;
  }
  if (tl_counters.less_equal > 0) {
    g_counters.less_equal.fetch_add(tl_counters.less_equal, std::memory_order_relaxed);
    tl_counters.less_equal = 0;
  }
  if (tl_counters.greater_equal > 0) {
    g_counters.greater_equal.fetch_add(tl_counters.greater_equal, std::memory_order_relaxed);
    tl_counters.greater_equal = 0;
  }
}

void initialize() {
  // Check environment variables
  const char *env_stats = std::getenv("OPTIWEAVE_STATS");
  const char *env_csv = std::getenv("OPTIWEAVE_STATS_CSV");
  const char *env_json = std::getenv("OPTIWEAVE_STATS_JSON");

  if (env_stats && std::string(env_stats) == "1") {
    g_stats_enabled = true;
  }

  if (env_csv) {
    g_stats_csv_path = env_csv;
    g_stats_enabled = true;
  }

  if (env_json) {
    g_stats_json_path = env_json;
    g_stats_enabled = true;
  }

  if (g_stats_enabled) {
    g_start_time = std::chrono::high_resolution_clock::now();
    std::atexit(finalize);
  }
}

void finalize() {
  if (!g_stats_enabled) {
    return;
  }

  // Flush thread-local counters to global atomics before reporting
  // Note: In multi-threaded programs, each thread should call this before exit
  // For single-threaded programs or main thread, this captures all counts
  flush_thread_local_counters();

  // Print to stdout
  print_statistics();

  // Export to CSV if requested
  if (!g_stats_csv_path.empty()) {
    export_csv(g_stats_csv_path);
    std::cout << "\nCSV report saved to: " << g_stats_csv_path << "\n";
  }

  // Export to JSON if requested
  if (!g_stats_json_path.empty()) {
    export_json(g_stats_json_path);
    std::cout << "JSON report saved to: " << g_stats_json_path << "\n";
  }
}

uint64_t get_total_operations() {
  uint64_t total = 0;

  total += g_counters.array_subscript.load();
  total += g_counters.addition.load();
  total += g_counters.subtraction.load();
  total += g_counters.multiplication.load();
  total += g_counters.division.load();
  total += g_counters.modulo.load();
  total += g_counters.assignment.load();
  total += g_counters.add_assign.load();
  total += g_counters.sub_assign.load();
  total += g_counters.mul_assign.load();
  total += g_counters.div_assign.load();
  total += g_counters.mod_assign.load();
  total += g_counters.equal.load();
  total += g_counters.not_equal.load();
  total += g_counters.less_than.load();
  total += g_counters.greater_than.load();
  total += g_counters.less_equal.load();
  total += g_counters.greater_equal.load();

  return total;
}

double get_elapsed_seconds() {
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - g_start_time);
  return duration.count() / 1000000.0;
}

void print_statistics() {
  if (!g_stats_enabled) {
    return;
  }

  uint64_t total_ops = get_total_operations();
  double elapsed = get_elapsed_seconds();

  std::cout << "\n";
  std::cout << "╔══════════════════════════════════════════════════╗\n";
  std::cout << "║       OptiWeave Operation Statistics             ║\n";
  std::cout << "╚══════════════════════════════════════════════════╝\n";
  std::cout << "\n";

  std::cout << "Runtime: " << std::fixed << std::setprecision(3) << elapsed
            << " seconds\n";
  std::cout << "Total Operations: " << total_ops << "\n\n";

  if (total_ops == 0) {
    std::cout << "No operations tracked.\n";
    return;
  }

  // Collect all operations
  std::vector<OperationInfo> ops;

  auto add_op = [&](const std::string &name, uint64_t count) {
    if (count > 0) {
      double percentage = (count * 100.0) / total_ops;
      double ops_per_sec = elapsed > 0 ? count / elapsed : 0;
      ops.push_back({name, count, percentage, ops_per_sec});
    }
  };

  add_op("Array Subscripts", g_counters.array_subscript.load());
  add_op("Additions", g_counters.addition.load());
  add_op("Subtractions", g_counters.subtraction.load());
  add_op("Multiplications", g_counters.multiplication.load());
  add_op("Divisions", g_counters.division.load());
  add_op("Modulo", g_counters.modulo.load());
  add_op("Assignments", g_counters.assignment.load());
  add_op("Add-Assign (+=)", g_counters.add_assign.load());
  add_op("Sub-Assign (-=)", g_counters.sub_assign.load());
  add_op("Mul-Assign (*=)", g_counters.mul_assign.load());
  add_op("Div-Assign (/=)", g_counters.div_assign.load());
  add_op("Mod-Assign (%=)", g_counters.mod_assign.load());
  add_op("Equal (==)", g_counters.equal.load());
  add_op("Not Equal (!=)", g_counters.not_equal.load());
  add_op("Less Than (<)", g_counters.less_than.load());
  add_op("Greater Than (>)", g_counters.greater_than.load());
  add_op("Less/Equal (<=)", g_counters.less_equal.load());
  add_op("Greater/Equal (>=)", g_counters.greater_equal.load());

  // Sort by count (descending)
  std::sort(ops.begin(), ops.end(),
            [](const OperationInfo &a, const OperationInfo &b) {
              return a.count > b.count;
            });

  // Print table
  std::cout << "Operation Breakdown:\n";
  std::cout
      << "┌─────────────────────┬────────────┬──────────┬────────────────┐\n";
  std::cout
      << "│ Operation Type      │ Count      │ Percent  │ Ops/Second     │\n";
  std::cout
      << "├─────────────────────┼────────────┼──────────┼────────────────┤\n";

  for (const auto &op : ops) {
    std::cout << "│ " << std::left << std::setw(19) << op.name << " │ "
              << std::right << std::setw(10) << op.count << " │ " << std::right
              << std::setw(7) << std::fixed << std::setprecision(1)
              << op.percentage << "% │ ";

    // Format ops/second
    if (op.ops_per_second >= 1e9) {
      std::cout << std::setw(10) << std::fixed << std::setprecision(2)
                << (op.ops_per_second / 1e9) << "G ops/s";
    } else if (op.ops_per_second >= 1e6) {
      std::cout << std::setw(10) << std::fixed << std::setprecision(2)
                << (op.ops_per_second / 1e6) << "M ops/s";
    } else if (op.ops_per_second >= 1e3) {
      std::cout << std::setw(10) << std::fixed << std::setprecision(2)
                << (op.ops_per_second / 1e3) << "K ops/s";
    } else {
      std::cout << std::setw(10) << std::fixed << std::setprecision(0)
                << op.ops_per_second << "  ops/s";
    }

    std::cout << " │\n";
  }

  std::cout
      << "└─────────────────────┴────────────┴──────────┴────────────────┘\n";

  // Insights
  if (!ops.empty()) {
    std::cout << "\nInsights:\n";
    const auto &dominant = ops[0];
    std::cout << "• " << dominant.name << " dominate (" << std::fixed
              << std::setprecision(1) << dominant.percentage << "%)\n";

    // Find if there are divisions (typically slower)
    auto div_it =
        std::find_if(ops.begin(), ops.end(),
                     [](const OperationInfo &op) { return op.name == "Divisions"; });
    if (div_it != ops.end() && div_it->percentage > 5.0) {
      std::cout << "• High division count (" << std::setprecision(1)
                << div_it->percentage
                << "%) - consider optimization if critical\n";
    }

    // Arithmetic intensity
    uint64_t arithmetic_ops = g_counters.addition.load() +
                              g_counters.subtraction.load() +
                              g_counters.multiplication.load() +
                              g_counters.division.load();
    uint64_t array_ops = g_counters.array_subscript.load();

    if (arithmetic_ops > 0 && array_ops > 0) {
      double intensity = static_cast<double>(arithmetic_ops) / array_ops;
      if (intensity > 2.0) {
        std::cout << "• High arithmetic intensity (" << std::setprecision(2)
                  << intensity
                  << " arithmetic ops per memory access) - compute-bound\n";
      } else if (intensity < 0.5) {
        std::cout << "• Low arithmetic intensity (" << std::setprecision(2)
                  << intensity
                  << " arithmetic ops per memory access) - memory-bound\n";
      }
    }
  }

  std::cout << "\n";
}

void export_csv(const std::string &filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open CSV file: " << filename << "\n";
    return;
  }

  uint64_t total_ops = get_total_operations();
  double elapsed = get_elapsed_seconds();

  // Write header
  file << "Operation,Count,Percentage,OpsPerSecond\n";

  // Write data
  auto write_op = [&](const std::string &name, uint64_t count) {
    if (count > 0) {
      double percentage = (count * 100.0) / total_ops;
      double ops_per_sec = elapsed > 0 ? count / elapsed : 0;
      file << name << "," << count << "," << std::fixed << std::setprecision(2)
           << percentage << "," << std::setprecision(0) << ops_per_sec << "\n";
    }
  };

  write_op("ArraySubscript", g_counters.array_subscript.load());
  write_op("Addition", g_counters.addition.load());
  write_op("Subtraction", g_counters.subtraction.load());
  write_op("Multiplication", g_counters.multiplication.load());
  write_op("Division", g_counters.division.load());
  write_op("Modulo", g_counters.modulo.load());
  write_op("Assignment", g_counters.assignment.load());
  write_op("AddAssign", g_counters.add_assign.load());
  write_op("SubAssign", g_counters.sub_assign.load());
  write_op("MulAssign", g_counters.mul_assign.load());
  write_op("DivAssign", g_counters.div_assign.load());
  write_op("ModAssign", g_counters.mod_assign.load());
  write_op("Equal", g_counters.equal.load());
  write_op("NotEqual", g_counters.not_equal.load());
  write_op("LessThan", g_counters.less_than.load());
  write_op("GreaterThan", g_counters.greater_than.load());
  write_op("LessEqual", g_counters.less_equal.load());
  write_op("GreaterEqual", g_counters.greater_equal.load());

  file.close();
}

void export_json(const std::string &filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open JSON file: " << filename << "\n";
    return;
  }

  uint64_t total_ops = get_total_operations();
  double elapsed = get_elapsed_seconds();

  file << "{\n";
  file << "  \"runtime_seconds\": " << std::fixed << std::setprecision(6)
       << elapsed << ",\n";
  file << "  \"total_operations\": " << total_ops << ",\n";
  file << "  \"operations\": {\n";

  bool first = true;
  auto write_op = [&](const std::string &name, uint64_t count) {
    if (count > 0) {
      if (!first)
        file << ",\n";
      first = false;

      double percentage = (count * 100.0) / total_ops;
      double ops_per_sec = elapsed > 0 ? count / elapsed : 0;

      file << "    \"" << name << "\": {\n";
      file << "      \"count\": " << count << ",\n";
      file << "      \"percentage\": " << std::fixed << std::setprecision(2)
           << percentage << ",\n";
      file << "      \"ops_per_second\": " << std::setprecision(0) << ops_per_sec
           << "\n";
      file << "    }";
    }
  };

  write_op("array_subscript", g_counters.array_subscript.load());
  write_op("addition", g_counters.addition.load());
  write_op("subtraction", g_counters.subtraction.load());
  write_op("multiplication", g_counters.multiplication.load());
  write_op("division", g_counters.division.load());
  write_op("modulo", g_counters.modulo.load());
  write_op("assignment", g_counters.assignment.load());
  write_op("add_assign", g_counters.add_assign.load());
  write_op("sub_assign", g_counters.sub_assign.load());
  write_op("mul_assign", g_counters.mul_assign.load());
  write_op("div_assign", g_counters.div_assign.load());
  write_op("mod_assign", g_counters.mod_assign.load());
  write_op("equal", g_counters.equal.load());
  write_op("not_equal", g_counters.not_equal.load());
  write_op("less_than", g_counters.less_than.load());
  write_op("greater_than", g_counters.greater_than.load());
  write_op("less_equal", g_counters.less_equal.load());
  write_op("greater_equal", g_counters.greater_equal.load());

  file << "\n  }\n";
  file << "}\n";

  file.close();
}

} // namespace statistics
} // namespace optiweave
