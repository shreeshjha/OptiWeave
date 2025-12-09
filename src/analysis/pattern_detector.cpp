#include <optiweave/analysis/pattern_detector.hpp>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

namespace optiweave {
namespace analysis {

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
    // Similar to markdown but with HTML formatting
    // Implementation omitted for brevity - would generate full HTML with CSS
    return "<html><!-- HTML report implementation --></html>";
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
        if (loop.total_time_ns < 10000) continue;  // Less than 10μs - not worth it (lowered for testing)

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
        else if (loop.nesting_level == 2 && loop.total_time_ns > 10000000) {  // > 10ms
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
    return loop.stride_value * 8 > cache_line;  // Assuming 8-byte elements
}

int MemoryAccessDetector::estimate_cache_line_size() const {
    return 64;  // Typical on x86_64
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
        if (loop.total_time_ns < 10000) continue;  // < 10μs, not worth it (lowered for testing)

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
        if (loop.total_time_ns < 10000) continue;  // Less than 10μs, skip

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
    return loop.total_time_ns > 100000 && loop.iteration_count > 5000;
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

    if (comparison_ratio > 0.15) {
        // Look for loops that might benefit from branchless code
        for (const auto& loop : loop_info) {
            if (loop.total_time_ns < 50000) continue;  // Less than 50μs, skip
            if (loop.iteration_count < 1000) continue;  // Small loops unlikely to benefit

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
    return loop.iteration_count > 1000;
}

} // namespace analysis
} // namespace optiweave
