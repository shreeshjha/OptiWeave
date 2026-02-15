#include <optiweave/runtime/json_exporter.hpp>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

namespace optiweave {
namespace json {

std::string escape_json_string(const std::string& str) {
    std::ostringstream oss;
    for (char c : str) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default:
                if (c < 32) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    oss << c;
                }
        }
    }
    return oss.str();
}

std::string export_hotspots_json(const hotspots::HotspotTracker& tracker) {
    auto hotspots = tracker.get_top_hotspots(100);

    // Calculate total time for percentages
    uint64_t total_time = 0;
    for (const auto& h : hotspots) {
        total_time += h.total_time_ns;
    }

    std::ostringstream oss;
    oss << "[\n";

    bool first = true;
    for (const auto& h : hotspots) {
        if (!first) oss << ",\n";
        first = false;

        double percent = total_time > 0 ? (100.0 * h.total_time_ns / total_time) : 0.0;
        double time_ms = h.total_time_ns / 1000000.0;

        oss << "  {\n"
            << "    \"name\": \"" << escape_json_string(h.location.function) << "\",\n"
            << "    \"file\": \"" << escape_json_string(h.location.file) << "\",\n"
            << "    \"line\": " << h.location.line << ",\n"
            << "    \"time_ns\": " << h.total_time_ns << ",\n"
            << "    \"time_ms\": " << std::fixed << std::setprecision(3) << time_ms << ",\n"
            << "    \"percent\": " << std::fixed << std::setprecision(2) << percent << ",\n"
            << "    \"operations\": " << h.operation_count << "\n"
            << "  }";
    }

    oss << "\n]\n";
    return oss.str();
}

std::string export_operations_json(const statistics::OperationCounters& stats) {
    struct OpData {
        std::string name;
        uint64_t count;
    };

    std::vector<OpData> ops = {
        {"Array Access", stats.array_subscript.load()},
        {"Addition", stats.addition.load()},
        {"Subtraction", stats.subtraction.load()},
        {"Multiplication", stats.multiplication.load()},
        {"Division", stats.division.load()},
        {"Modulo", stats.modulo.load()},
        {"Less Than", stats.less_than.load()},
        {"Greater Than", stats.greater_than.load()},
        {"Less Equal", stats.less_equal.load()},
        {"Greater Equal", stats.greater_equal.load()},
        {"Equal", stats.equal.load()},
        {"Not Equal", stats.not_equal.load()},
        {"Assignment", stats.assignment.load()},
        {"Add Assign", stats.add_assign.load()},
        {"Sub Assign", stats.sub_assign.load()},
        {"Mul Assign", stats.mul_assign.load()},
        {"Div Assign", stats.div_assign.load()}
    };

    // Calculate total
    uint64_t total = 0;
    for (const auto& op : ops) {
        total += op.count;
    }

    // Sort by count (descending)
    std::sort(ops.begin(), ops.end(), [](const OpData& a, const OpData& b) {
        return a.count > b.count;
    });

    std::ostringstream oss;
    oss << "[\n";

    bool first = true;
    for (const auto& op : ops) {
        if (op.count == 0) continue;  // Skip zero counts

        if (!first) oss << ",\n";
        first = false;

        double percent = total > 0 ? (100.0 * op.count / total) : 0.0;

        oss << "  {\n"
            << "    \"type\": \"" << op.name << "\",\n"
            << "    \"count\": " << op.count << ",\n"
            << "    \"percent\": " << std::fixed << std::setprecision(2) << percent << "\n"
            << "  }";
    }

    oss << "\n]\n";
    return oss.str();
}

std::string export_suggestions_json(const analysis::AnalysisResult& result) {
    std::ostringstream oss;
    oss << "[\n";

    bool first = true;
    for (const auto& pattern : result.patterns) {
        if (!first) oss << ",\n";
        first = false;

        oss << "  {\n"
            << "    \"severity\": \"" << pattern.severity_string() << "\",\n"
            << "    \"title\": \"" << escape_json_string(pattern.pattern_name) << "\",\n"
            << "    \"category\": \"" << pattern.category_string() << "\",\n"
            << "    \"location\": {\n"
            << "      \"file\": \"" << escape_json_string(pattern.location.file) << "\",\n"
            << "      \"line\": " << pattern.location.line << ",\n"
            << "      \"function\": \"" << escape_json_string(pattern.location.function) << "\"\n"
            << "    },\n"
            << "    \"speedup\": \"" << pattern.speedup_string() << "\",\n"
            << "    \"speedup_min\": " << std::fixed << std::setprecision(2) << pattern.estimated_speedup_min << ",\n"
            << "    \"speedup_max\": " << std::fixed << std::setprecision(2) << pattern.estimated_speedup_max << ",\n"
            << "    \"description\": \"" << escape_json_string(pattern.description) << "\",\n"
            << "    \"why_slow\": \"" << escape_json_string(pattern.why_slow) << "\",\n"
            << "    \"rationale\": \"" << escape_json_string(pattern.rationale) << "\",\n"
            << "    \"current_code\": \"" << escape_json_string(pattern.current_code) << "\",\n"
            << "    \"optimized_code\": \"" << escape_json_string(pattern.optimized_code) << "\",\n"
            << "    \"time_ns\": " << pattern.time_ns << ",\n"
            << "    \"time_percent\": " << std::fixed << std::setprecision(2) << pattern.percentage_of_total << ",\n"
            << "    \"requirements\": [";

        bool first_req = true;
        for (const auto& req : pattern.requirements) {
            if (!first_req) oss << ", ";
            first_req = false;
            oss << "\"" << escape_json_string(req) << "\"";
        }

        oss << "],\n"
            << "    \"references\": [";

        bool first_ref = true;
        for (const auto& ref : pattern.references) {
            if (!first_ref) oss << ", ";
            first_ref = false;
            oss << "\"" << escape_json_string(ref) << "\"";
        }

        oss << "],\n"
            << "    \"line_start\": " << pattern.line_start << ",\n"
            << "    \"line_end\": " << pattern.line_end << ",\n"
            << "    \"patchable\": " << (pattern.patchable ? "true" : "false") << "\n"
            << "  }";
    }

    oss << "\n]\n";
    return oss.str();
}

std::string export_dashboard_json(
    const hotspots::HotspotTracker& tracker,
    const statistics::OperationCounters& stats,
    const analysis::AnalysisResult& result
) {
    // Calculate summary metrics
    uint64_t total_ops = stats.array_subscript.load() +
                        stats.addition.load() +
                        stats.subtraction.load() +
                        stats.multiplication.load() +
                        stats.division.load();

    auto hotspots = tracker.get_top_hotspots(100);
    uint64_t total_time_ns = 0;
    for (const auto& h : hotspots) {
        total_time_ns += h.total_time_ns;
    }
    double total_time_ms = total_time_ns / 1000000.0;

    // Calculate combined speedup (more conservative approach - average instead of multiply)
    double avg_speedup = 0.0;
    if (!result.patterns.empty()) {
        for (const auto& p : result.patterns) {
            avg_speedup += (p.estimated_speedup_min + p.estimated_speedup_max) / 2.0;
        }
        avg_speedup /= result.patterns.size();
    }

    std::ostringstream oss;
    oss << "{\n"
        << "  \"summary\": {\n"
        << "    \"total_operations\": " << total_ops << ",\n"
        << "    \"total_runtime_ns\": " << total_time_ns << ",\n"
        << "    \"total_runtime_ms\": " << std::fixed << std::setprecision(3) << total_time_ms << ",\n"
        << "    \"issues_found\": " << result.patterns.size() << ",\n"
        << "    \"potential_speedup\": \"" << std::fixed << std::setprecision(1) << avg_speedup << "x\"\n"
        << "  },\n"
        << "  \"hotspots\": " << export_hotspots_json(tracker) << ",\n"
        << "  \"operations\": " << export_operations_json(stats) << ",\n"
        << "  \"suggestions\": " << export_suggestions_json(result) << "\n"
        << "}\n";

    return oss.str();
}

bool write_json_file(const std::string& json, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    file << json;
    file.close();
    return true;
}

} // namespace json
} // namespace optiweave
