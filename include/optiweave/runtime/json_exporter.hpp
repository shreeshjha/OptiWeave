#pragma once

#include <optiweave/analysis/optimization_pattern.hpp>
#include <optiweave/runtime/hotspot_tracker.hpp>
#include <optiweave/runtime/statistics.hpp>
#include <string>

namespace optiweave {
namespace json {

/// Export hotspots to JSON format
std::string export_hotspots_json(const hotspots::HotspotTracker& tracker);

/// Export operation statistics to JSON format
std::string export_operations_json(const statistics::OperationCounters& stats);

/// Export optimization suggestions to JSON format
std::string export_suggestions_json(const analysis::AnalysisResult& result);

/// Export complete dashboard data to JSON
std::string export_dashboard_json(
    const hotspots::HotspotTracker& tracker,
    const statistics::OperationCounters& stats,
    const analysis::AnalysisResult& result
);

/// Write JSON to file
bool write_json_file(const std::string& json, const std::string& filename);

/// Helper to escape JSON strings
std::string escape_json_string(const std::string& str);

} // namespace json
} // namespace optiweave
