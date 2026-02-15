#pragma once

#include <optiweave/runtime/json_parser.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace optiweave {
namespace analysis {

struct HotspotDelta {
    std::string function;
    std::string file;
    int line = 0;
    int64_t time_delta_ns = 0;       // negative = faster
    double time_delta_percent = 0.0;
    int64_t ops_delta = 0;
};

struct DiffResult {
    // Summary deltas
    int64_t runtime_delta_ns = 0;
    double runtime_delta_percent = 0.0;
    int64_t ops_delta = 0;
    double ops_delta_percent = 0.0;

    // Hotspot changes (sorted by |time_delta|)
    std::vector<HotspotDelta> changed_hotspots;
    std::vector<HotspotDelta> new_hotspots;       // in current only
    std::vector<HotspotDelta> resolved_hotspots;   // in baseline only

    // Suggestion changes
    std::vector<std::string> new_suggestions;
    std::vector<std::string> resolved_suggestions;
};

/// Compare two profiling dashboard JSONs and produce a delta report
DiffResult compute_diff(const json::ParsedDashboard& baseline,
                         const json::ParsedDashboard& current);

/// Format diff as colored terminal output
std::string format_diff_terminal(const DiffResult& diff);

/// Format diff as JSON
std::string format_diff_json(const DiffResult& diff);

/// Format diff as Markdown
std::string format_diff_markdown(const DiffResult& diff);

} // namespace analysis
} // namespace optiweave
