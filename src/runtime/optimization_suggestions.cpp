#include <optiweave/runtime/optimization_suggestions.hpp>
#include <optiweave/runtime/hotspot_tracker.hpp>
#include <optiweave/runtime/statistics.hpp>
#include <optiweave/runtime/loop_info_serializer.hpp>
#include <optiweave/runtime/json_exporter.hpp>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

namespace optiweave {
namespace optimization {

// Global state
bool g_suggestions_enabled = false;
static analysis::OptimizationAnalyzer* g_analyzer = nullptr;

// Use function-local static to avoid static initialization order issues
static std::vector<analysis::LoopInfo>& get_loop_info() {
  static std::vector<analysis::LoopInfo> loop_info;
  return loop_info;
}

static size_t g_loop_info_loaded_count = 0;  // DEBUG

void initialize() {
  // Check environment variable
  const char* env = std::getenv("OPTIWEAVE_SUGGESTIONS");
  if (env && (std::strcmp(env, "1") == 0 || std::strcmp(env, "true") == 0)) {
    g_suggestions_enabled = true;
  }

  if (g_suggestions_enabled) {
    g_analyzer = new analysis::OptimizationAnalyzer();

    // Load loop information from serialized file
    std::string loop_info_file = serialization::get_loop_info_path();
    auto& loop_info = get_loop_info();
    if (serialization::deserialize_loop_info(loop_info_file, loop_info)) {
      g_loop_info_loaded_count = loop_info.size();  // DEBUG
      std::cerr << "Loaded loop information: " << loop_info.size() << " loops from "
                << loop_info_file << " (loop_info addr: " << &loop_info << ")\n";
    } else {
      std::cerr << "Failed to load loop information from " << loop_info_file << "\n";
    }

    std::atexit(finalize);
  }
}

void finalize() {
  if (!g_suggestions_enabled || !g_analyzer) {
    return;
  }

  // Get hotspot and statistics data
  auto& hotspot_tracker = hotspots::g_hotspot_tracker;
  auto& stats = statistics::g_counters;

  // Correlate loop info with hotspot data
  auto hotspots_data = hotspot_tracker.get_top_hotspots(1000);  // Get all hotspots
  auto& loop_info = get_loop_info();

  // Correlate loop bodies with hotspots
  for (auto& loop : loop_info) {
    // Accumulate all hotspots within this loop's body
    uint64_t total_time = 0;
    uint64_t total_ops = 0;

    for (const auto& hotspot : hotspots_data) {
      if (hotspot.location.file == loop.location.file &&
          loop.contains_line(hotspot.location.line)) {
        total_time += hotspot.total_time_ns;
        total_ops += hotspot.operation_count;
      }
    }

    if (total_time > 0) {
      loop.total_time_ns = total_time;
      loop.iteration_count = total_ops;
    }
  }

  // Run analysis
  auto result = g_analyzer->analyze(hotspot_tracker, stats, loop_info);

  // Check if JSON export is enabled
  const char* json_export_env = std::getenv("OPTIWEAVE_JSON_EXPORT");
  if (json_export_env && (std::strcmp(json_export_env, "1") == 0 || std::strcmp(json_export_env, "true") == 0)) {
    // Export complete dashboard data
    std::string json = json::export_dashboard_json(hotspot_tracker, stats, result);

    const char* json_file_env = std::getenv("OPTIWEAVE_JSON_FILE");
    std::string json_filename = json_file_env ? json_file_env : "optiweave_dashboard.json";

    if (json::write_json_file(json, json_filename)) {
      std::cerr << "\n✅ Dashboard data exported to: " << json_filename << "\n";
    } else {
      std::cerr << "\n❌ Failed to export JSON to: " << json_filename << "\n";
    }
  }

  // Check output format environment variable
  const char* format_env = std::getenv("OPTIWEAVE_SUGGESTIONS_FORMAT");
  std::string format = format_env ? format_env : "terminal";

  const char* output_file_env = std::getenv("OPTIWEAVE_SUGGESTIONS_FILE");

  if (output_file_env) {
    // Export to file
    g_analyzer->export_to_file(result, output_file_env, format);
    std::cerr << "\n✅ Optimization suggestions exported to: " << output_file_env << "\n";
  } else {
    // Print to terminal
    std::string output = g_analyzer->generate_terminal_output(result);
    std::cerr << output;
  }

  // Cleanup
  delete g_analyzer;
  g_analyzer = nullptr;
}

analysis::OptimizationAnalyzer& get_analyzer() {
  if (!g_analyzer) {
    g_analyzer = new analysis::OptimizationAnalyzer();
  }
  return *g_analyzer;
}

// Helper function to set loop info from transformer
// This would be called during transformation (not implemented yet)
void set_loop_info(const std::vector<analysis::LoopInfo>& loop_info_arg) {
  get_loop_info() = loop_info_arg;
}

} // namespace optimization
} // namespace optiweave
