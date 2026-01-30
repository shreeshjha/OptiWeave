#include <optiweave/runtime/hotspot_tracker.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace optiweave {
namespace hotspots {

// Global instances
#ifdef OPTIWEAVE_ENABLE_HOTSPOTS
bool g_hotspots_enabled = false; // Start disabled, enable in initialize()
#else
bool g_hotspots_enabled = false;
#endif

HotspotTracker g_hotspot_tracker;

// Thread-local hotspot buffer (fast path - no locking)
thread_local ThreadLocalHotspotBuffer tl_hotspot_buffer;

size_t g_hotspots_top_n = 10;
std::string g_hotspots_csv_file;
std::string g_hotspots_json_file;
std::string g_hotspots_flamegraph_file;
std::string g_hotspots_html_file;

// Track if we're in finalization to avoid mutex issues
static std::atomic<bool> g_in_finalization{false};

// Track recording errors (for diagnostics)
static std::atomic<uint64_t> g_recording_errors{0};

// Flush thread-local hotspot buffer to global tracker
void flush_thread_local_hotspots() {
  if (tl_hotspot_buffer.hotspots.empty()) {
    return;
  }
  
  // Merge thread-local hotspots into global tracker
  g_hotspot_tracker.merge_thread_local_buffer(tl_hotspot_buffer);
}

// HotspotTracker implementation
HotspotTracker::HotspotTracker()
    : hotspots_(new std::unordered_map<SourceLocation, HotspotInfo>()),
      start_time_(std::chrono::high_resolution_clock::now()) {}

void HotspotTracker::record_operation(const std::string &op_type,
                                      const SourceLocation &loc,
                                      uint64_t duration_ns) {
  if (!g_hotspots_enabled) {
    return;
  }

  try {
    std::lock_guard<std::mutex> lock(mutex_);

    auto &info = (*hotspots_)[loc];
    info.location = loc;
    info.operation_count++;
    info.total_time_ns += duration_ns;
    info.operation_breakdown[op_type]++;
  } catch (const std::exception& e) {
    // Count errors but don't spam logs during high-frequency operations
    ++g_recording_errors;
  } catch (...) {
    // Count unknown errors
    ++g_recording_errors;
  }
}

std::vector<HotspotInfo>
HotspotTracker::get_top_hotspots(size_t n) const {
  // Only lock if not in finalization path (avoids mutex destruction issues)
  std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
  if (!g_in_finalization.load(std::memory_order_acquire)) {
    lock.lock();
  }

  std::vector<HotspotInfo> all_hotspots;
  all_hotspots.reserve(hotspots_->size());

  for (const auto &pair : *hotspots_) {
    all_hotspots.push_back(pair.second);
  }

  // Sort by total time (descending)
  std::sort(all_hotspots.begin(), all_hotspots.end(),
            [](const HotspotInfo &a, const HotspotInfo &b) {
              return a.total_time_ns > b.total_time_ns;
            });

  // Return top N
  if (all_hotspots.size() > n) {
    all_hotspots.resize(n);
  }

  return all_hotspots;
}

std::map<std::string, HotspotInfo>
HotspotTracker::get_hotspots_by_function() const {
  // Only lock if not in finalization path (avoids mutex destruction issues)
  std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
  if (!g_in_finalization.load(std::memory_order_acquire)) {
    lock.lock();
  }

  std::map<std::string, HotspotInfo> by_function;

  for (const auto &pair : *hotspots_) {
    const auto &info = pair.second;
    std::string func_name = info.location.function;

    auto &func_info = by_function[func_name];
    func_info.location.function = info.location.function;
    func_info.operation_count += info.operation_count;
    func_info.total_time_ns += info.total_time_ns;

    for (const auto &op : info.operation_breakdown) {
      func_info.operation_breakdown[op.first] += op.second;
    }
  }

  return by_function;
}

std::map<std::string, HotspotInfo>
HotspotTracker::get_hotspots_by_file() const {
  // Only lock if not in finalization path (avoids mutex destruction issues)
  std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
  if (!g_in_finalization.load(std::memory_order_acquire)) {
    lock.lock();
  }

  std::map<std::string, HotspotInfo> by_file;

  for (const auto &pair : *hotspots_) {
    const auto &info = pair.second;
    std::string file_name = info.location.file;

    auto &file_info = by_file[file_name];
    file_info.location.file = info.location.file;
    file_info.operation_count += info.operation_count;
    file_info.total_time_ns += info.total_time_ns;

    for (const auto &op : info.operation_breakdown) {
      file_info.operation_breakdown[op.first] += op.second;
    }
  }

  return by_file;
}

uint64_t HotspotTracker::get_total_runtime_ns() const {
  return total_runtime_ns_;
}

size_t HotspotTracker::get_location_count() const {
  // Only lock if not in finalization path (avoids mutex destruction issues)
  std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
  if (!g_in_finalization.load(std::memory_order_acquire)) {
    lock.lock();
  }
  return hotspots_->size();
}

void HotspotTracker::finalize() {
  auto end_time = std::chrono::high_resolution_clock::now();
  total_runtime_ns_ =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end_time -
                                                            start_time_)
          .count();
}

void HotspotTracker::merge_thread_local_buffer(ThreadLocalHotspotBuffer& buffer) {
  if (buffer.hotspots.empty()) {
    return;
  }
  
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : buffer.hotspots) {
      const auto& loc = pair.first;
      auto& local_info = pair.second;
      
      // Merge into global hotspots
      auto& global_info = (*hotspots_)[loc];
      global_info.location = loc;
      global_info.operation_count += local_info.operation_count;
      global_info.total_time_ns += local_info.total_time_ns;
      
      // Merge operation breakdown
      for (const auto& op : local_info.operation_breakdown) {
        global_info.operation_breakdown[op.first] += op.second;
      }
    }
    
    // Clear thread-local buffer after successful flush
    buffer.hotspots.clear();
  } catch (...) {
    ++g_recording_errors;
  }
}

// Helper function to format time
static std::string format_time(uint64_t ns) {
  if (ns < 1000) {
    return std::to_string(ns) + "ns";
  } else if (ns < 1000000) {
    return std::to_string(ns / 1000) + "." +
           std::to_string((ns % 1000) / 100) + "μs";
  } else if (ns < 1000000000) {
    return std::to_string(ns / 1000000) + "." +
           std::to_string((ns % 1000000) / 100000) + "ms";
  } else {
    return std::to_string(ns / 1000000000) + "." +
           std::to_string((ns % 1000000000) / 100000000) + "s";
  }
}

// Helper function to format large numbers
static std::string format_count(uint64_t count) {
  if (count < 1000) {
    return std::to_string(count);
  } else if (count < 1000000) {
    return std::to_string(count / 1000) + "." +
           std::to_string((count % 1000) / 100) + "K";
  } else if (count < 1000000000) {
    return std::to_string(count / 1000000) + "." +
           std::to_string((count % 1000000) / 100) + "M";
  } else {
    return std::to_string(count / 1000000000) + "." +
           std::to_string((count % 1000000000) / 100) + "B";
  }
}

void HotspotTracker::print_hotspots(size_t top_n) const {
  if (!g_hotspots_enabled) {
    return;
  }

  auto hotspots = get_top_hotspots(top_n);

  if (hotspots.empty()) {
    std::cout << "\nNo hotspots tracked.\n";
    return;
  }

  std::cout << "\n";
  std::cout
      << "╔══════════════════════════════════════════════════════════════╗\n";
  std::cout
      << "║            OptiWeave Hotspot Analysis                        ║\n";
  std::cout
      << "╚══════════════════════════════════════════════════════════════╝\n";
  std::cout << "\n";

  std::cout << "Total Runtime: " << format_time(total_runtime_ns_) << "\n";
  std::cout << "Locations Tracked: " << get_location_count() << "\n";
  std::cout << "\n";

  std::cout << "Top " << hotspots.size() << " Hotspots (by time spent):\n";
  std::cout << "┌────┬───────────────────────────────────────────────────┬──────"
               "────┬──────────┬─────────┬──────────┐\n";
  std::cout << "│ #  │ Location                                          │ Ops  "
               "    │ Time     │ % Total │ Avg/Op   │\n";
  std::cout << "├────┼───────────────────────────────────────────────────┼──────"
               "────┼──────────┼─────────┼──────────┤\n";

  for (size_t i = 0; i < hotspots.size(); ++i) {
    const auto &hs = hotspots[i];
    std::cout << "│ " << std::setw(2) << (i + 1) << " │ " << std::setw(49)
              << std::left << hs.location.to_string() << " │ " << std::setw(8)
              << std::right << format_count(hs.operation_count) << " │ "
              << std::setw(8) << format_time(hs.total_time_ns) << " │ "
              << std::setw(6) << std::fixed << std::setprecision(1)
              << hs.percentage_of_total(total_runtime_ns_) << "% │ "
              << std::setw(8) << format_time(static_cast<uint64_t>(hs.avg_time_ns())) << " │\n";
  }

  std::cout << "└────┴───────────────────────────────────────────────────┴──────"
               "────┴──────────┴─────────┴──────────┘\n";
  std::cout << "\n";

  // Hotspot visualization
  std::cout << "Hotspot Visualization:\n";
  for (size_t i = 0; i < std::min(hotspots.size(), size_t(10)); ++i) {
    const auto &hs = hotspots[i];
    double pct = hs.percentage_of_total(total_runtime_ns_);
    int bar_length = static_cast<int>(pct / 2); // Scale to fit console

    std::cout << "  " << std::setw(5) << std::fixed << std::setprecision(1)
              << pct << "% ";
    for (int j = 0; j < bar_length; ++j) {
      std::cout << "█";
    }
    std::cout << " " << hs.location.function << " (" << hs.location.file << ":"
              << hs.location.line << ")\n";
  }
  std::cout << "\n";

  // Function-level aggregation
  auto by_function = get_hotspots_by_function();
  if (!by_function.empty()) {
    std::cout << "Function-Level Hotspots:\n";
    std::cout
        << "┌────────────────────────────────────────┬──────────┬─────────┐\n";
    std::cout
        << "│ Function                               │ Time     │ % Total │\n";
    std::cout
        << "├────────────────────────────────────────┼──────────┼─────────┤\n";

    // Sort by time
    std::vector<std::pair<std::string, HotspotInfo>> func_vec(
        by_function.begin(), by_function.end());
    std::sort(func_vec.begin(), func_vec.end(),
              [](const auto &a, const auto &b) {
                return a.second.total_time_ns > b.second.total_time_ns;
              });

    for (size_t i = 0; i < std::min(func_vec.size(), size_t(10)); ++i) {
      const auto &func = func_vec[i];
      std::cout << "│ " << std::setw(38) << std::left << func.first << " │ "
                << std::setw(8) << std::right
                << format_time(func.second.total_time_ns) << " │ "
                << std::setw(6) << std::fixed << std::setprecision(1)
                << func.second.percentage_of_total(total_runtime_ns_) << "% │\n";
    }

    std::cout
        << "└────────────────────────────────────────┴──────────┴─────────┘\n";
    std::cout << "\n";
  }

  // File-level aggregation
  auto by_file = get_hotspots_by_file();
  if (!by_file.empty()) {
    std::cout << "File-Level Hotspots:\n";
    std::cout
        << "┌────────────────────────────────────────┬──────────┬─────────┐\n";
    std::cout
        << "│ File                                   │ Time     │ % Total │\n";
    std::cout
        << "├────────────────────────────────────────┼──────────┼─────────┤\n";

    // Sort by time
    std::vector<std::pair<std::string, HotspotInfo>> file_vec(by_file.begin(),
                                                               by_file.end());
    std::sort(file_vec.begin(), file_vec.end(),
              [](const auto &a, const auto &b) {
                return a.second.total_time_ns > b.second.total_time_ns;
              });

    for (size_t i = 0; i < std::min(file_vec.size(), size_t(10)); ++i) {
      const auto &file = file_vec[i];
      std::cout << "│ " << std::setw(38) << std::left << file.first << " │ "
                << std::setw(8) << std::right
                << format_time(file.second.total_time_ns) << " │ "
                << std::setw(6) << std::fixed << std::setprecision(1)
                << file.second.percentage_of_total(total_runtime_ns_) << "% │\n";
    }

    std::cout
        << "└────────────────────────────────────────┴──────────┴─────────┘\n";
    std::cout << "\n";
  }
}

void HotspotTracker::export_csv(const std::string &filename) const {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Failed to open " << filename << " for writing\n";
    return;
  }

  file << "Rank,File,Line,Function,Operations,Time_ns,Percent,Avg_ns\n";

  auto hotspots = get_top_hotspots(1000); // Export more than we display

  for (size_t i = 0; i < hotspots.size(); ++i) {
    const auto &hs = hotspots[i];
    file << (i + 1) << "," << hs.location.file << "," << hs.location.line
         << "," << hs.location.function << "," << hs.operation_count << ","
         << hs.total_time_ns << ","
         << hs.percentage_of_total(total_runtime_ns_) << ","
         << hs.avg_time_ns() << "\n";
  }

  file.close();
  std::cout << "Hotspot data exported to " << filename << "\n";
}

void HotspotTracker::export_json(const std::string &filename) const {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Failed to open " << filename << " for writing\n";
    return;
  }

  auto hotspots = get_top_hotspots(1000);

  file << "{\n";
  file << "  \"total_runtime_ns\": " << total_runtime_ns_ << ",\n";
  file << "  \"locations_tracked\": " << get_location_count() << ",\n";
  file << "  \"hotspots\": [\n";

  for (size_t i = 0; i < hotspots.size(); ++i) {
    const auto &hs = hotspots[i];
    file << "    {\n";
    file << "      \"rank\": " << (i + 1) << ",\n";
    file << "      \"file\": \"" << hs.location.file << "\",\n";
    file << "      \"line\": " << hs.location.line << ",\n";
    file << "      \"function\": \"" << hs.location.function << "\",\n";
    file << "      \"operation_count\": " << hs.operation_count << ",\n";
    file << "      \"total_time_ns\": " << hs.total_time_ns << ",\n";
    file << "      \"percentage\": "
         << hs.percentage_of_total(total_runtime_ns_) << ",\n";
    file << "      \"avg_time_ns\": " << hs.avg_time_ns() << "\n";
    file << "    }";
    if (i < hotspots.size() - 1) {
      file << ",";
    }
    file << "\n";
  }

  file << "  ]\n";
  file << "}\n";

  file.close();
  std::cout << "Hotspot data exported to " << filename << "\n";
}

void HotspotTracker::export_flamegraph(const std::string &filename) const {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Failed to open " << filename << " for writing\n";
    return;
  }

  // Flame graph format: one line per stack, with sample count at the end
  // Format: func1;func2;func3 <sample_count>
  // For hotspots, we create simplified "stacks" from source locations

  auto hotspots = get_top_hotspots(1000); // Export more data for flame graph

  for (const auto &hs : hotspots) {
    // Create a pseudo-stack: file;function;line
    // This works well for visualizing which files/functions dominate
    std::string stack = hs.location.file + ";" + hs.location.function;

    // For flame graphs, we need sample counts, not time
    // Convert time to "samples" (nanoseconds work as samples)
    file << stack << " " << hs.total_time_ns << "\n";
  }

  file.close();
  std::cout << "Flame graph data exported to " << filename << "\n";
  std::cout << "View with: flamegraph.pl " << filename << " > flamegraph.svg\n";
  std::cout << "Or upload to https://www.speedscope.app/\n";
}

void HotspotTracker::export_html(const std::string &filename) const {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Failed to open " << filename << " for writing\n";
    return;
  }

  auto hotspots = get_top_hotspots(100);
  auto by_function = get_hotspots_by_function();
  auto by_file = get_hotspots_by_file();

  // Generate HTML report
  file << R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>OptiWeave Hotspot Analysis Report</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: #f5f5f5;
      color: #333;
      line-height: 1.6;
    }
    .container {
      max-width: 1400px;
      margin: 0 auto;
      padding: 20px;
    }
    header {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      padding: 30px;
      border-radius: 10px;
      margin-bottom: 30px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.1);
    }
    header h1 { font-size: 2.5em; margin-bottom: 10px; }
    header p { font-size: 1.1em; opacity: 0.9; }
    .summary {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
      gap: 20px;
      margin-bottom: 30px;
    }
    .stat-card {
      background: white;
      padding: 25px;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      border-left: 4px solid #667eea;
    }
    .stat-card h3 { color: #667eea; margin-bottom: 10px; font-size: 0.9em; text-transform: uppercase; }
    .stat-card .value { font-size: 2.2em; font-weight: bold; color: #333; }
    .section {
      background: white;
      padding: 30px;
      border-radius: 8px;
      margin-bottom: 20px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .section h2 { color: #667eea; margin-bottom: 20px; padding-bottom: 10px; border-bottom: 2px solid #f0f0f0; }
    table {
      width: 100%;
      border-collapse: collapse;
      margin-top: 15px;
    }
    thead { background: #f8f9fa; }
    th, td {
      padding: 12px;
      text-align: left;
      border-bottom: 1px solid #e9ecef;
    }
    th { font-weight: 600; color: #495057; text-transform: uppercase; font-size: 0.85em; }
    tr:hover { background: #f8f9fa; }
    .hotspot-critical { background: #fff5f5; }
    .hotspot-warning { background: #fffbf0; }
    .bar {
      background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
      height: 20px;
      border-radius: 3px;
      display: inline-block;
      min-width: 2px;
    }
    .bar-container {
      background: #e9ecef;
      height: 20px;
      border-radius: 3px;
      overflow: hidden;
      width: 200px;
      display: inline-block;
    }
    .location { font-family: 'Consolas', 'Monaco', monospace; font-size: 0.9em; color: #495057; }
    .percentage { font-weight: bold; color: #667eea; }
    .rank {
      background: #667eea;
      color: white;
      padding: 4px 10px;
      border-radius: 12px;
      font-weight: bold;
      font-size: 0.85em;
    }
    .rank.top-3 { background: #e63946; }
    footer {
      text-align: center;
      padding: 20px;
      color: #6c757d;
      font-size: 0.9em;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>🔥 OptiWeave Hotspot Analysis</h1>
      <p>Performance bottleneck detection and analysis report</p>
    </header>

    <div class="summary">
      <div class="stat-card">
        <h3>Total Runtime</h3>
        <div class="value">)";

  file << format_time(total_runtime_ns_) << R"(</div>
      </div>
      <div class="stat-card">
        <h3>Locations Tracked</h3>
        <div class="value">)" << get_location_count() << R"(</div>
      </div>
      <div class="stat-card">
        <h3>Top Hotspot</h3>
        <div class="value">)";

  if (!hotspots.empty()) {
    file << std::fixed << std::setprecision(1)
         << hotspots[0].percentage_of_total(total_runtime_ns_) << "%";
  } else {
    file << "N/A";
  }

  file << R"(</div>
      </div>
      <div class="stat-card">
        <h3>Files Analyzed</h3>
        <div class="value">)" << by_file.size() << R"(</div>
      </div>
    </div>

    <div class="section">
      <h2>Top 20 Hotspots by Time</h2>
      <table>
        <thead>
          <tr>
            <th>Rank</th>
            <th>Location</th>
            <th>Operations</th>
            <th>Time</th>
            <th>% Total</th>
            <th>Avg/Op</th>
            <th>Distribution</th>
          </tr>
        </thead>
        <tbody>
)";

  size_t rank = 1;
  for (const auto &hs : hotspots) {
    if (rank > 20) break;

    double pct = hs.percentage_of_total(total_runtime_ns_);
    std::string row_class = "";
    if (pct > 20.0) row_class = " class=\"hotspot-critical\"";
    else if (pct > 10.0) row_class = " class=\"hotspot-warning\"";

    file << "          <tr" << row_class << ">\n";
    file << "            <td><span class=\"rank" << (rank <= 3 ? " top-3" : "")
         << "\">" << rank << "</span></td>\n";
    file << "            <td class=\"location\">" << hs.location.file << ":"
         << hs.location.line << "<br><small>" << hs.location.function
         << "</small></td>\n";
    file << "            <td>" << format_count(hs.operation_count) << "</td>\n";
    file << "            <td>" << format_time(hs.total_time_ns) << "</td>\n";
    file << "            <td class=\"percentage\">" << std::fixed << std::setprecision(1)
         << pct << "%</td>\n";
    file << "            <td>" << format_time(static_cast<uint64_t>(hs.avg_time_ns()))
         << "</td>\n";
    file << "            <td><div class=\"bar-container\"><div class=\"bar\" style=\"width: "
         << std::min(100.0, pct * 2.0) << "%\"></div></div></td>\n";
    file << "          </tr>\n";

    rank++;
  }

  file << R"(        </tbody>
      </table>
    </div>

    <div class="section">
      <h2>Hotspots by Function</h2>
      <table>
        <thead>
          <tr>
            <th>Function</th>
            <th>Time</th>
            <th>% Total</th>
            <th>Distribution</th>
          </tr>
        </thead>
        <tbody>
)";

  // Sort by time
  std::vector<std::pair<std::string, HotspotInfo>> func_vec(
      by_function.begin(), by_function.end());
  std::sort(func_vec.begin(), func_vec.end(),
            [](const auto &a, const auto &b) {
              return a.second.total_time_ns > b.second.total_time_ns;
            });

  for (size_t i = 0; i < std::min(func_vec.size(), size_t(15)); ++i) {
    const auto &func = func_vec[i];
    double pct = func.second.percentage_of_total(total_runtime_ns_);

    file << "          <tr>\n";
    file << "            <td class=\"location\">" << func.first << "</td>\n";
    file << "            <td>" << format_time(func.second.total_time_ns) << "</td>\n";
    file << "            <td class=\"percentage\">" << std::fixed << std::setprecision(1)
         << pct << "%</td>\n";
    file << "            <td><div class=\"bar-container\"><div class=\"bar\" style=\"width: "
         << std::min(100.0, pct * 2.0) << "%\"></div></div></td>\n";
    file << "          </tr>\n";
  }

  file << R"(        </tbody>
      </table>
    </div>

    <div class="section">
      <h2>Hotspots by File</h2>
      <table>
        <thead>
          <tr>
            <th>File</th>
            <th>Time</th>
            <th>% Total</th>
            <th>Distribution</th>
          </tr>
        </thead>
        <tbody>
)";

  // Sort by time
  std::vector<std::pair<std::string, HotspotInfo>> file_vec(by_file.begin(),
                                                              by_file.end());
  std::sort(file_vec.begin(), file_vec.end(),
            [](const auto &a, const auto &b) {
              return a.second.total_time_ns > b.second.total_time_ns;
            });

  for (size_t i = 0; i < std::min(file_vec.size(), size_t(15)); ++i) {
    const auto &f = file_vec[i];
    double pct = f.second.percentage_of_total(total_runtime_ns_);

    file << "          <tr>\n";
    file << "            <td class=\"location\">" << f.first << "</td>\n";
    file << "            <td>" << format_time(f.second.total_time_ns) << "</td>\n";
    file << "            <td class=\"percentage\">" << std::fixed << std::setprecision(1)
         << pct << "%</td>\n";
    file << "            <td><div class=\"bar-container\"><div class=\"bar\" style=\"width: "
         << std::min(100.0, pct * 2.0) << "%\"></div></div></td>\n";
    file << "          </tr>\n";
  }

  file << R"(        </tbody>
      </table>
    </div>

    <footer>
      <p>Generated by OptiWeave Profiler | <a href="https://github.com/OptiWeave">github.com/OptiWeave</a></p>
    </footer>
  </div>
</body>
</html>
)";

  file.close();
  std::cout << "HTML report exported to " << filename << "\n";
  std::cout << "Open in browser: open " << filename << "\n";
}

// Initialization and finalization
void initialize() {
  // Check environment variables
  const char *env_csv = std::getenv("OPTIWEAVE_HOTSPOTS_CSV");
  if (env_csv) {
    g_hotspots_csv_file = env_csv;
  }

  const char *env_json = std::getenv("OPTIWEAVE_HOTSPOTS_JSON");
  if (env_json) {
    g_hotspots_json_file = env_json;
  }

  const char *env_flamegraph = std::getenv("OPTIWEAVE_HOTSPOTS_FLAMEGRAPH");
  if (env_flamegraph) {
    g_hotspots_flamegraph_file = env_flamegraph;
  }

  const char *env_html = std::getenv("OPTIWEAVE_HOTSPOTS_HTML");
  if (env_html) {
    g_hotspots_html_file = env_html;
  }

  const char *env_top_n = std::getenv("OPTIWEAVE_HOTSPOTS_TOP_N");
  if (env_top_n) {
    g_hotspots_top_n = std::atoi(env_top_n);
  }

  // Enable hotspots
  g_hotspots_enabled = true;

  // Note: We still register atexit for convenience, but recommend manual reporting
  std::atexit(finalize);
}

void print_report(size_t top_n) {
  if (!g_hotspots_enabled) {
    return;
  }

  // Flush thread-local hotspots to global tracker before reporting
  // Note: In multi-threaded programs, each thread should call flush before exit
  flush_thread_local_hotspots();

  // Finalize timing first
  g_hotspot_tracker.finalize();

  // Print the report
  g_hotspot_tracker.print_hotspots(top_n);

  // Export if requested
  if (!g_hotspots_csv_file.empty()) {
    g_hotspot_tracker.export_csv(g_hotspots_csv_file);
  }

  if (!g_hotspots_json_file.empty()) {
    g_hotspot_tracker.export_json(g_hotspots_json_file);
  }

  if (!g_hotspots_flamegraph_file.empty()) {
    g_hotspot_tracker.export_flamegraph(g_hotspots_flamegraph_file);
  }

  if (!g_hotspots_html_file.empty()) {
    g_hotspot_tracker.export_html(g_hotspots_html_file);
  }
}

void finalize() {
  // Mark that we're in finalization to avoid mutex issues in const methods
  g_in_finalization.store(true, std::memory_order_release);

  // Call print_report for automatic reporting at exit
  // Note: This may experience data corruption due to atexit() ordering issues
  // Users should prefer calling print_report() manually before main() returns
  print_report(g_hotspots_top_n);

  // Report any recording errors that occurred
  uint64_t errors = g_recording_errors.load();
  if (errors > 0) {
    std::cerr << "Note: " << errors << " hotspot recording error(s) occurred during execution\n";
  }
}

} // namespace hotspots
} // namespace optiweave
