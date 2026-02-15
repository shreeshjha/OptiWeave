#include <optiweave/analysis/diff_profile.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace optiweave {
namespace analysis {

namespace {

// Key for matching hotspots: (file, line, function)
struct HotspotKey {
    std::string file;
    int line;
    std::string function;

    bool operator==(const HotspotKey& o) const {
        return file == o.file && line == o.line && function == o.function;
    }
};

struct HotspotKeyHash {
    size_t operator()(const HotspotKey& k) const {
        size_t h1 = std::hash<std::string>{}(k.file);
        size_t h2 = std::hash<int>{}(k.line);
        size_t h3 = std::hash<std::string>{}(k.function);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

// Key for matching suggestions: (file, line, pattern_name)
struct SuggestionKey {
    std::string file;
    int line;
    std::string pattern_name;

    bool operator==(const SuggestionKey& o) const {
        return file == o.file && line == o.line && pattern_name == o.pattern_name;
    }
};

struct SuggestionKeyHash {
    size_t operator()(const SuggestionKey& k) const {
        size_t h1 = std::hash<std::string>{}(k.file);
        size_t h2 = std::hash<int>{}(k.line);
        size_t h3 = std::hash<std::string>{}(k.pattern_name);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

std::string suggestion_description(const OptimizationPattern& pat) {
    return pat.pattern_name + " at " + pat.location.file + ":" +
           std::to_string(pat.location.line) + " (" + pat.location.function + ")";
}

} // anonymous namespace

DiffResult compute_diff(const json::ParsedDashboard& baseline,
                         const json::ParsedDashboard& current) {
    DiffResult result;

    // Summary deltas
    result.runtime_delta_ns = static_cast<int64_t>(current.summary.total_runtime_ns) -
                               static_cast<int64_t>(baseline.summary.total_runtime_ns);
    if (baseline.summary.total_runtime_ns > 0) {
        result.runtime_delta_percent = 100.0 * result.runtime_delta_ns /
                                        static_cast<double>(baseline.summary.total_runtime_ns);
    }

    result.ops_delta = static_cast<int64_t>(current.summary.total_operations) -
                        static_cast<int64_t>(baseline.summary.total_operations);
    if (baseline.summary.total_operations > 0) {
        result.ops_delta_percent = 100.0 * result.ops_delta /
                                    static_cast<double>(baseline.summary.total_operations);
    }

    // Match hotspots by (file, line, function)
    std::unordered_map<HotspotKey, const json::ParsedHotspot*, HotspotKeyHash> baseline_map;
    for (const auto& h : baseline.hotspots) {
        baseline_map[{h.file, h.line, h.name}] = &h;
    }

    std::unordered_map<HotspotKey, bool, HotspotKeyHash> matched_baseline;

    for (const auto& h : current.hotspots) {
        HotspotKey key{h.file, h.line, h.name};
        auto it = baseline_map.find(key);

        HotspotDelta delta;
        delta.function = h.name;
        delta.file = h.file;
        delta.line = h.line;

        if (it != baseline_map.end()) {
            // Matched — compute delta
            const auto* bh = it->second;
            delta.time_delta_ns = static_cast<int64_t>(h.time_ns) -
                                   static_cast<int64_t>(bh->time_ns);
            if (bh->time_ns > 0) {
                delta.time_delta_percent = 100.0 * delta.time_delta_ns /
                                            static_cast<double>(bh->time_ns);
            }
            delta.ops_delta = static_cast<int64_t>(h.operations) -
                               static_cast<int64_t>(bh->operations);
            result.changed_hotspots.push_back(delta);
            matched_baseline[key] = true;
        } else {
            // New hotspot
            delta.time_delta_ns = static_cast<int64_t>(h.time_ns);
            delta.ops_delta = static_cast<int64_t>(h.operations);
            result.new_hotspots.push_back(delta);
        }
    }

    // Resolved hotspots (in baseline but not current)
    for (const auto& h : baseline.hotspots) {
        HotspotKey key{h.file, h.line, h.name};
        if (matched_baseline.find(key) == matched_baseline.end()) {
            HotspotDelta delta;
            delta.function = h.name;
            delta.file = h.file;
            delta.line = h.line;
            delta.time_delta_ns = -static_cast<int64_t>(h.time_ns);
            delta.ops_delta = -static_cast<int64_t>(h.operations);
            result.resolved_hotspots.push_back(delta);
        }
    }

    // Sort changed hotspots by |time_delta| descending
    std::sort(result.changed_hotspots.begin(), result.changed_hotspots.end(),
              [](const HotspotDelta& a, const HotspotDelta& b) {
                  return std::abs(a.time_delta_ns) > std::abs(b.time_delta_ns);
              });

    // Match suggestions by (file, line, pattern_name)
    std::unordered_map<SuggestionKey, bool, SuggestionKeyHash> baseline_suggestions;
    for (const auto& s : baseline.suggestions) {
        baseline_suggestions[{s.location.file, s.location.line, s.pattern_name}] = true;
    }

    std::unordered_map<SuggestionKey, bool, SuggestionKeyHash> current_suggestions;
    for (const auto& s : current.suggestions) {
        SuggestionKey key{s.location.file, s.location.line, s.pattern_name};
        current_suggestions[key] = true;
        if (baseline_suggestions.find(key) == baseline_suggestions.end()) {
            result.new_suggestions.push_back(suggestion_description(s));
        }
    }

    for (const auto& s : baseline.suggestions) {
        SuggestionKey key{s.location.file, s.location.line, s.pattern_name};
        if (current_suggestions.find(key) == current_suggestions.end()) {
            result.resolved_suggestions.push_back(suggestion_description(s));
        }
    }

    return result;
}

// Helper to format nanoseconds as human-readable
static std::string format_time(int64_t ns) {
    std::ostringstream oss;
    if (std::abs(ns) >= 1000000000) {
        oss << std::fixed << std::setprecision(3) << (ns / 1000000000.0) << "s";
    } else if (std::abs(ns) >= 1000000) {
        oss << std::fixed << std::setprecision(3) << (ns / 1000000.0) << "ms";
    } else if (std::abs(ns) >= 1000) {
        oss << std::fixed << std::setprecision(1) << (ns / 1000.0) << "us";
    } else {
        oss << ns << "ns";
    }
    return oss.str();
}

static std::string sign_prefix(int64_t val) {
    return val > 0 ? "+" : "";
}

std::string format_diff_terminal(const DiffResult& diff) {
    std::ostringstream oss;

    oss << "=== OptiWeave Differential Profile ===\n\n";

    // Summary
    oss << "Runtime: " << sign_prefix(diff.runtime_delta_ns) << format_time(diff.runtime_delta_ns);
    if (diff.runtime_delta_percent != 0.0) {
        oss << " (" << sign_prefix(static_cast<int64_t>(diff.runtime_delta_percent))
            << std::fixed << std::setprecision(1) << diff.runtime_delta_percent << "%)";
    }
    oss << "\n";

    oss << "Operations: " << sign_prefix(diff.ops_delta) << diff.ops_delta;
    if (diff.ops_delta_percent != 0.0) {
        oss << " (" << sign_prefix(static_cast<int64_t>(diff.ops_delta_percent))
            << std::fixed << std::setprecision(1) << diff.ops_delta_percent << "%)";
    }
    oss << "\n\n";

    // Changed hotspots
    if (!diff.changed_hotspots.empty()) {
        oss << "--- Changed Hotspots ---\n";
        for (const auto& h : diff.changed_hotspots) {
            const char* arrow = h.time_delta_ns < 0 ? "  FASTER " : "  SLOWER ";
            oss << arrow << h.function << " (" << h.file << ":" << h.line << "): "
                << sign_prefix(h.time_delta_ns) << format_time(h.time_delta_ns);
            if (h.time_delta_percent != 0.0) {
                oss << " (" << sign_prefix(static_cast<int64_t>(h.time_delta_percent))
                    << std::fixed << std::setprecision(1) << h.time_delta_percent << "%)";
            }
            oss << "\n";
        }
        oss << "\n";
    }

    // New hotspots
    if (!diff.new_hotspots.empty()) {
        oss << "--- New Hotspots ---\n";
        for (const auto& h : diff.new_hotspots) {
            oss << "  NEW " << h.function << " (" << h.file << ":" << h.line << "): "
                << format_time(h.time_delta_ns) << "\n";
        }
        oss << "\n";
    }

    // Resolved hotspots
    if (!diff.resolved_hotspots.empty()) {
        oss << "--- Resolved Hotspots ---\n";
        for (const auto& h : diff.resolved_hotspots) {
            oss << "  RESOLVED " << h.function << " (" << h.file << ":" << h.line << ")\n";
        }
        oss << "\n";
    }

    // Suggestion changes
    if (!diff.new_suggestions.empty()) {
        oss << "--- New Suggestions ---\n";
        for (const auto& s : diff.new_suggestions) {
            oss << "  + " << s << "\n";
        }
        oss << "\n";
    }

    if (!diff.resolved_suggestions.empty()) {
        oss << "--- Resolved Suggestions ---\n";
        for (const auto& s : diff.resolved_suggestions) {
            oss << "  - " << s << "\n";
        }
        oss << "\n";
    }

    return oss.str();
}

std::string format_diff_json(const DiffResult& diff) {
    std::ostringstream oss;
    oss << "{\n";

    // Summary
    oss << "  \"summary\": {\n"
        << "    \"runtime_delta_ns\": " << diff.runtime_delta_ns << ",\n"
        << "    \"runtime_delta_percent\": " << std::fixed << std::setprecision(2)
        << diff.runtime_delta_percent << ",\n"
        << "    \"ops_delta\": " << diff.ops_delta << ",\n"
        << "    \"ops_delta_percent\": " << std::fixed << std::setprecision(2)
        << diff.ops_delta_percent << "\n"
        << "  },\n";

    // Changed hotspots
    oss << "  \"changed_hotspots\": [\n";
    for (size_t i = 0; i < diff.changed_hotspots.size(); i++) {
        const auto& h = diff.changed_hotspots[i];
        oss << "    {\"function\": \"" << h.function << "\", \"file\": \"" << h.file
            << "\", \"line\": " << h.line
            << ", \"time_delta_ns\": " << h.time_delta_ns
            << ", \"time_delta_percent\": " << std::fixed << std::setprecision(2)
            << h.time_delta_percent
            << ", \"ops_delta\": " << h.ops_delta << "}";
        if (i + 1 < diff.changed_hotspots.size()) oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";

    // New hotspots
    oss << "  \"new_hotspots\": [\n";
    for (size_t i = 0; i < diff.new_hotspots.size(); i++) {
        const auto& h = diff.new_hotspots[i];
        oss << "    {\"function\": \"" << h.function << "\", \"file\": \"" << h.file
            << "\", \"line\": " << h.line
            << ", \"time_ns\": " << h.time_delta_ns
            << ", \"ops\": " << h.ops_delta << "}";
        if (i + 1 < diff.new_hotspots.size()) oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";

    // Resolved hotspots
    oss << "  \"resolved_hotspots\": [\n";
    for (size_t i = 0; i < diff.resolved_hotspots.size(); i++) {
        const auto& h = diff.resolved_hotspots[i];
        oss << "    {\"function\": \"" << h.function << "\", \"file\": \"" << h.file
            << "\", \"line\": " << h.line << "}";
        if (i + 1 < diff.resolved_hotspots.size()) oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";

    // New suggestions
    oss << "  \"new_suggestions\": [\n";
    for (size_t i = 0; i < diff.new_suggestions.size(); i++) {
        oss << "    \"" << diff.new_suggestions[i] << "\"";
        if (i + 1 < diff.new_suggestions.size()) oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";

    // Resolved suggestions
    oss << "  \"resolved_suggestions\": [\n";
    for (size_t i = 0; i < diff.resolved_suggestions.size(); i++) {
        oss << "    \"" << diff.resolved_suggestions[i] << "\"";
        if (i + 1 < diff.resolved_suggestions.size()) oss << ",";
        oss << "\n";
    }
    oss << "  ]\n";

    oss << "}\n";
    return oss.str();
}

std::string format_diff_markdown(const DiffResult& diff) {
    std::ostringstream oss;

    oss << "# OptiWeave Differential Profile\n\n";

    // Summary
    oss << "## Summary\n\n";
    oss << "| Metric | Delta | Change |\n";
    oss << "|--------|-------|--------|\n";
    oss << "| Runtime | " << sign_prefix(diff.runtime_delta_ns) << format_time(diff.runtime_delta_ns)
        << " | " << sign_prefix(static_cast<int64_t>(diff.runtime_delta_percent))
        << std::fixed << std::setprecision(1) << diff.runtime_delta_percent << "% |\n";
    oss << "| Operations | " << sign_prefix(diff.ops_delta) << diff.ops_delta
        << " | " << sign_prefix(static_cast<int64_t>(diff.ops_delta_percent))
        << std::fixed << std::setprecision(1) << diff.ops_delta_percent << "% |\n\n";

    // Changed hotspots
    if (!diff.changed_hotspots.empty()) {
        oss << "## Changed Hotspots\n\n";
        oss << "| Status | Function | Location | Time Delta | Change |\n";
        oss << "|--------|----------|----------|------------|--------|\n";
        for (const auto& h : diff.changed_hotspots) {
            const char* status = h.time_delta_ns < 0 ? "FASTER" : "SLOWER";
            oss << "| " << status << " | " << h.function << " | " << h.file << ":" << h.line
                << " | " << sign_prefix(h.time_delta_ns) << format_time(h.time_delta_ns)
                << " | " << sign_prefix(static_cast<int64_t>(h.time_delta_percent))
                << std::fixed << std::setprecision(1) << h.time_delta_percent << "% |\n";
        }
        oss << "\n";
    }

    // New hotspots
    if (!diff.new_hotspots.empty()) {
        oss << "## New Hotspots\n\n";
        for (const auto& h : diff.new_hotspots) {
            oss << "- **" << h.function << "** (" << h.file << ":" << h.line << "): "
                << format_time(h.time_delta_ns) << "\n";
        }
        oss << "\n";
    }

    // Resolved hotspots
    if (!diff.resolved_hotspots.empty()) {
        oss << "## Resolved Hotspots\n\n";
        for (const auto& h : diff.resolved_hotspots) {
            oss << "- ~~" << h.function << "~~ (" << h.file << ":" << h.line << ")\n";
        }
        oss << "\n";
    }

    // Suggestion changes
    if (!diff.new_suggestions.empty()) {
        oss << "## New Suggestions\n\n";
        for (const auto& s : diff.new_suggestions) {
            oss << "- " << s << "\n";
        }
        oss << "\n";
    }

    if (!diff.resolved_suggestions.empty()) {
        oss << "## Resolved Suggestions\n\n";
        for (const auto& s : diff.resolved_suggestions) {
            oss << "- ~~" << s << "~~\n";
        }
        oss << "\n";
    }

    return oss.str();
}

} // namespace analysis
} // namespace optiweave
