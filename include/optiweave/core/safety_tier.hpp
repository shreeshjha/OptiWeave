#pragma once

#include <string>

namespace optiweave {
namespace core {

enum class SafetyTier {
    SAFE,         // No semantic change possible (e.g., add initializer, add braces)
    MOSTLY_SAFE,  // Correct for well-defined code, may change UB behavior
    AGGRESSIVE,   // May change observable behavior (e.g., FP equality, reciprocal)
    ADVISORY      // Comment-only, no code change
};

inline std::string safetyTierToString(SafetyTier tier) {
    switch (tier) {
    case SafetyTier::SAFE:        return "safe";
    case SafetyTier::MOSTLY_SAFE: return "mostly-safe";
    case SafetyTier::AGGRESSIVE:  return "aggressive";
    case SafetyTier::ADVISORY:    return "advisory";
    }
    return "unknown";
}

inline SafetyTier parseSafetyTier(const std::string& s) {
    if (s == "safe")        return SafetyTier::SAFE;
    if (s == "mostly-safe") return SafetyTier::MOSTLY_SAFE;
    if (s == "aggressive")  return SafetyTier::AGGRESSIVE;
    if (s == "advisory")    return SafetyTier::ADVISORY;
    return SafetyTier::AGGRESSIVE; // default
}

} // namespace core
} // namespace optiweave
