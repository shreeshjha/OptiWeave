#include <optiweave/analysis/complexity_analyzer.hpp>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace optiweave {
namespace analysis {
namespace export_utils {

std::string export_terminal(const CodeAnalysisResult& result) {
    std::ostringstream oss;

    oss << "\n";
    oss << "╔══════════════════════════════════════════════════════════════╗\n";
    oss << "║        OptiWeave Code Complexity Analysis                   ║\n";
    oss << "╚══════════════════════════════════════════════════════════════╝\n";
    oss << "\n";

    // Project-wide statistics
    const auto& stats = result.project_stats;

    oss << "Project Statistics:\n";
    oss << "  Total Functions: " << stats.total_functions << "\n";
    oss << "  Total Lines: " << stats.total_lines << "\n";
    oss << "  Total SLOC: " << stats.total_sloc << "\n";
    oss << "  Complex Functions (CC > 20): " << stats.complex_functions << "\n";
    oss << "  Very Complex Functions (CC > 50): " << stats.very_complex_functions << "\n";
    oss << "\n";

    oss << "Average Metrics:\n";
    oss << "  Cyclomatic Complexity: " << std::fixed << std::setprecision(2)
        << stats.avg_cyclomatic_complexity << "\n";
    oss << "  Cognitive Complexity: " << std::fixed << std::setprecision(2)
        << stats.avg_cognitive_complexity << "\n";
    oss << "  Maintainability Index: " << std::fixed << std::setprecision(2)
        << stats.avg_maintainability_index << "\n";
    oss << "\n";

    // Most complex functions
    auto most_complex = result.get_most_complex(10);
    if (!most_complex.empty()) {
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "🔥 Most Complex Functions (by Cyclomatic Complexity)\n";
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "\n";

        oss << "┌────┬─────────────────────────────┬─────────┬─────────┬──────────┬────────────┐\n";
        oss << "│ #  │ Function                    │ CC      │ CogC    │ MI       │ Risk       │\n";
        oss << "├────┼─────────────────────────────┼─────────┼─────────┼──────────┼────────────┤\n";

        int rank = 1;
        for (const auto& func : most_complex) {
            oss << "│ " << std::setw(2) << rank++ << " │ ";
            oss << std::left << std::setw(27) << func.function_name.substr(0, 27) << " │ ";
            oss << std::right << std::setw(7) << func.complexity.cyclomatic_complexity << " │ ";
            oss << std::setw(7) << func.complexity.cognitive_complexity << " │ ";
            oss << std::setw(8) << std::fixed << std::setprecision(1) << func.maintainability_index() << " │ ";
            oss << std::left << std::setw(10) << func.complexity.get_risk_level() << " │\n";
        }

        oss << "└────┴─────────────────────────────┴─────────┴─────────┴──────────┴────────────┘\n";
        oss << "\n";

        // Legend
        oss << "Legend:\n";
        oss << "  CC   = Cyclomatic Complexity (decision points)\n";
        oss << "  CogC = Cognitive Complexity (understandability)\n";
        oss << "  MI   = Maintainability Index (0-100, higher is better)\n";
        oss << "\n";
    }

    // Least maintainable functions
    auto least_maintainable = result.get_least_maintainable(5);
    if (!least_maintainable.empty()) {
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "⚠️  Least Maintainable Functions\n";
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "\n";

        for (const auto& func : least_maintainable) {
            oss << "Function: " << func.function_name << "\n";
            oss << "  Location: " << func.location.file << ":" << func.location.line << "\n";
            oss << "  Maintainability Index: " << std::fixed << std::setprecision(1)
                << func.maintainability_index() << " (" << func.get_maintainability_level() << ")\n";
            oss << "  Cyclomatic Complexity: " << func.complexity.cyclomatic_complexity
                << " (" << func.complexity.get_cyclomatic_level() << ")\n";
            oss << "  Lines of Code: " << func.source_lines_of_code << "\n";
            oss << "\n";
        }
    }

    // Hardest to understand
    auto hardest = result.get_hardest_to_understand(5);
    if (!hardest.empty()) {
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "🧠 Hardest to Understand Functions (by Cognitive Complexity)\n";
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "\n";

        for (const auto& func : hardest) {
            oss << "Function: " << func.function_name << "\n";
            oss << "  Cognitive Complexity: " << func.complexity.cognitive_complexity
                << " (" << func.complexity.get_cognitive_level() << ")\n";
            oss << "  Max Nesting Depth: " << func.complexity.max_nesting_depth << "\n";
            oss << "  Decision Points: " << func.complexity.decision_points << "\n";
            oss << "\n";
        }
    }

    // Call graph insights
    auto circular_deps = result.find_circular_dependencies();
    if (!circular_deps.empty()) {
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "🔄 Circular Dependencies Detected\n";
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "\n";

        for (const auto& cycle : circular_deps) {
            oss << "Cycle: ";
            for (size_t i = 0; i < cycle.size(); ++i) {
                if (i > 0) oss << " → ";
                oss << cycle[i];
            }
            oss << "\n";
        }
        oss << "\n";
    }

    // Summary and recommendations
    oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    oss << "📊 Summary & Recommendations\n";
    oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    oss << "\n";

    if (stats.very_complex_functions > 0) {
        oss << "⚠️  CRITICAL: " << stats.very_complex_functions
            << " function(s) with very high complexity (CC > 50)\n";
        oss << "   → Consider refactoring into smaller functions\n";
        oss << "   → Extract helper functions for complex logic\n";
        oss << "\n";
    }

    if (stats.complex_functions > 0) {
        oss << "⚡ WARNING: " << stats.complex_functions
            << " function(s) with high complexity (CC > 20)\n";
        oss << "   → Review for simplification opportunities\n";
        oss << "   → Add comprehensive unit tests\n";
        oss << "\n";
    }

    if (stats.avg_maintainability_index < 65) {
        oss << "📉 Low average maintainability index (" << std::fixed << std::setprecision(1)
            << stats.avg_maintainability_index << ")\n";
        oss << "   → Focus on reducing complexity\n";
        oss << "   → Improve code documentation\n";
        oss << "   → Break down large functions\n";
        oss << "\n";
    }

    if (circular_deps.empty() && stats.very_complex_functions == 0) {
        oss << "✅ Good code structure!\n";
        oss << "   → No circular dependencies detected\n";
        oss << "   → No extremely complex functions\n";
        oss << "\n";
    }

    return oss.str();
}

std::string export_json(const CodeAnalysisResult& result) {
    std::ostringstream oss;

    oss << "{\n";
    oss << "  \"project_stats\": {\n";
    oss << "    \"total_functions\": " << result.project_stats.total_functions << ",\n";
    oss << "    \"total_lines\": " << result.project_stats.total_lines << ",\n";
    oss << "    \"total_sloc\": " << result.project_stats.total_sloc << ",\n";
    oss << "    \"avg_cyclomatic_complexity\": " << result.project_stats.avg_cyclomatic_complexity << ",\n";
    oss << "    \"avg_cognitive_complexity\": " << result.project_stats.avg_cognitive_complexity << ",\n";
    oss << "    \"avg_maintainability_index\": " << result.project_stats.avg_maintainability_index << ",\n";
    oss << "    \"complex_functions\": " << result.project_stats.complex_functions << ",\n";
    oss << "    \"very_complex_functions\": " << result.project_stats.very_complex_functions << "\n";
    oss << "  },\n";

    oss << "  \"functions\": [\n";
    bool first_func = true;
    for (const auto& [name, func] : result.functions) {
        if (!first_func) oss << ",\n";
        first_func = false;

        oss << "    {\n";
        oss << "      \"name\": \"" << name << "\",\n";
        oss << "      \"file\": \"" << func.location.file << "\",\n";
        oss << "      \"line\": " << func.location.line << ",\n";
        oss << "      \"cyclomatic_complexity\": " << func.complexity.cyclomatic_complexity << ",\n";
        oss << "      \"cognitive_complexity\": " << func.complexity.cognitive_complexity << ",\n";
        oss << "      \"maintainability_index\": " << func.maintainability_index() << ",\n";
        oss << "      \"lines_of_code\": " << func.lines_of_code << ",\n";
        oss << "      \"sloc\": " << func.source_lines_of_code << ",\n";
        oss << "      \"max_nesting_depth\": " << func.complexity.max_nesting_depth << ",\n";
        oss << "      \"parameter_count\": " << func.parameter_count << ",\n";
        oss << "      \"is_recursive\": " << (func.is_recursive ? "true" : "false") << "\n";
        oss << "    }";
    }
    oss << "\n  ]\n";
    oss << "}\n";

    return oss.str();
}

std::string export_call_graph_dot(const CodeAnalysisResult& result) {
    std::ostringstream oss;

    oss << "digraph CallGraph {\n";
    oss << "  rankdir=TB;\n";
    oss << "  node [shape=box, style=rounded];\n";
    oss << "\n";

    // Color code by complexity
    for (const auto& [name, func] : result.functions) {
        std::string color = "lightgreen";
        if (func.complexity.cyclomatic_complexity > 50) {
            color = "red";
        } else if (func.complexity.cyclomatic_complexity > 20) {
            color = "orange";
        } else if (func.complexity.cyclomatic_complexity > 10) {
            color = "yellow";
        }

        oss << "  \"" << name << "\" [fillcolor=" << color << ", style=\"rounded,filled\""
            << ", label=\"" << name << "\\nCC: " << func.complexity.cyclomatic_complexity << "\"];\n";
    }

    oss << "\n";

    // Edges
    for (const auto& edge : result.call_graph) {
        oss << "  \"" << edge.caller << "\" -> \"" << edge.callee << "\";\n";
    }

    oss << "}\n";

    return oss.str();
}

std::string export_markdown(const CodeAnalysisResult& result) {
    std::ostringstream oss;

    oss << "# Code Complexity Analysis Report\n\n";

    // Project statistics
    oss << "## Project Statistics\n\n";
    oss << "| Metric | Value |\n";
    oss << "|--------|-------|\n";
    oss << "| Total Functions | " << result.project_stats.total_functions << " |\n";
    oss << "| Total Lines | " << result.project_stats.total_lines << " |\n";
    oss << "| Total SLOC | " << result.project_stats.total_sloc << " |\n";
    oss << "| Avg Cyclomatic Complexity | " << std::fixed << std::setprecision(2)
        << result.project_stats.avg_cyclomatic_complexity << " |\n";
    oss << "| Avg Cognitive Complexity | " << std::fixed << std::setprecision(2)
        << result.project_stats.avg_cognitive_complexity << " |\n";
    oss << "| Avg Maintainability Index | " << std::fixed << std::setprecision(2)
        << result.project_stats.avg_maintainability_index << " |\n";
    oss << "| Complex Functions (CC > 20) | " << result.project_stats.complex_functions << " |\n";
    oss << "| Very Complex Functions (CC > 50) | " << result.project_stats.very_complex_functions << " |\n";
    oss << "\n";

    // Most complex functions
    auto most_complex = result.get_most_complex(10);
    if (!most_complex.empty()) {
        oss << "## Most Complex Functions\n\n";
        oss << "| Rank | Function | CC | CogC | MI | Risk |\n";
        oss << "|------|----------|----:|-----:|---:|------|\n";

        int rank = 1;
        for (const auto& func : most_complex) {
            oss << "| " << rank++ << " | `" << func.function_name << "` | "
                << func.complexity.cyclomatic_complexity << " | "
                << func.complexity.cognitive_complexity << " | "
                << std::fixed << std::setprecision(1) << func.maintainability_index() << " | "
                << func.complexity.get_risk_level() << " |\n";
        }
        oss << "\n";
    }

    return oss.str();
}

std::string export_html(const CodeAnalysisResult& result) {
    // Similar to markdown but with HTML formatting
    // Implementation omitted for brevity - would be similar to hotspot HTML export
    return export_markdown(result); // Placeholder
}

std::string export_dependency_graph_dot(const CodeAnalysisResult& result) {
    std::ostringstream oss;

    oss << "digraph DependencyGraph {\n";
    oss << "  rankdir=LR;\n";
    oss << "  node [shape=box];\n";
    oss << "\n";

    // File dependencies
    for (const auto& [file, metrics] : result.files) {
        for (const auto& include : metrics.includes) {
            oss << "  \"" << file << "\" -> \"" << include << "\";\n";
        }
    }

    oss << "}\n";

    return oss.str();
}

} // namespace export_utils
} // namespace analysis
} // namespace optiweave
