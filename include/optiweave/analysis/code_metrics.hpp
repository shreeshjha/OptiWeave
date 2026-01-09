#pragma once

#include <optiweave/runtime/hotspot_tracker.hpp>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdint>

namespace optiweave {
namespace analysis {

/// Source location information
using SourceLocation = hotspots::SourceLocation;

/// Complexity metrics for a function or code block
struct ComplexityMetrics {
    /// Cyclomatic complexity (number of linearly independent paths)
    /// Formula: M = E - N + 2P
    /// Where E = edges, N = nodes, P = connected components
    /// Interpretation:
    ///   1-10: Simple, low risk
    ///   11-20: Moderate, medium risk
    ///   21-50: Complex, high risk
    ///   >50: Very complex, very high risk
    uint32_t cyclomatic_complexity = 0;

    /// Cognitive complexity (measure of understandability)
    /// Increments for: nested control structures, recursion, goto, etc.
    /// Interpretation:
    ///   0-5: Very easy to understand
    ///   6-10: Easy to understand
    ///   11-20: Moderate
    ///   21-50: Difficult
    ///   >50: Very difficult
    uint32_t cognitive_complexity = 0;

    /// Nesting depth (maximum nesting level)
    uint32_t max_nesting_depth = 0;

    /// Number of decision points (if, while, for, case, etc.)
    uint32_t decision_points = 0;

    /// Number of return statements
    uint32_t return_statements = 0;

    /// Get complexity level as string
    std::string get_cyclomatic_level() const {
        if (cyclomatic_complexity <= 10) return "Simple";
        if (cyclomatic_complexity <= 20) return "Moderate";
        if (cyclomatic_complexity <= 50) return "Complex";
        return "Very Complex";
    }

    /// Get cognitive complexity level
    std::string get_cognitive_level() const {
        if (cognitive_complexity <= 5) return "Very Easy";
        if (cognitive_complexity <= 10) return "Easy";
        if (cognitive_complexity <= 20) return "Moderate";
        if (cognitive_complexity <= 50) return "Difficult";
        return "Very Difficult";
    }

    /// Get risk level based on cyclomatic complexity
    std::string get_risk_level() const {
        if (cyclomatic_complexity <= 10) return "Low";
        if (cyclomatic_complexity <= 20) return "Medium";
        if (cyclomatic_complexity <= 50) return "High";
        return "Very High";
    }
};

/// Halstead metrics for code volume and difficulty
struct HalsteadMetrics {
    /// Number of distinct operators
    uint32_t n1 = 0;

    /// Number of distinct operands
    uint32_t n2 = 0;

    /// Total number of operators
    uint32_t N1 = 0;

    /// Total number of operands
    uint32_t N2 = 0;

    /// Calculated metrics
    uint32_t vocabulary() const { return n1 + n2; }
    uint32_t length() const { return N1 + N2; }
    double volume() const {
        if (vocabulary() == 0) return 0.0;
        return length() * std::log2(vocabulary());
    }
    double difficulty() const {
        if (n2 == 0) return 0.0;
        return (n1 / 2.0) * (static_cast<double>(N2) / n2);
    }
    double effort() const { return volume() * difficulty(); }
};

/// Function-level metrics
struct FunctionMetrics {
    SourceLocation location;
    std::string function_name;
    std::string return_type;

    /// Basic metrics
    uint32_t lines_of_code = 0;           // Total lines
    uint32_t source_lines_of_code = 0;    // Non-comment, non-blank lines
    uint32_t comment_lines = 0;           // Comment lines
    uint32_t blank_lines = 0;             // Blank lines
    uint32_t parameter_count = 0;         // Number of parameters

    /// Complexity
    ComplexityMetrics complexity;
    HalsteadMetrics halstead;

    /// Call information
    std::vector<std::string> calls_to;        // Functions this calls
    std::vector<std::string> called_by;       // Functions that call this
    uint32_t call_depth = 0;                  // Max depth in call graph
    bool is_recursive = false;                // Direct or indirect recursion

    /// Maintainability Index (0-100, higher is better)
    /// MI = 171 - 5.2 * ln(HV) - 0.23 * CC - 16.2 * ln(LOC)
    /// Where HV = Halstead Volume, CC = Cyclomatic Complexity, LOC = Lines of Code
    /// Interpretation:
    ///   85-100: Highly maintainable
    ///   65-85: Moderately maintainable
    ///   <65: Difficult to maintain
    double maintainability_index() const {
        if (source_lines_of_code == 0) return 100.0;
        double hv = halstead.volume();
        double cc = complexity.cyclomatic_complexity;
        double loc = source_lines_of_code;

        double mi = 171.0 - 5.2 * std::log(hv > 0 ? hv : 1.0)
                          - 0.23 * cc
                          - 16.2 * std::log(loc);

        // Normalize to 0-100
        if (mi < 0) mi = 0;
        if (mi > 100) mi = 100;

        return mi;
    }

    std::string get_maintainability_level() const {
        double mi = maintainability_index();
        if (mi >= 85) return "Highly Maintainable";
        if (mi >= 65) return "Moderately Maintainable";
        return "Difficult to Maintain";
    }
};

/// File-level metrics
struct FileMetrics {
    std::string file_path;

    /// Aggregated metrics
    uint32_t total_lines = 0;
    uint32_t total_sloc = 0;
    uint32_t total_functions = 0;
    uint32_t total_classes = 0;

    /// Average complexity
    double avg_cyclomatic_complexity = 0.0;
    double avg_cognitive_complexity = 0.0;
    double avg_maintainability_index = 0.0;

    /// Dependencies
    std::vector<std::string> includes;        // Files this includes
    std::vector<std::string> included_by;     // Files that include this

    /// Functions in this file
    std::vector<FunctionMetrics> functions;
};

/// Call graph edge
struct CallEdge {
    std::string caller;       // Function that makes the call
    std::string callee;       // Function being called
    SourceLocation location;  // Where the call happens
    uint32_t call_count = 1;  // How many times (if runtime data available)
};

/// Complete code analysis result
struct CodeAnalysisResult {
    /// All analyzed functions
    std::map<std::string, FunctionMetrics> functions;

    /// All analyzed files
    std::map<std::string, FileMetrics> files;

    /// Call graph edges
    std::vector<CallEdge> call_graph;

    /// Project-wide statistics
    struct ProjectStats {
        uint32_t total_files = 0;
        uint32_t total_functions = 0;
        uint32_t total_lines = 0;
        uint32_t total_sloc = 0;

        double avg_cyclomatic_complexity = 0.0;
        double avg_cognitive_complexity = 0.0;
        double avg_maintainability_index = 0.0;

        uint32_t complex_functions = 0;      // CC > 20
        uint32_t very_complex_functions = 0; // CC > 50
    } project_stats;

    /// Get most complex functions
    std::vector<FunctionMetrics> get_most_complex(size_t top_n = 10) const;

    /// Get least maintainable functions
    std::vector<FunctionMetrics> get_least_maintainable(size_t top_n = 10) const;

    /// Get functions with highest cognitive complexity
    std::vector<FunctionMetrics> get_hardest_to_understand(size_t top_n = 10) const;

    /// Get call graph depth
    uint32_t get_max_call_depth() const;

    /// Find circular dependencies
    std::vector<std::vector<std::string>> find_circular_dependencies() const;
};

} // namespace analysis
} // namespace optiweave
