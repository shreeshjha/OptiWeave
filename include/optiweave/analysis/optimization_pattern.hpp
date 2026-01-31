#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <optiweave/runtime/hotspot_tracker.hpp>

namespace optiweave {
namespace analysis {

// Import types from hotspots namespace for convenience
using SourceLocation = hotspots::SourceLocation;

/// Severity level of optimization opportunity
enum class Severity {
    HIGH,      // 10x+ potential speedup
    MEDIUM,    // 2-10x potential speedup
    LOW        // < 2x potential speedup
};

/// Category of optimization pattern
enum class PatternCategory {
    ALGORITHMIC,        // Algorithm-level improvements (BLAS, FFT, etc.)
    MEMORY_ACCESS,      // Cache, memory layout, blocking
    ARITHMETIC,         // Division elimination, strength reduction
    VECTORIZATION,      // SIMD opportunities
    PARALLELIZATION,    // Multi-threading opportunities
    PRECISION,          // Numerical stability, overflow risks
    OTHER
};

/// Represents a detected optimization pattern and suggestion
struct OptimizationPattern {
    // Identification
    std::string pattern_name;
    SourceLocation location;
    Severity severity;
    PatternCategory category;

    // Description
    std::string description;          // What was detected
    std::string why_slow;             // Why current code is slow
    std::string rationale;            // Why suggestion helps

    // Code snippets
    std::string current_code;         // Current implementation
    std::string optimized_code;       // Suggested optimization
    std::vector<std::string> requirements;  // Requirements for optimization (e.g., "-O3 -march=native")

    // Impact estimation
    float estimated_speedup_min;      // Conservative estimate
    float estimated_speedup_max;      // Optimistic estimate

    // Supporting data
    uint64_t operation_count;         // Operations at this location
    uint64_t time_ns;                 // Time spent at this location
    double percentage_of_total;       // % of total execution time

    // Additional context
    std::vector<std::string> references;  // Links to documentation, papers, etc.

    /// Get severity as string
    std::string severity_string() const {
        switch (severity) {
            case Severity::HIGH:   return "HIGH";
            case Severity::MEDIUM: return "MEDIUM";
            case Severity::LOW:    return "LOW";
        }
        return "UNKNOWN";
    }

    /// Get severity emoji
    std::string severity_emoji() const {
        switch (severity) {
            case Severity::HIGH:   return "🔥";
            case Severity::MEDIUM: return "⚡";
            case Severity::LOW:    return "💡";
        }
        return "❓";
    }

    /// Get category as string
    std::string category_string() const {
        switch (category) {
            case PatternCategory::ALGORITHMIC:     return "Algorithmic";
            case PatternCategory::MEMORY_ACCESS:   return "Memory Access";
            case PatternCategory::ARITHMETIC:      return "Arithmetic";
            case PatternCategory::VECTORIZATION:   return "Vectorization";
            case PatternCategory::PARALLELIZATION: return "Parallelization";
            case PatternCategory::PRECISION:       return "Precision";
            case PatternCategory::OTHER:           return "Other";
        }
        return "Unknown";
    }

    /// Get estimated speedup range as string
    std::string speedup_string() const {
        if (estimated_speedup_max > estimated_speedup_min + 0.1f) {
            return std::to_string(static_cast<int>(estimated_speedup_min)) + "-" +
                   std::to_string(static_cast<int>(estimated_speedup_max)) + "x";
        } else {
            return std::to_string(static_cast<int>(estimated_speedup_min)) + "x";
        }
    }
};

/// Result of pattern detection analysis
struct AnalysisResult {
    std::vector<OptimizationPattern> patterns;

    // Summary statistics
    size_t high_impact_count = 0;
    size_t medium_impact_count = 0;
    size_t low_impact_count = 0;
    float combined_speedup_min = 1.0f;
    float combined_speedup_max = 1.0f;

    /// Add pattern and update statistics
    void add_pattern(const OptimizationPattern& pattern) {
        patterns.push_back(pattern);

        switch (pattern.severity) {
            case Severity::HIGH:
                high_impact_count++;
                break;
            case Severity::MEDIUM:
                medium_impact_count++;
                break;
            case Severity::LOW:
                low_impact_count++;
                break;
        }

        // Conservative estimate: multiply speedups (assuming independent optimizations)
        // This is optimistic; real-world speedups don't always multiply
        // A more conservative approach would be to sum them or take max
        combined_speedup_min *= pattern.estimated_speedup_min;
        combined_speedup_max *= pattern.estimated_speedup_max;
    }

    /// Sort patterns by severity (high to low) and then by time percentage
    void sort_by_impact() {
        std::sort(patterns.begin(), patterns.end(),
            [](const OptimizationPattern& a, const OptimizationPattern& b) {
                // Sort by severity first (high > medium > low)
                if (a.severity != b.severity) {
                    return static_cast<int>(a.severity) < static_cast<int>(b.severity);
                }
                // Then by percentage of total time
                return a.percentage_of_total > b.percentage_of_total;
            });
    }

    /// Get total number of patterns
    size_t total_count() const {
        return patterns.size();
    }
};

} // namespace analysis
} // namespace optiweave
