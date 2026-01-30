#include <optiweave/analysis/pattern_detector.hpp>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

namespace optiweave {
namespace analysis {

// ============================================================================
// Threshold Constants
// ============================================================================
// These thresholds determine when optimization patterns are flagged.
// Values can be adjusted based on target platform and use case.

namespace thresholds {
    // Minimum time spent in a loop to consider it "hot" (10 microseconds)
    constexpr uint64_t kMinHotLoopTimeNs = 10000;

    // Minimum time for O(n^2) algorithm detection (10 milliseconds)
    constexpr uint64_t kMinQuadraticAlgoTimeNs = 10000000;

    // Minimum time for branch misprediction detection (50 microseconds)
    constexpr uint64_t kMinBranchAnalysisTimeNs = 50000;

    // Minimum time for loop-invariant code motion suggestion (100 microseconds)
    constexpr uint64_t kMinLoopInvariantTimeNs = 100000;

    // Minimum iterations for loop-invariant code motion suggestion
    constexpr uint64_t kMinLoopInvariantIterations = 5000;

    // Minimum iterations for branch optimization to be worthwhile
    constexpr uint64_t kMinBranchOptIterations = 1000;

    // Comparison ratio threshold for branch misprediction detection (15%)
    constexpr double kBranchComparisonRatioThreshold = 0.15;

    // Typical cache line size on x86_64 (bytes)
    constexpr int kCacheLineSize = 64;

    // Assumed element size for cache analysis (bytes, e.g., double)
    constexpr int kAssumedElementSize = 8;
} // namespace thresholds

// ============================================================================
// OptimizationAnalyzer Implementation
// ============================================================================

OptimizationAnalyzer::OptimizationAnalyzer() {
    // Register default detectors
    register_detector(std::make_unique<DivisionInLoopDetector>());
    register_detector(std::make_unique<ComplexityDetector>());
    register_detector(std::make_unique<MemoryAccessDetector>());
    register_detector(std::make_unique<VectorizationDetector>());
    register_detector(std::make_unique<RepeatedComputationDetector>());
    register_detector(std::make_unique<BranchMispredictionDetector>());
}

void OptimizationAnalyzer::register_detector(std::unique_ptr<PatternDetector> detector) {
    detectors_.push_back(std::move(detector));
}

AnalysisResult OptimizationAnalyzer::analyze(
    const hotspots::HotspotTracker& hotspots,
    const optiweave::statistics::OperationCounters& stats,
    const std::vector<LoopInfo>& loop_info
) {
    AnalysisResult result;

    // Run all registered detectors
    for (auto& detector : detectors_) {
        detector->analyze(hotspots, stats, loop_info);
        auto patterns = detector->get_patterns();

        for (const auto& pattern : patterns) {
            result.add_pattern(pattern);
        }
    }

    // Sort patterns by impact
    result.sort_by_impact();

    return result;
}

std::string OptimizationAnalyzer::generate_terminal_output(const AnalysisResult& result) const {
    std::ostringstream oss;

    // Header
    oss << "\n";
    oss << "╔══════════════════════════════════════════════════════════════╗\n";
    oss << "║        OptiWeave Optimization Suggestions                    ║\n";
    oss << "╚══════════════════════════════════════════════════════════════╝\n";
    oss << "\n";

    if (result.total_count() == 0) {
        oss << "No optimization opportunities detected.\n";
        oss << "Your code looks well-optimized! 🎉\n\n";
        return oss.str();
    }

    // Summary
    oss << "Analysis Complete: " << result.total_count() << " optimization opportunities found\n";
    oss << "Potential combined speedup: " << std::fixed << std::setprecision(1)
        << result.combined_speedup_min << "-"
        << result.combined_speedup_max << "x\n";
    oss << "\n";

    // High impact suggestions
    if (result.high_impact_count > 0) {
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "🔥 HIGH IMPACT (Estimated 10x+ speedup)\n";
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "\n";
        oss << format_severity_section(result, Severity::HIGH);
    }

    // Medium impact suggestions
    if (result.medium_impact_count > 0) {
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "⚡ MEDIUM IMPACT (Estimated 2-10x speedup)\n";
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "\n";
        oss << format_severity_section(result, Severity::MEDIUM);
    }

    // Low impact suggestions
    if (result.low_impact_count > 0) {
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "💡 LOW IMPACT (Estimated < 2x speedup)\n";
        oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        oss << "\n";
        oss << format_severity_section(result, Severity::LOW);
    }

    // Summary
    oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    oss << "📊 Summary\n";
    oss << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    oss << "\n";
    oss << "Total Optimizations: " << result.total_count() << "\n";
    oss << "  🔥 High Impact:   " << result.high_impact_count << "\n";
    oss << "  ⚡ Medium Impact: " << result.medium_impact_count << "\n";
    oss << "  💡 Low Impact:    " << result.low_impact_count << "\n";
    oss << "\n";
    oss << "Estimated Combined Speedup: " << std::fixed << std::setprecision(1)
        << result.combined_speedup_min << "-"
        << result.combined_speedup_max << "x\n";
    oss << "\n";

    return oss.str();
}

std::string OptimizationAnalyzer::format_severity_section(
    const AnalysisResult& result,
    Severity severity
) const {
    std::ostringstream oss;
    int count = 1;

    for (const auto& pattern : result.patterns) {
        if (pattern.severity != severity) continue;

        oss << "[" << count++ << "] " << pattern.pattern_name << "\n";
        oss << "    Location: " << pattern.location.file << ":" << pattern.location.line
            << " (" << pattern.location.function << ")\n";
        oss << "\n";

        oss << "    Pattern Detected:\n";
        oss << "    • " << pattern.description << "\n";
        if (pattern.percentage_of_total > 0) {
            oss << "    • Time spent: " << std::fixed << std::setprecision(1)
                << pattern.percentage_of_total << "% of total execution\n";
        }
        oss << "\n";

        if (!pattern.current_code.empty()) {
            oss << "    Current Code:\n";
            std::istringstream code_stream(pattern.current_code);
            std::string line;
            while (std::getline(code_stream, line)) {
                oss << "      " << line << "\n";
            }
            oss << "\n";
        }

        if (!pattern.why_slow.empty()) {
            oss << "    Why It's Slow:\n";
            oss << "    • " << pattern.why_slow << "\n";
            oss << "\n";
        }

        if (!pattern.optimized_code.empty()) {
            oss << "    Recommended Fix:\n";
            std::istringstream opt_stream(pattern.optimized_code);
            std::string line;
            while (std::getline(opt_stream, line)) {
                oss << "      " << line << "\n";
            }
            oss << "\n";
        }

        oss << "    Expected Speedup: " << pattern.speedup_string() << "\n";

        if (!pattern.rationale.empty()) {
            oss << "\n";
            oss << "    Rationale:\n";
            oss << "    " << pattern.rationale << "\n";
        }

        if (!pattern.requirements.empty()) {
            oss << "\n";
            oss << "    Requirements:\n";
            for (const auto& req : pattern.requirements) {
                oss << "    • " << req << "\n";
            }
        }

        oss << "\n";
    }

    return oss.str();
}

std::string OptimizationAnalyzer::generate_markdown_report(const AnalysisResult& result) const {
    std::ostringstream oss;

    oss << "# OptiWeave Optimization Suggestions\n\n";

    if (result.total_count() == 0) {
        oss << "No optimization opportunities detected. Your code looks well-optimized! 🎉\n";
        return oss.str();
    }

    // Summary
    oss << "## Summary\n\n";
    oss << "- **Total Optimizations Found:** " << result.total_count() << "\n";
    oss << "- **High Impact:** " << result.high_impact_count << " 🔥\n";
    oss << "- **Medium Impact:** " << result.medium_impact_count << " ⚡\n";
    oss << "- **Low Impact:** " << result.low_impact_count << " 💡\n";
    oss << "- **Estimated Combined Speedup:** " << std::fixed << std::setprecision(1)
        << result.combined_speedup_min << "-" << result.combined_speedup_max << "x\n";
    oss << "\n---\n\n";

    // High impact
    if (result.high_impact_count > 0) {
        oss << "## 🔥 High Impact Optimizations (10x+ speedup)\n\n";
        for (const auto& pattern : result.patterns) {
            if (pattern.severity != Severity::HIGH) continue;
            oss << "### " << pattern.pattern_name << "\n\n";
            oss << "**Location:** `" << pattern.location.file << ":" << pattern.location.line << "`\n\n";
            oss << "**Pattern:** " << pattern.description << "\n\n";
            if (!pattern.current_code.empty()) {
                oss << "**Current Code:**\n```cpp\n" << pattern.current_code << "\n```\n\n";
            }
            if (!pattern.optimized_code.empty()) {
                oss << "**Optimized Code:**\n```cpp\n" << pattern.optimized_code << "\n```\n\n";
            }
            oss << "**Expected Speedup:** " << pattern.speedup_string() << "\n\n";
            if (!pattern.rationale.empty()) {
                oss << "**Rationale:** " << pattern.rationale << "\n\n";
            }
            oss << "---\n\n";
        }
    }

    return oss.str();
}

std::string OptimizationAnalyzer::generate_html_report(const AnalysisResult& result) const {
    std::ostringstream oss;

    oss << R"(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>OptiWeave Optimization Suggestions</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: #f5f5f5;
      color: #333;
      line-height: 1.6;
    }
    .container {
      max-width: 1200px;
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
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 20px;
      margin-bottom: 30px;
    }
    .stat-card {
      background: white;
      padding: 25px;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
      text-align: center;
    }
    .stat-card.high { border-top: 4px solid #e63946; }
    .stat-card.medium { border-top: 4px solid #f9a825; }
    .stat-card.low { border-top: 4px solid #4caf50; }
    .stat-card.speedup { border-top: 4px solid #667eea; }
    .stat-card h3 { color: #666; margin-bottom: 10px; font-size: 0.9em; text-transform: uppercase; }
    .stat-card .value { font-size: 2.2em; font-weight: bold; }
    .stat-card.high .value { color: #e63946; }
    .stat-card.medium .value { color: #f9a825; }
    .stat-card.low .value { color: #4caf50; }
    .stat-card.speedup .value { color: #667eea; }
    .suggestion {
      background: white;
      padding: 25px;
      border-radius: 8px;
      margin-bottom: 20px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .suggestion.high { border-left: 5px solid #e63946; }
    .suggestion.medium { border-left: 5px solid #f9a825; }
    .suggestion.low { border-left: 5px solid #4caf50; }
    .suggestion-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 15px;
    }
    .suggestion h3 { font-size: 1.3em; color: #333; }
    .badge {
      padding: 5px 12px;
      border-radius: 20px;
      font-size: 0.8em;
      font-weight: bold;
      text-transform: uppercase;
    }
    .badge.high { background: #ffebee; color: #e63946; }
    .badge.medium { background: #fff8e1; color: #f9a825; }
    .badge.low { background: #e8f5e9; color: #4caf50; }
    .location {
      font-family: 'Consolas', 'Monaco', monospace;
      font-size: 0.9em;
      color: #666;
      margin-bottom: 15px;
    }
    .description { margin-bottom: 15px; }
    .why-slow {
      background: #fff3e0;
      padding: 12px;
      border-radius: 5px;
      margin-bottom: 15px;
      border-left: 3px solid #ff9800;
    }
    .why-slow strong { color: #e65100; }
    .code-section { margin-bottom: 15px; }
    .code-section h4 {
      font-size: 0.9em;
      color: #666;
      margin-bottom: 8px;
      text-transform: uppercase;
    }
    pre {
      background: #263238;
      color: #eeffff;
      padding: 15px;
      border-radius: 5px;
      overflow-x: auto;
      font-family: 'Consolas', 'Monaco', monospace;
      font-size: 0.9em;
      line-height: 1.4;
    }
    .optimized pre {
      background: #1b5e20;
    }
    .speedup-info {
      background: #e3f2fd;
      padding: 12px;
      border-radius: 5px;
      display: inline-block;
    }
    .speedup-info strong { color: #1565c0; }
    .requirements {
      margin-top: 15px;
      padding: 12px;
      background: #f5f5f5;
      border-radius: 5px;
    }
    .requirements h4 {
      font-size: 0.9em;
      color: #666;
      margin-bottom: 8px;
    }
    .requirements ul {
      margin-left: 20px;
      color: #555;
    }
    .section-title {
      font-size: 1.5em;
      color: #333;
      margin: 30px 0 20px;
      padding-bottom: 10px;
      border-bottom: 2px solid #e0e0e0;
    }
    .no-suggestions {
      text-align: center;
      padding: 60px;
      background: white;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .no-suggestions h2 { color: #4caf50; margin-bottom: 10px; }
    footer {
      text-align: center;
      padding: 20px;
      color: #999;
      font-size: 0.9em;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>OptiWeave Optimization Suggestions</h1>
      <p>Automated performance analysis and optimization recommendations</p>
    </header>
)";

    if (result.total_count() == 0) {
        oss << R"(
    <div class="no-suggestions">
      <h2>No Optimization Opportunities Detected</h2>
      <p>Your code looks well-optimized! Great job!</p>
    </div>
)";
    } else {
        // Summary cards
        oss << R"(
    <div class="summary">
      <div class="stat-card high">
        <h3>High Impact</h3>
        <div class="value">)" << result.high_impact_count << R"(</div>
      </div>
      <div class="stat-card medium">
        <h3>Medium Impact</h3>
        <div class="value">)" << result.medium_impact_count << R"(</div>
      </div>
      <div class="stat-card low">
        <h3>Low Impact</h3>
        <div class="value">)" << result.low_impact_count << R"(</div>
      </div>
      <div class="stat-card speedup">
        <h3>Potential Speedup</h3>
        <div class="value">)" << std::fixed << std::setprecision(1)
            << result.combined_speedup_min << "-" << result.combined_speedup_max << R"(x</div>
      </div>
    </div>
)";

        // Helper lambda to output patterns by severity
        auto output_patterns = [&](Severity sev, const std::string& title) {
            bool has_patterns = false;
            for (const auto& pattern : result.patterns) {
                if (pattern.severity == sev) {
                    has_patterns = true;
                    break;
                }
            }
            if (!has_patterns) return;

            oss << "    <h2 class=\"section-title\">" << title << "</h2>\n";

            for (const auto& pattern : result.patterns) {
                if (pattern.severity != sev) continue;

                std::string sev_class;
                switch (sev) {
                    case Severity::HIGH: sev_class = "high"; break;
                    case Severity::MEDIUM: sev_class = "medium"; break;
                    case Severity::LOW: sev_class = "low"; break;
                }

                oss << "    <div class=\"suggestion " << sev_class << "\">\n";
                oss << "      <div class=\"suggestion-header\">\n";
                oss << "        <h3>" << pattern.pattern_name << "</h3>\n";
                oss << "        <span class=\"badge " << sev_class << "\">"
                    << (sev == Severity::HIGH ? "High Impact" :
                        sev == Severity::MEDIUM ? "Medium Impact" : "Low Impact")
                    << "</span>\n";
                oss << "      </div>\n";

                oss << "      <div class=\"location\">" << pattern.location.file
                    << ":" << pattern.location.line;
                if (!pattern.location.function.empty()) {
                    oss << " (" << pattern.location.function << ")";
                }
                oss << "</div>\n";

                oss << "      <div class=\"description\">" << pattern.description << "</div>\n";

                if (!pattern.why_slow.empty()) {
                    oss << "      <div class=\"why-slow\"><strong>Why it's slow:</strong> "
                        << pattern.why_slow << "</div>\n";
                }

                if (!pattern.current_code.empty()) {
                    oss << "      <div class=\"code-section\">\n";
                    oss << "        <h4>Current Code</h4>\n";
                    oss << "        <pre>" << pattern.current_code << "</pre>\n";
                    oss << "      </div>\n";
                }

                if (!pattern.optimized_code.empty()) {
                    oss << "      <div class=\"code-section optimized\">\n";
                    oss << "        <h4>Recommended Fix</h4>\n";
                    oss << "        <pre>" << pattern.optimized_code << "</pre>\n";
                    oss << "      </div>\n";
                }

                oss << "      <div class=\"speedup-info\"><strong>Expected Speedup:</strong> "
                    << pattern.speedup_string() << "</div>\n";

                if (!pattern.requirements.empty()) {
                    oss << "      <div class=\"requirements\">\n";
                    oss << "        <h4>Requirements</h4>\n";
                    oss << "        <ul>\n";
                    for (const auto& req : pattern.requirements) {
                        oss << "          <li>" << req << "</li>\n";
                    }
                    oss << "        </ul>\n";
                    oss << "      </div>\n";
                }

                oss << "    </div>\n";
            }
        };

        output_patterns(Severity::HIGH, "High Impact Optimizations (10x+ speedup)");
        output_patterns(Severity::MEDIUM, "Medium Impact Optimizations (2-10x speedup)");
        output_patterns(Severity::LOW, "Low Impact Optimizations (&lt;2x speedup)");
    }

    oss << R"(
    <footer>
      <p>Generated by OptiWeave Profiler | <a href="https://github.com/OptiWeave">github.com/OptiWeave</a></p>
    </footer>
  </div>
</body>
</html>
)";

    return oss.str();
}

bool OptimizationAnalyzer::export_to_file(
    const AnalysisResult& result,
    const std::string& filename,
    const std::string& format
) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    if (format == "markdown") {
        file << generate_markdown_report(result);
    } else if (format == "html") {
        file << generate_html_report(result);
    } else if (format == "terminal") {
        file << generate_terminal_output(result);
    } else {
        return false;
    }

    file.close();
    return true;
}

// ============================================================================
// DivisionInLoopDetector Implementation
// ============================================================================

void DivisionInLoopDetector::analyze(
    const hotspots::HotspotTracker& hotspots,
    const optiweave::statistics::OperationCounters& stats,
    const std::vector<LoopInfo>& loop_info
) {
    patterns_.clear();

    // Check each loop for divisions
    for (const auto& loop : loop_info) {
        if (!loop.has_divisions) continue;
        if (!loop.has_constant_divisor) continue;  // Only optimize constant divisors

        // Only flag if it's in a hot spot
        if (loop.total_time_ns < thresholds::kMinHotLoopTimeNs) continue;

        OptimizationPattern pattern;
        pattern.pattern_name = "Division in Hot Loop";
        pattern.location = loop.location;
        pattern.severity = Severity::MEDIUM;
        pattern.category = PatternCategory::ARITHMETIC;

        pattern.description = "Loop contains division by constant value";
        pattern.why_slow = "Division is 8-10x slower than multiplication on most CPUs";
        pattern.rationale = "Compute reciprocal once before loop, multiply inside loop";

        // Example code transformation
        pattern.current_code =
            "for (int i = 0; i < n; i++) {\n"
            "    result[i] = numerator[i] / constant;\n"
            "}";

        pattern.optimized_code =
            "double inv_constant = 1.0 / constant;  // Compute reciprocal once\n"
            "for (int i = 0; i < n; i++) {\n"
            "    result[i] = numerator[i] * inv_constant;  // Multiply instead\n"
            "}";

        pattern.estimated_speedup_min = 3.0f;
        pattern.estimated_speedup_max = 8.0f;

        pattern.operation_count = loop.iteration_count;
        pattern.time_ns = loop.total_time_ns;
        // Note: percentage_of_total needs to be set by caller with total program time

        pattern.requirements.push_back("Ensure divisor is not zero");
        pattern.requirements.push_back("Be aware of potential precision differences");

        patterns_.push_back(pattern);
    }
}

// ============================================================================
// ComplexityDetector Implementation
// ============================================================================

void ComplexityDetector::analyze(
    const hotspots::HotspotTracker& hotspots,
    const optiweave::statistics::OperationCounters& stats,
    const std::vector<LoopInfo>& loop_info
) {
    patterns_.clear();

    // Look for deeply nested loops (potential O(n²) or O(n³))
    for (const auto& loop : loop_info) {
        if (loop.nesting_level < 2) continue;  // Single loops are fine

        // Triple nested loops might be matrix multiplication
        if (loop.nesting_level == 3 && is_matrix_multiply_pattern(loop)) {
            OptimizationPattern pattern;
            pattern.pattern_name = "Naive Matrix Multiplication";
            pattern.location = loop.location;
            pattern.severity = Severity::HIGH;
            pattern.category = PatternCategory::ALGORITHMIC;

            pattern.description = "O(n³) triple-nested loop detected (likely matrix multiplication)";
            pattern.why_slow =
                "No cache blocking, no SIMD vectorization, column-major access causes cache misses";
            pattern.rationale =
                "BLAS implementations use cache-friendly blocking, hand-tuned SIMD kernels, "
                "and multi-threading for large matrices";

            pattern.current_code =
                "for (int i = 0; i < n; i++)\n"
                "    for (int j = 0; j < n; j++)\n"
                "        for (int k = 0; k < n; k++)\n"
                "            C[i*n + j] += A[i*n + k] * B[k*n + j];";

            pattern.optimized_code =
                "#include <cblas.h>\n"
                "\n"
                "cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,\n"
                "            n, n, n,           // dimensions\n"
                "            1.0, A, n,         // alpha, A, lda\n"
                "            B, n,              // B, ldb\n"
                "            0.0, C, n);        // beta, C, ldc";

            pattern.estimated_speedup_min = 20.0f;
            pattern.estimated_speedup_max = 50.0f;

            pattern.operation_count = loop.iteration_count;
            pattern.time_ns = loop.total_time_ns;

            pattern.requirements.push_back("Link with BLAS library (OpenBLAS, MKL, etc.)");
            pattern.requirements.push_back("Install: apt-get install libopenblas-dev");

            pattern.references.push_back("https://www.netlib.org/blas/");
            pattern.references.push_back("https://software.intel.com/mkl");

            patterns_.push_back(pattern);
        }
        // Other O(n²) patterns
        else if (loop.nesting_level == 2 && loop.total_time_ns > thresholds::kMinQuadraticAlgoTimeNs) {
            OptimizationPattern pattern;
            pattern.pattern_name = "O(n²) Algorithm";
            pattern.location = loop.location;
            pattern.severity = Severity::MEDIUM;
            pattern.category = PatternCategory::ALGORITHMIC;

            pattern.description = "Double-nested loop detected - potential O(n²) complexity";
            pattern.why_slow = "Quadratic algorithms scale poorly with input size";
            pattern.rationale = "Consider if there's a more efficient algorithm (sorting, hashing, etc.)";

            pattern.estimated_speedup_min = 2.0f;
            pattern.estimated_speedup_max = 10.0f;

            pattern.time_ns = loop.total_time_ns;

            patterns_.push_back(pattern);
        }
    }
}

bool ComplexityDetector::is_matrix_multiply_pattern(const LoopInfo& loop) const {
    // Heuristic: triple-nested loop with multiplications and additions
    if (loop.nesting_level != 3) return false;

    // Check if loop contains both multiplications and additions
    bool has_mult = false;
    bool has_add = false;

    for (const auto& op : loop.operations_in_loop) {
        if (op == "multiplication") has_mult = true;
        if (op == "addition") has_add = true;
    }

    return has_mult && has_add;
}

// ============================================================================
// MemoryAccessDetector Implementation
// ============================================================================

void MemoryAccessDetector::analyze(
    const hotspots::HotspotTracker& hotspots,
    const optiweave::statistics::OperationCounters& stats,
    const std::vector<LoopInfo>& loop_info
) {
    patterns_.clear();

    for (const auto& loop : loop_info) {
        if (!loop.has_strided_access) continue;
        if (loop.stride_value <= 1) continue;  // Sequential access is fine

        if (has_poor_locality(loop)) {
            OptimizationPattern pattern;
            pattern.pattern_name = "Poor Memory Locality";
            pattern.location = loop.location;
            pattern.severity = Severity::MEDIUM;
            pattern.category = PatternCategory::MEMORY_ACCESS;

            pattern.description =
                "Non-sequential memory access detected (stride = " +
                std::to_string(loop.stride_value) + ")";
            pattern.why_slow = "Large stride causes cache misses and poor memory bandwidth utilization";
            pattern.rationale = "Consider data layout reorganization or cache blocking";

            pattern.estimated_speedup_min = 3.0f;
            pattern.estimated_speedup_max = 5.0f;

            pattern.time_ns = loop.total_time_ns;

            pattern.requirements.push_back("May require data structure reorganization");

            patterns_.push_back(pattern);
        }
    }
}

bool MemoryAccessDetector::has_poor_locality(const LoopInfo& loop) const {
    int cache_line = estimate_cache_line_size();
    // If stride is larger than cache line, we're definitely missing cache
    return loop.stride_value * thresholds::kAssumedElementSize > cache_line;
}

int MemoryAccessDetector::estimate_cache_line_size() const {
    return thresholds::kCacheLineSize;
}

// ============================================================================
// VectorizationDetector Implementation
// ============================================================================

void VectorizationDetector::analyze(
    const hotspots::HotspotTracker& hotspots,
    const optiweave::statistics::OperationCounters& stats,
    const std::vector<LoopInfo>& loop_info
) {
    patterns_.clear();

    for (const auto& loop : loop_info) {
        if (!loop.is_vectorizable) continue;
        if (loop.total_time_ns < thresholds::kMinHotLoopTimeNs) continue;

        if (is_simd_friendly(loop)) {
            OptimizationPattern pattern;
            pattern.pattern_name = "SIMD Vectorization Opportunity";
            pattern.location = loop.location;
            pattern.severity = Severity::MEDIUM;
            pattern.category = PatternCategory::VECTORIZATION;

            pattern.description = "Loop is vectorizable but not using SIMD instructions";
            pattern.why_slow = "Processing one element at a time instead of 4 or 8 in parallel";
            pattern.rationale = "Modern CPUs have SIMD units that can process multiple values simultaneously";

            pattern.current_code =
                "for (int i = 0; i < n; i++) {\n"
                "    result[i] = a[i] + b[i];\n"
                "}";

            pattern.optimized_code =
                "// Option 1: Compiler auto-vectorization\n"
                "#pragma omp simd aligned(a, b, result : 32)\n"
                "for (int i = 0; i < n; i++) {\n"
                "    result[i] = a[i] + b[i];\n"
                "}\n"
                "\n"
                "// Option 2: Manual SIMD (AVX2)\n"
                "for (int i = 0; i < n; i += 4) {\n"
                "    __m256d va = _mm256_load_pd(&a[i]);\n"
                "    __m256d vb = _mm256_load_pd(&b[i]);\n"
                "    __m256d vr = _mm256_add_pd(va, vb);\n"
                "    _mm256_store_pd(&result[i], vr);\n"
                "}";

            pattern.estimated_speedup_min = 2.0f;
            pattern.estimated_speedup_max = 4.0f;

            pattern.time_ns = loop.total_time_ns;

            pattern.requirements.push_back("Compile with: -O3 -march=native -ffast-math");
            pattern.requirements.push_back("Ensure 32-byte alignment: alignas(32) double array[N]");

            patterns_.push_back(pattern);
        }
    }
}

bool VectorizationDetector::is_simd_friendly(const LoopInfo& loop) const {
    // Simple heuristic: if loop is marked as vectorizable
    // TODO: Check operations_in_loop when we populate it during AST analysis
    return loop.is_vectorizable;
}

// ============================================================================
// RepeatedComputationDetector Implementation
// ============================================================================

void RepeatedComputationDetector::analyze(
    const hotspots::HotspotTracker& hotspots,
    const optiweave::statistics::OperationCounters& stats,
    const std::vector<LoopInfo>& loop_info
) {
    patterns_.clear();

    // Look for loops with expensive operations that might be loop-invariant
    // Heuristic: loops with high iteration counts and significant time
    for (const auto& loop : loop_info) {
        if (loop.total_time_ns < thresholds::kMinHotLoopTimeNs) continue;

        // Check if loop likely has loop-invariant code that could be hoisted
        if (has_loop_invariant_code(loop)) {
            OptimizationPattern pattern;
            pattern.pattern_name = "Repeated Computation in Loop";
            pattern.severity = Severity::MEDIUM;
            pattern.category = PatternCategory::ARITHMETIC;
            pattern.location = loop.location;

            pattern.description = "Loop contains repeated computations that could be hoisted outside the loop";
            pattern.why_slow = "The same calculation is performed on every iteration, wasting CPU cycles";

            pattern.current_code =
                "for (int i = 0; i < n; i++) {\n"
                "    result[i] = data[i] * (expensive_function() + constant);\n"
                "}";

            pattern.optimized_code =
                "// Compute loop-invariant values once\n"
                "auto invariant = expensive_function() + constant;\n"
                "for (int i = 0; i < n; i++) {\n"
                "    result[i] = data[i] * invariant;\n"
                "}";

            pattern.estimated_speedup_min = 1.5f;
            pattern.estimated_speedup_max = 3.0f;
            pattern.time_ns = loop.total_time_ns;

            pattern.rationale = "Loop-invariant code motion (LICM) eliminates redundant computation. "
                              "Compilers may not optimize this if the code involves function calls or "
                              "if optimization level is low.";

            pattern.requirements.push_back("Ensure loop-invariant expressions have no side effects");
            pattern.requirements.push_back("Verify function calls are safe to hoist (no state changes)");

            patterns_.push_back(pattern);
        }
    }
}

bool RepeatedComputationDetector::has_loop_invariant_code(const LoopInfo& loop) const {
    // Simple heuristic: loops with high iteration counts and significant time
    // are likely candidates for loop-invariant code motion
    // TODO: Proper data flow analysis would be better

    // Only flag loops that spend significant time (>100μs) AND have many iterations (>5000)
    // This avoids false positives on already-optimized loops
    return loop.total_time_ns > thresholds::kMinLoopInvariantTimeNs &&
           loop.iteration_count > thresholds::kMinLoopInvariantIterations;
}

// ============================================================================
// BranchMispredictionDetector Implementation
// ============================================================================

void BranchMispredictionDetector::analyze(
    const hotspots::HotspotTracker& hotspots,
    const optiweave::statistics::OperationCounters& stats,
    const std::vector<LoopInfo>& loop_info
) {
    patterns_.clear();

    // Look for loops with many comparisons - potential branch-heavy code
    // Branch mispredictions are expensive (10-20 cycle penalty)
    // Use ratio of comparisons to total operations as a heuristic
    uint64_t total_ops = stats.array_subscript.load() +
                        stats.addition.load() +
                        stats.multiplication.load() +
                        stats.division.load();

    uint64_t comparison_ops = stats.less_than.load() +
                             stats.greater_than.load() +
                             stats.equal.load() +
                             stats.not_equal.load();

    if (total_ops == 0) return;

    // If comparisons are >15% of operations, might have branch issues
    double comparison_ratio = static_cast<double>(comparison_ops) / total_ops;

    if (comparison_ratio > thresholds::kBranchComparisonRatioThreshold) {
        // Look for loops that might benefit from branchless code
        for (const auto& loop : loop_info) {
            if (loop.total_time_ns < thresholds::kMinBranchAnalysisTimeNs) continue;
            if (loop.iteration_count < thresholds::kMinBranchOptIterations) continue;

            if (likely_has_unpredictable_branches(loop)) {
                OptimizationPattern pattern;
                pattern.pattern_name = "Potential Branch Misprediction";
                pattern.severity = Severity::MEDIUM;
                pattern.category = PatternCategory::ARITHMETIC;
                pattern.location = loop.location;

                pattern.description = "Loop contains many comparisons that may cause branch mispredictions";
                pattern.why_slow = "Unpredictable branches cause CPU pipeline stalls (10-20 cycle penalty per misprediction)";

                pattern.current_code =
                    "for (int i = 0; i < n; i++) {\n"
                    "    if (data[i] > threshold) {\n"
                    "        result[i] = data[i] * 2;\n"
                    "    } else {\n"
                    "        result[i] = data[i] / 2;\n"
                    "    }\n"
                    "}";

                pattern.optimized_code =
                    "// Option 1: Branchless code using conditional moves\n"
                    "for (int i = 0; i < n; i++) {\n"
                    "    bool cond = data[i] > threshold;\n"
                    "    result[i] = cond ? (data[i] * 2) : (data[i] / 2);\n"
                    "}\n"
                    "\n"
                    "// Option 2: SIMD with blend instructions\n"
                    "for (int i = 0; i < n; i += 4) {\n"
                    "    __m256d v = _mm256_load_pd(&data[i]);\n"
                    "    __m256d mask = _mm256_cmp_pd(v, thresh_vec, _CMP_GT_OQ);\n"
                    "    __m256d mul_result = _mm256_mul_pd(v, two_vec);\n"
                    "    __m256d div_result = _mm256_div_pd(v, two_vec);\n"
                    "    __m256d result = _mm256_blendv_pd(div_result, mul_result, mask);\n"
                    "    _mm256_store_pd(&result[i], result);\n"
                    "}";

                pattern.estimated_speedup_min = 1.3f;
                pattern.estimated_speedup_max = 3.0f;
                pattern.time_ns = loop.total_time_ns;

                pattern.rationale = "Branch mispredictions cause pipeline stalls. Modern CPUs have "
                                  "conditional move instructions (CMOV) and SIMD blend instructions "
                                  "that eliminate branches. This works best when the branch is unpredictable.";

                pattern.requirements.push_back("Profile to verify branch misprediction rate is high");
                pattern.requirements.push_back("Ensure data is aligned for SIMD (32-byte alignment)");
                pattern.requirements.push_back("Compile with: -O3 -march=native");

                patterns_.push_back(pattern);
            }
        }
    }
}

bool BranchMispredictionDetector::likely_has_unpredictable_branches(const LoopInfo& loop) const {
    // Heuristic: loops with high iteration counts are candidates
    // In practice, we'd want to check:
    // - Actual branch prediction miss rate (requires perf counters)
    // - Pattern of branches (random vs predictable)
    // For now, use a simple heuristic: high iteration count suggests benefit from branchless code
    return loop.iteration_count > thresholds::kMinBranchOptIterations;
}

} // namespace analysis
} // namespace optiweave
