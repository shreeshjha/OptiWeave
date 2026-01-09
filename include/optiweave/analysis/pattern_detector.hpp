#pragma once

#include <optiweave/analysis/optimization_pattern.hpp>
#include <optiweave/runtime/hotspot_tracker.hpp>
#include <optiweave/runtime/statistics.hpp>
#include <memory>
#include <vector>

namespace optiweave {
namespace analysis {

/// Loop information collected during AST analysis
struct LoopInfo {
    SourceLocation location;        // Location of loop header
    int line_start = 0;             // First line of loop body
    int line_end = 0;               // Last line of loop body
    int nesting_level;              // 1 for outermost, 2 for nested, etc.
    bool has_divisions;             // Does loop contain divisions?
    bool has_constant_divisor;      // Division by loop-invariant value?
    bool has_strided_access;        // Non-sequential memory access?
    int stride_value;               // Stride size (if constant)
    bool is_vectorizable;           // No loop-carried dependencies?
    std::vector<std::string> operations_in_loop;  // Operation types in loop body

    // Set at runtime
    uint64_t iteration_count = 0;   // How many times loop executed
    uint64_t total_time_ns = 0;     // Time spent in loop

    // Check if a line is within this loop's body
    bool contains_line(int line) const {
        return line >= line_start && line <= line_end;
    }
};

/// Base class for pattern detectors
class PatternDetector {
public:
    virtual ~PatternDetector() = default;

    /// Analyze and detect patterns
    /// @param hotspots Hotspot data from runtime profiling
    /// @param stats Operation statistics
    /// @param loop_info Loop information from AST analysis (optional)
    virtual void analyze(
        const hotspots::HotspotTracker& hotspots,
        const optiweave::statistics::OperationCounters& stats,
        const std::vector<LoopInfo>& loop_info = {}
    ) = 0;

    /// Get detected patterns
    virtual std::vector<OptimizationPattern> get_patterns() const = 0;

    /// Get detector name
    virtual std::string name() const = 0;

protected:
    std::vector<OptimizationPattern> patterns_;
};

/// Main analyzer that coordinates all detectors
class OptimizationAnalyzer {
public:
    OptimizationAnalyzer();

    /// Register a pattern detector
    void register_detector(std::unique_ptr<PatternDetector> detector);

    /// Run analysis with all registered detectors
    AnalysisResult analyze(
        const hotspots::HotspotTracker& hotspots,
        const optiweave::statistics::OperationCounters& stats,
        const std::vector<LoopInfo>& loop_info = {}
    );

    /// Generate markdown report
    std::string generate_markdown_report(const AnalysisResult& result) const;

    /// Generate HTML report
    std::string generate_html_report(const AnalysisResult& result) const;

    /// Generate terminal output
    std::string generate_terminal_output(const AnalysisResult& result) const;

    /// Export to file
    bool export_to_file(
        const AnalysisResult& result,
        const std::string& filename,
        const std::string& format  // "markdown", "html", "terminal"
    ) const;

private:
    std::vector<std::unique_ptr<PatternDetector>> detectors_;

    // Helper methods for report generation
    std::string format_code_block(const std::string& code, const std::string& language = "cpp") const;
    std::string format_severity_section(const AnalysisResult& result, Severity severity) const;
};

/// Detector for divisions in hot loops
class DivisionInLoopDetector : public PatternDetector {
public:
    void analyze(
        const hotspots::HotspotTracker& hotspots,
        const optiweave::statistics::OperationCounters& stats,
        const std::vector<LoopInfo>& loop_info
    ) override;

    std::vector<OptimizationPattern> get_patterns() const override {
        return patterns_;
    }

    std::string name() const override {
        return "Division in Loop Detector";
    }
};

/// Detector for O(n²) and O(n³) complexity patterns
class ComplexityDetector : public PatternDetector {
public:
    void analyze(
        const hotspots::HotspotTracker& hotspots,
        const optiweave::statistics::OperationCounters& stats,
        const std::vector<LoopInfo>& loop_info
    ) override;

    std::vector<OptimizationPattern> get_patterns() const override {
        return patterns_;
    }

    std::string name() const override {
        return "Complexity Detector";
    }

private:
    bool is_matrix_multiply_pattern(const LoopInfo& loop) const;
};

/// Detector for non-sequential memory access patterns
class MemoryAccessDetector : public PatternDetector {
public:
    void analyze(
        const hotspots::HotspotTracker& hotspots,
        const optiweave::statistics::OperationCounters& stats,
        const std::vector<LoopInfo>& loop_info
    ) override;

    std::vector<OptimizationPattern> get_patterns() const override {
        return patterns_;
    }

    std::string name() const override {
        return "Memory Access Detector";
    }

private:
    bool has_poor_locality(const LoopInfo& loop) const;
    int estimate_cache_line_size() const;
};

/// Detector for vectorization opportunities
class VectorizationDetector : public PatternDetector {
public:
    void analyze(
        const hotspots::HotspotTracker& hotspots,
        const optiweave::statistics::OperationCounters& stats,
        const std::vector<LoopInfo>& loop_info
    ) override;

    std::vector<OptimizationPattern> get_patterns() const override {
        return patterns_;
    }

    std::string name() const override {
        return "Vectorization Detector";
    }

private:
    bool is_simd_friendly(const LoopInfo& loop) const;
};

/// Detector for repeated computation in loops
class RepeatedComputationDetector : public PatternDetector {
public:
    void analyze(
        const hotspots::HotspotTracker& hotspots,
        const optiweave::statistics::OperationCounters& stats,
        const std::vector<LoopInfo>& loop_info
    ) override;

    std::vector<OptimizationPattern> get_patterns() const override {
        return patterns_;
    }

    std::string name() const override {
        return "Repeated Computation Detector";
    }

private:
    bool has_loop_invariant_code(const LoopInfo& loop) const;
};

/// Detector for branch misprediction opportunities
class BranchMispredictionDetector : public PatternDetector {
public:
    void analyze(
        const hotspots::HotspotTracker& hotspots,
        const optiweave::statistics::OperationCounters& stats,
        const std::vector<LoopInfo>& loop_info
    ) override;

    std::vector<OptimizationPattern> get_patterns() const override {
        return patterns_;
    }

    std::string name() const override {
        return "Branch Misprediction Detector";
    }

private:
    bool likely_has_unpredictable_branches(const LoopInfo& loop) const;
};

} // namespace analysis
} // namespace optiweave
