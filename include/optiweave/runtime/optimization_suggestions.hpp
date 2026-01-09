#pragma once

#include <optiweave/analysis/pattern_detector.hpp>
#include <optiweave/runtime/hotspot_tracker.hpp>
#include <optiweave/runtime/statistics.hpp>

namespace optiweave {
namespace optimization {

/// Global enable flag
extern bool g_suggestions_enabled;

/// Initialize optimization suggestion system
void initialize();

/// Finalize and generate optimization report
void finalize();

/// Get the global optimization analyzer
analysis::OptimizationAnalyzer& get_analyzer();

} // namespace optimization
} // namespace optiweave
