#include <gtest/gtest.h>
#include <optiweave/core/rule_registry.hpp>
#include <optiweave/core/safety_tier.hpp>
#include <optiweave/analysis/pattern_names.hpp>

using namespace optiweave::core;

class RuleRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure rules are registered (idempotent via static singleton)
        register_builtin_rules();
    }
};

// --- FixRuleRegistry Tests ---

TEST_F(RuleRegistryTest, AllFixRulesRegistered) {
    auto& reg = FixRuleRegistry::instance();
    // We expect 13 fix rules
    EXPECT_GE(reg.all_rules().size(), 13u);
}

TEST_F(RuleRegistryTest, FindFixRuleById) {
    auto& reg = FixRuleRegistry::instance();
    EXPECT_NE(reg.find_rule("unsigned-wraparound"), nullptr);
    EXPECT_NE(reg.find_rule("signed-negation"), nullptr);
    EXPECT_NE(reg.find_rule("division-by-zero"), nullptr);
    EXPECT_NE(reg.find_rule("dangling-else"), nullptr);
    EXPECT_EQ(reg.find_rule("nonexistent-rule"), nullptr);
}

TEST_F(RuleRegistryTest, FixRuleSafetyTiers) {
    auto& reg = FixRuleRegistry::instance();

    auto* uninit = reg.find_rule("uninitialized-var");
    ASSERT_NE(uninit, nullptr);
    EXPECT_EQ(uninit->safety_tier(), SafetyTier::SAFE);

    auto* fp_eq = reg.find_rule("fp-equality");
    ASSERT_NE(fp_eq, nullptr);
    EXPECT_EQ(fp_eq->safety_tier(), SafetyTier::AGGRESSIVE);
}

TEST_F(RuleRegistryTest, RulesForBinaryOperator) {
    auto& reg = FixRuleRegistry::instance();
    auto& rules = reg.rules_for(ASTNodeKind::BinaryOperator);
    EXPECT_GT(rules.size(), 0u);
}

TEST_F(RuleRegistryTest, RulesForUnknownKindReturnsEmpty) {
    auto& reg = FixRuleRegistry::instance();
    // IfStmt may have rules, but let's just test the mechanism
    auto& empty = reg.rules_for(static_cast<ASTNodeKind>(999));
    EXPECT_EQ(empty.size(), 0u);
}

// --- PatchRuleRegistry Tests ---

TEST_F(RuleRegistryTest, AllPatchRulesRegistered) {
    auto& reg = PatchRuleRegistry::instance();
    // We expect 8 patch rules
    EXPECT_GE(reg.all_rules().size(), 8u);
}

TEST_F(RuleRegistryTest, FindPatchRuleById) {
    auto& reg = PatchRuleRegistry::instance();
    EXPECT_NE(reg.find_rule("division-reciprocal"), nullptr);
    EXPECT_NE(reg.find_rule("vectorization-pragma"), nullptr);
    EXPECT_NE(reg.find_rule("strength-reduction"), nullptr);
    EXPECT_NE(reg.find_rule("prefetch-hint"), nullptr);
    EXPECT_NE(reg.find_rule("loop-interchange"), nullptr);
    EXPECT_NE(reg.find_rule("loop-unroll-hint"), nullptr);
    EXPECT_NE(reg.find_rule("restrict-qualifier"), nullptr);
    EXPECT_EQ(reg.find_rule("nonexistent-rule"), nullptr);
}

TEST_F(RuleRegistryTest, PatchRuleSafetyTiers) {
    auto& reg = PatchRuleRegistry::instance();

    auto* prefetch = reg.find_rule("prefetch-hint");
    ASSERT_NE(prefetch, nullptr);
    EXPECT_EQ(prefetch->safety_tier(), SafetyTier::SAFE);

    auto* interchange = reg.find_rule("loop-interchange");
    ASSERT_NE(interchange, nullptr);
    EXPECT_EQ(interchange->safety_tier(), SafetyTier::AGGRESSIVE);

    auto* strength = reg.find_rule("strength-reduction");
    ASSERT_NE(strength, nullptr);
    EXPECT_EQ(strength->safety_tier(), SafetyTier::MOSTLY_SAFE);
}

TEST_F(RuleRegistryTest, RulesForPattern) {
    auto& reg = PatchRuleRegistry::instance();

    auto div_rules = reg.rules_for_pattern(
        optiweave::analysis::patterns::kDivisionInHotLoop);
    EXPECT_GT(div_rules.size(), 0u);

    auto sr_rules = reg.rules_for_pattern(
        optiweave::analysis::patterns::kStrengthReduction);
    EXPECT_GT(sr_rules.size(), 0u);

    auto none = reg.rules_for_pattern("nonexistent-pattern");
    EXPECT_EQ(none.size(), 0u);
}

TEST_F(RuleRegistryTest, PatchRuleHandledPatterns) {
    auto& reg = PatchRuleRegistry::instance();

    auto* restrict_rule = reg.find_rule("restrict-qualifier");
    ASSERT_NE(restrict_rule, nullptr);
    auto patterns = restrict_rule->handled_patterns();
    EXPECT_EQ(patterns.size(), 1u);
    EXPECT_EQ(patterns[0], optiweave::analysis::patterns::kRestrictQualifier);
    EXPECT_TRUE(restrict_rule->is_function_level());
}
