#pragma once

#include <optiweave/core/safety_tier.hpp>
#include <cstdint>
#include <string>
#include <sstream>

namespace optiweave {
namespace core {

struct RuleConfig {
    // Safety and confidence filtering
    SafetyTier max_safety_tier = SafetyTier::AGGRESSIVE;
    double min_confidence = 0.0;

    // Pattern detector thresholds (overridable via --rule-config)
    uint64_t min_hot_loop_time_ns = 10000;           // 10 us
    uint64_t min_quadratic_algo_time_ns = 10000000;  // 10 ms
    uint64_t min_branch_analysis_time_ns = 50000;    // 50 us
    uint64_t min_loop_invariant_time_ns = 100000;    // 100 us
    uint64_t min_loop_invariant_iterations = 5000;
    uint64_t min_branch_opt_iterations = 1000;
    double branch_comparison_ratio_threshold = 0.15;
    int cache_line_size = 64;
    int assumed_element_size = 8;

    // New rule thresholds
    int loop_unroll_max_body_stmts = 8;
    int loop_unroll_max_trip_count = 64;
    int prefetch_min_stride = 64;   // bytes

    /// Parse "key=val,key=val" overrides from --rule-config
    void apply_overrides(const std::string& config_str) {
        if (config_str.empty()) return;
        std::istringstream ss(config_str);
        std::string token;
        while (std::getline(ss, token, ',')) {
            auto eq = token.find('=');
            if (eq == std::string::npos) continue;
            std::string key = token.substr(0, eq);
            std::string val = token.substr(eq + 1);
            // Trim
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            val.erase(0, val.find_first_not_of(" \t"));
            val.erase(val.find_last_not_of(" \t") + 1);

            if (key == "min_hot_loop_time_ns")          min_hot_loop_time_ns = std::stoull(val);
            else if (key == "min_quadratic_algo_time_ns") min_quadratic_algo_time_ns = std::stoull(val);
            else if (key == "min_branch_analysis_time_ns") min_branch_analysis_time_ns = std::stoull(val);
            else if (key == "min_loop_invariant_time_ns")  min_loop_invariant_time_ns = std::stoull(val);
            else if (key == "min_loop_invariant_iterations") min_loop_invariant_iterations = std::stoull(val);
            else if (key == "min_branch_opt_iterations")   min_branch_opt_iterations = std::stoull(val);
            else if (key == "branch_comparison_ratio")     branch_comparison_ratio_threshold = std::stod(val);
            else if (key == "cache_line_size")             cache_line_size = std::stoi(val);
            else if (key == "assumed_element_size")        assumed_element_size = std::stoi(val);
            else if (key == "loop_unroll_max_body_stmts")  loop_unroll_max_body_stmts = std::stoi(val);
            else if (key == "loop_unroll_max_trip_count")  loop_unroll_max_trip_count = std::stoi(val);
            else if (key == "prefetch_min_stride")         prefetch_min_stride = std::stoi(val);
        }
    }
};

} // namespace core
} // namespace optiweave
