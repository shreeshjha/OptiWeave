#pragma once

#include <optiweave/core/safety_tier.hpp>
#include <string>

namespace optiweave {
namespace core {

struct RuleResult {
    bool applied = false;
    double confidence = 0.0;   // 0.0–1.0; exact line match = 1.0
    SafetyTier tier = SafetyTier::SAFE;
    std::string reason;        // Human-readable explanation
    std::string original;      // Original source text (for reporting)
    std::string replacement;   // Replacement text (for reporting)
    std::string header;        // Header to inject (empty = none)

    static RuleResult skip(const std::string& reason = "") {
        return {false, 0.0, SafetyTier::SAFE, reason, {}, {}, {}};
    }

    static RuleResult success(double confidence, SafetyTier tier,
                              const std::string& reason,
                              const std::string& original = {},
                              const std::string& replacement = {},
                              const std::string& header = {}) {
        return {true, confidence, tier, reason, original, replacement, header};
    }
};

} // namespace core
} // namespace optiweave
