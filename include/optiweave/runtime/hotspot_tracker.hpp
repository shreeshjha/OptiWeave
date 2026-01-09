#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace optiweave {
namespace hotspots {

/**
 * @brief Represents a source code location
 */
struct SourceLocation {
  std::string file;
  int line;
  std::string function;

  SourceLocation(const char *f = "", int l = 0, const char *fn = "")
      : file(f ? f : ""), line(l), function(fn ? fn : "") {}

  bool operator==(const SourceLocation &other) const {
    return line == other.line && file == other.file && function == other.function;
  }

  std::string to_string() const {
    return file + ":" + std::to_string(line) +
           (function.empty() ? "" : " (" + function + ")");
  }
};

} // namespace hotspots
} // namespace optiweave

// Hash function for SourceLocation to use in unordered_map
namespace std {
template <> struct hash<optiweave::hotspots::SourceLocation> {
  size_t operator()(const optiweave::hotspots::SourceLocation &loc) const {
    size_t h1 = std::hash<std::string>{}(loc.file);
    size_t h2 = std::hash<int>{}(loc.line);
    size_t h3 = std::hash<std::string>{}(loc.function);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};
} // namespace std

namespace optiweave {
namespace hotspots {

/**
 * @brief Information about a performance hotspot
 */
struct HotspotInfo {
  SourceLocation location;
  uint64_t operation_count = 0;
  uint64_t total_time_ns = 0;
  std::unordered_map<std::string, uint64_t> operation_breakdown; // Changed to unordered_map

  double avg_time_ns() const {
    return operation_count > 0 ? static_cast<double>(total_time_ns) / operation_count : 0.0;
  }

  double percentage_of_total(uint64_t total_runtime_ns) const {
    return total_runtime_ns > 0
               ? 100.0 * static_cast<double>(total_time_ns) / total_runtime_ns
               : 0.0;
  }
};

/**
 * @brief Tracks performance hotspots by source location
 */
class HotspotTracker {
private:
  // Use pointer to avoid destruction order issues with atexit
  // Never deleted to ensure data persists until program exit
  std::unordered_map<SourceLocation, HotspotInfo>* hotspots_;
  mutable std::mutex mutex_;
  std::chrono::high_resolution_clock::time_point start_time_;
  uint64_t total_runtime_ns_ = 0;

public:
  HotspotTracker();

  /**
   * @brief Record an operation at a specific source location
   */
  void record_operation(const std::string &op_type, const SourceLocation &loc,
                        uint64_t duration_ns);

  /**
   * @brief Get top N hotspots sorted by total time
   */
  std::vector<HotspotInfo> get_top_hotspots(size_t n = 10) const;

  /**
   * @brief Get hotspots aggregated by function
   */
  std::map<std::string, HotspotInfo> get_hotspots_by_function() const;

  /**
   * @brief Get hotspots aggregated by file
   */
  std::map<std::string, HotspotInfo> get_hotspots_by_file() const;

  /**
   * @brief Get total runtime in nanoseconds
   */
  uint64_t get_total_runtime_ns() const;

  /**
   * @brief Get number of unique hotspot locations
   */
  size_t get_location_count() const;

  /**
   * @brief Print hotspot analysis to console
   */
  void print_hotspots(size_t top_n = 10) const;

  /**
   * @brief Export hotspot data to CSV
   */
  void export_csv(const std::string &filename) const;

  /**
   * @brief Export hotspot data to JSON
   */
  void export_json(const std::string &filename) const;

  /**
   * @brief Export hotspot data to flame graph format (folded stacks)
   * Compatible with flamegraph.pl and speedscope.app
   */
  void export_flamegraph(const std::string &filename) const;

  /**
   * @brief Export hotspot data to interactive HTML report
   */
  void export_html(const std::string &filename) const;

  /**
   * @brief Finalize tracking (stop timer)
   */
  void finalize();
};

// Global hotspot tracker instance
extern HotspotTracker g_hotspot_tracker;

// Runtime flags
extern bool g_hotspots_enabled;
extern size_t g_hotspots_top_n;
extern std::string g_hotspots_csv_file;
extern std::string g_hotspots_json_file;
extern std::string g_hotspots_flamegraph_file;
extern std::string g_hotspots_html_file;

/**
 * @brief Initialize hotspot tracking
 */
void initialize();

/**
 * @brief Finalize and print hotspot statistics (internal, called by atexit)
 */
void finalize();

/**
 * @brief Manually print hotspot report (call this before program exits)
 * This is the recommended way to get hotspot reports, as it avoids
 * potential corruption issues with atexit() handlers.
 */
void print_report(size_t top_n = 10);

} // namespace hotspots
} // namespace optiweave
