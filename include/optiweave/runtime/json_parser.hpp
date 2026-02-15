#pragma once

#include <optiweave/analysis/optimization_pattern.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace optiweave {
namespace json {

struct ParsedHotspot {
    std::string name;       // function name
    std::string file;
    int line = 0;
    uint64_t time_ns = 0;
    double time_ms = 0.0;
    double percent = 0.0;
    uint64_t operations = 0;
};

struct ParsedOperation {
    std::string type;
    uint64_t count = 0;
    double percent = 0.0;
};

struct ParsedSummary {
    uint64_t total_operations = 0;
    uint64_t total_runtime_ns = 0;
    double total_runtime_ms = 0.0;
    size_t issues_found = 0;
    std::string potential_speedup;
};

struct ParsedDashboard {
    ParsedSummary summary;
    std::vector<ParsedHotspot> hotspots;
    std::vector<ParsedOperation> operations;
    std::vector<analysis::OptimizationPattern> suggestions;
};

/// Parse a dashboard JSON file produced by export_dashboard_json()
ParsedDashboard parse_dashboard_json(const std::string& filepath);

} // namespace json
} // namespace optiweave
