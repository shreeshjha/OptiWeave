#include <gtest/gtest.h>
#include <optiweave/core/rule_config.hpp>

using namespace optiweave::core;

TEST(RuleConfigTest, DefaultValuesMatchOldConstants) {
    RuleConfig config;
    EXPECT_EQ(config.min_hot_loop_time_ns, 10000u);
    EXPECT_EQ(config.min_quadratic_algo_time_ns, 10000000u);
    EXPECT_EQ(config.min_branch_analysis_time_ns, 50000u);
    EXPECT_EQ(config.min_loop_invariant_time_ns, 100000u);
    EXPECT_EQ(config.min_loop_invariant_iterations, 5000u);
    EXPECT_EQ(config.min_branch_opt_iterations, 1000u);
    EXPECT_DOUBLE_EQ(config.branch_comparison_ratio_threshold, 0.15);
    EXPECT_EQ(config.cache_line_size, 64);
    EXPECT_EQ(config.assumed_element_size, 8);
    EXPECT_EQ(config.loop_unroll_max_body_stmts, 8);
    EXPECT_EQ(config.loop_unroll_max_trip_count, 64);
    EXPECT_EQ(config.prefetch_min_stride, 64);
}

TEST(RuleConfigTest, ApplyOverridesParsesCorrectly) {
    RuleConfig config;
    config.apply_overrides("min_hot_loop_time_ns=1,cache_line_size=128");
    EXPECT_EQ(config.min_hot_loop_time_ns, 1u);
    EXPECT_EQ(config.cache_line_size, 128);
    // Other values remain default
    EXPECT_EQ(config.min_quadratic_algo_time_ns, 10000000u);
}

TEST(RuleConfigTest, ApplyOverridesHandlesWhitespace) {
    RuleConfig config;
    config.apply_overrides(" assumed_element_size = 16 , prefetch_min_stride = 32 ");
    EXPECT_EQ(config.assumed_element_size, 16);
    EXPECT_EQ(config.prefetch_min_stride, 32);
}

TEST(RuleConfigTest, ApplyOverridesIgnoresUnknownKeys) {
    RuleConfig config;
    config.apply_overrides("unknown_key=42,min_hot_loop_time_ns=5000");
    EXPECT_EQ(config.min_hot_loop_time_ns, 5000u);
    // Should not crash or modify anything unexpected
}

TEST(RuleConfigTest, ApplyOverridesHandlesEmptyString) {
    RuleConfig config;
    config.apply_overrides("");
    EXPECT_EQ(config.min_hot_loop_time_ns, 10000u);
}

TEST(RuleConfigTest, ApplyOverridesHandlesMalformedInput) {
    RuleConfig config;
    // No equals sign
    config.apply_overrides("garbage,also_garbage");
    EXPECT_EQ(config.min_hot_loop_time_ns, 10000u);
}

TEST(RuleConfigTest, ApplyOverridesDouble) {
    RuleConfig config;
    config.apply_overrides("branch_comparison_ratio=0.25");
    EXPECT_DOUBLE_EQ(config.branch_comparison_ratio_threshold, 0.25);
}

TEST(RuleConfigTest, SafetyTierDefault) {
    RuleConfig config;
    EXPECT_EQ(config.max_safety_tier, SafetyTier::AGGRESSIVE);
    EXPECT_DOUBLE_EQ(config.min_confidence, 0.0);
}
