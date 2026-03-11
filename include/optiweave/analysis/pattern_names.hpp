#pragma once

#include <string_view>

namespace optiweave::analysis::patterns {

inline constexpr std::string_view kDivisionInHotLoop   = "Division in Hot Loop";
inline constexpr std::string_view kSIMDVectorization   = "SIMD Vectorization Opportunity";
inline constexpr std::string_view kNaiveMatrixMultiply = "Naive Matrix Multiplication";
inline constexpr std::string_view kQuadraticAlgorithm  = "O(n\u00b2) Algorithm";
inline constexpr std::string_view kPoorMemoryLocality  = "Poor Memory Locality";
inline constexpr std::string_view kRepeatedComputation = "Repeated Computation in Loop";
inline constexpr std::string_view kBranchMisprediction = "Potential Branch Misprediction";

// New patterns (Phase 6)
inline constexpr std::string_view kLoopInterchange    = "Loop Interchange Opportunity";
inline constexpr std::string_view kStrengthReduction  = "Strength Reduction Opportunity";
inline constexpr std::string_view kLoopUnrollHint     = "Loop Unroll Hint";
inline constexpr std::string_view kPrefetchHint       = "Prefetch Hint Opportunity";
inline constexpr std::string_view kRestrictQualifier  = "Restrict Qualifier Opportunity";

} // namespace optiweave::analysis::patterns
