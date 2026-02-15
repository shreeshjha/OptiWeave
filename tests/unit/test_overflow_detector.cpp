#include "optiweave/analysis/overflow_detector.hpp"
#include <gtest/gtest.h>

using namespace optiweave::analysis;

class OverflowDetectorTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Test setup
  }
};

// Test OverflowType enum values exist
TEST_F(OverflowDetectorTest, OverflowTypeEnumValues) {
  OverflowType signed_add = OverflowType::SIGNED_ADDITION;
  OverflowType signed_sub = OverflowType::SIGNED_SUBTRACTION;
  OverflowType signed_mul = OverflowType::SIGNED_MULTIPLICATION;
  OverflowType signed_div = OverflowType::SIGNED_DIVISION;
  OverflowType signed_neg = OverflowType::SIGNED_NEGATION;
  OverflowType signed_shift = OverflowType::SIGNED_LEFT_SHIFT;
  OverflowType unsigned_wrap = OverflowType::UNSIGNED_WRAPAROUND;
  OverflowType mixed_sign = OverflowType::MIXED_SIGNEDNESS;
  OverflowType narrow = OverflowType::NARROWING_CONVERSION;
  OverflowType loop_overflow = OverflowType::LOOP_COUNTER_OVERFLOW;

  // Verify we can assign and compare enum values
  EXPECT_EQ(signed_add, OverflowType::SIGNED_ADDITION);
  EXPECT_EQ(loop_overflow, OverflowType::LOOP_COUNTER_OVERFLOW);
  EXPECT_NE(signed_add, signed_sub);
}

// Test OverflowSeverity enum values
TEST_F(OverflowDetectorTest, OverflowSeverityEnumValues) {
  OverflowSeverity critical = OverflowSeverity::CRITICAL;
  OverflowSeverity warning = OverflowSeverity::WARNING;
  OverflowSeverity info = OverflowSeverity::INFO;

  EXPECT_EQ(critical, OverflowSeverity::CRITICAL);
  EXPECT_EQ(warning, OverflowSeverity::WARNING);
  EXPECT_EQ(info, OverflowSeverity::INFO);
  EXPECT_NE(critical, warning);
}

// Test OverflowIssue structure initialization
TEST_F(OverflowDetectorTest, OverflowIssueStructure) {
  OverflowIssue issue{};  // Use aggregate initialization to zero-initialize

  // Test default construction - strings should be empty
  EXPECT_TRUE(issue.file.empty());
  EXPECT_EQ(issue.line, 0u);
  EXPECT_EQ(issue.column, 0u);
  EXPECT_TRUE(issue.function.empty());
  EXPECT_TRUE(issue.description.empty());
  EXPECT_TRUE(issue.code_snippet.empty());
  EXPECT_TRUE(issue.suggestion.empty());
  EXPECT_TRUE(issue.operator_str.empty());
  EXPECT_TRUE(issue.lhs_type.empty());
  EXPECT_TRUE(issue.rhs_type.empty());
  EXPECT_FALSE(issue.is_constant_expr);
  EXPECT_FALSE(issue.is_in_loop);
}

// Test OverflowIssue with values
TEST_F(OverflowDetectorTest, OverflowIssueWithValues) {
  OverflowIssue issue;
  issue.type = OverflowType::SIGNED_ADDITION;
  issue.severity = OverflowSeverity::CRITICAL;
  issue.file = "test.cpp";
  issue.line = 42;
  issue.column = 10;
  issue.function = "foo";
  issue.description = "Potential signed overflow";
  issue.operator_str = "+";
  issue.lhs_type = "int";
  issue.rhs_type = "int";
  issue.is_constant_expr = false;
  issue.is_in_loop = true;

  EXPECT_EQ(issue.type, OverflowType::SIGNED_ADDITION);
  EXPECT_EQ(issue.severity, OverflowSeverity::CRITICAL);
  EXPECT_EQ(issue.file, "test.cpp");
  EXPECT_EQ(issue.line, 42u);
  EXPECT_EQ(issue.column, 10u);
  EXPECT_EQ(issue.function, "foo");
  EXPECT_EQ(issue.description, "Potential signed overflow");
  EXPECT_EQ(issue.operator_str, "+");
  EXPECT_EQ(issue.lhs_type, "int");
  EXPECT_EQ(issue.rhs_type, "int");
  EXPECT_FALSE(issue.is_constant_expr);
  EXPECT_TRUE(issue.is_in_loop);
}

// Test OverflowStatistics initialization
TEST_F(OverflowDetectorTest, StatisticsInitialization) {
  OverflowStatistics stats;

  EXPECT_EQ(stats.total_issues, 0u);
  EXPECT_EQ(stats.critical_issues, 0u);
  EXPECT_EQ(stats.warning_issues, 0u);
  EXPECT_EQ(stats.info_issues, 0u);
  EXPECT_EQ(stats.signed_overflows, 0u);
  EXPECT_EQ(stats.unsigned_wraparounds, 0u);
  EXPECT_EQ(stats.mixed_signedness, 0u);
  EXPECT_EQ(stats.narrowing_conversions, 0u);
  EXPECT_EQ(stats.loop_overflows, 0u);
  EXPECT_TRUE(stats.issues_per_file.empty());
  EXPECT_TRUE(stats.issues_per_function.empty());
}

// Test OverflowStatistics with values
TEST_F(OverflowDetectorTest, StatisticsWithValues) {
  OverflowStatistics stats;
  stats.total_issues = 10;
  stats.critical_issues = 5;
  stats.warning_issues = 3;
  stats.info_issues = 2;
  stats.signed_overflows = 4;
  stats.unsigned_wraparounds = 2;
  stats.mixed_signedness = 2;
  stats.narrowing_conversions = 1;
  stats.loop_overflows = 1;
  stats.issues_per_file["test.cpp"] = 5;
  stats.issues_per_file["main.cpp"] = 5;
  stats.issues_per_function["foo"] = 3;
  stats.issues_per_function["bar"] = 7;

  EXPECT_EQ(stats.total_issues, 10u);
  EXPECT_EQ(stats.critical_issues, 5u);
  EXPECT_EQ(stats.warning_issues, 3u);
  EXPECT_EQ(stats.info_issues, 2u);
  EXPECT_EQ(stats.signed_overflows, 4u);
  EXPECT_EQ(stats.unsigned_wraparounds, 2u);
  EXPECT_EQ(stats.mixed_signedness, 2u);
  EXPECT_EQ(stats.narrowing_conversions, 1u);
  EXPECT_EQ(stats.loop_overflows, 1u);
  EXPECT_EQ(stats.issues_per_file.size(), 2u);
  EXPECT_EQ(stats.issues_per_function.size(), 2u);
  EXPECT_EQ(stats.issues_per_file.at("test.cpp"), 5u);
  EXPECT_EQ(stats.issues_per_function.at("foo"), 3u);
}

// Test overflow_type_to_string function
TEST_F(OverflowDetectorTest, OverflowTypeToString) {
  EXPECT_STREQ(overflow_type_to_string(OverflowType::SIGNED_ADDITION), "signed_addition_overflow");
  EXPECT_STREQ(overflow_type_to_string(OverflowType::SIGNED_SUBTRACTION), "signed_subtraction_overflow");
  EXPECT_STREQ(overflow_type_to_string(OverflowType::SIGNED_MULTIPLICATION), "signed_multiplication_overflow");
  EXPECT_STREQ(overflow_type_to_string(OverflowType::SIGNED_DIVISION), "signed_division_overflow");
  EXPECT_STREQ(overflow_type_to_string(OverflowType::SIGNED_NEGATION), "signed_negation_overflow");
  EXPECT_STREQ(overflow_type_to_string(OverflowType::SIGNED_LEFT_SHIFT), "signed_left_shift_overflow");
  EXPECT_STREQ(overflow_type_to_string(OverflowType::UNSIGNED_WRAPAROUND), "unsigned_wraparound");
  EXPECT_STREQ(overflow_type_to_string(OverflowType::MIXED_SIGNEDNESS), "mixed_signedness");
  EXPECT_STREQ(overflow_type_to_string(OverflowType::NARROWING_CONVERSION), "narrowing_conversion");
  EXPECT_STREQ(overflow_type_to_string(OverflowType::LOOP_COUNTER_OVERFLOW), "loop_counter_overflow");
}

// Test severity_to_string function
TEST_F(OverflowDetectorTest, SeverityToString) {
  EXPECT_STREQ(severity_to_string(OverflowSeverity::CRITICAL), "critical");
  EXPECT_STREQ(severity_to_string(OverflowSeverity::WARNING), "warning");
  EXPECT_STREQ(severity_to_string(OverflowSeverity::INFO), "info");
}

// Test that different overflow types map to appropriate severities conceptually
TEST_F(OverflowDetectorTest, OverflowTypeSeverityRelationship) {
  // Signed overflows should typically be CRITICAL (undefined behavior)
  OverflowIssue signed_issue;
  signed_issue.type = OverflowType::SIGNED_ADDITION;
  signed_issue.severity = OverflowSeverity::CRITICAL;
  EXPECT_EQ(signed_issue.severity, OverflowSeverity::CRITICAL);

  // Unsigned wraparound might be WARNING (defined but often unintended)
  OverflowIssue unsigned_issue;
  unsigned_issue.type = OverflowType::UNSIGNED_WRAPAROUND;
  unsigned_issue.severity = OverflowSeverity::WARNING;
  EXPECT_EQ(unsigned_issue.severity, OverflowSeverity::WARNING);

  // Mixed signedness might be INFO (potential issue)
  OverflowIssue mixed_issue;
  mixed_issue.type = OverflowType::MIXED_SIGNEDNESS;
  mixed_issue.severity = OverflowSeverity::INFO;
  EXPECT_EQ(mixed_issue.severity, OverflowSeverity::INFO);
}

// Test issue collection in vector
TEST_F(OverflowDetectorTest, IssueCollection) {
  std::vector<OverflowIssue> issues;

  OverflowIssue issue1;
  issue1.type = OverflowType::SIGNED_ADDITION;
  issue1.severity = OverflowSeverity::CRITICAL;
  issue1.line = 10;

  OverflowIssue issue2;
  issue2.type = OverflowType::NARROWING_CONVERSION;
  issue2.severity = OverflowSeverity::WARNING;
  issue2.line = 20;

  issues.push_back(issue1);
  issues.push_back(issue2);

  EXPECT_EQ(issues.size(), 2u);
  EXPECT_EQ(issues[0].type, OverflowType::SIGNED_ADDITION);
  EXPECT_EQ(issues[1].type, OverflowType::NARROWING_CONVERSION);
  EXPECT_EQ(issues[0].line, 10u);
  EXPECT_EQ(issues[1].line, 20u);
}

// Test filtering issues by severity
TEST_F(OverflowDetectorTest, FilterBySeverity) {
  std::vector<OverflowIssue> all_issues;

  OverflowIssue critical1;
  critical1.severity = OverflowSeverity::CRITICAL;
  critical1.description = "Critical issue 1";
  all_issues.push_back(critical1);

  OverflowIssue warning1;
  warning1.severity = OverflowSeverity::WARNING;
  warning1.description = "Warning issue 1";
  all_issues.push_back(warning1);

  OverflowIssue critical2;
  critical2.severity = OverflowSeverity::CRITICAL;
  critical2.description = "Critical issue 2";
  all_issues.push_back(critical2);

  OverflowIssue info1;
  info1.severity = OverflowSeverity::INFO;
  info1.description = "Info issue 1";
  all_issues.push_back(info1);

  // Filter critical issues
  std::vector<OverflowIssue> critical_issues;
  for (const auto& issue : all_issues) {
    if (issue.severity == OverflowSeverity::CRITICAL) {
      critical_issues.push_back(issue);
    }
  }

  EXPECT_EQ(critical_issues.size(), 2u);
  EXPECT_EQ(critical_issues[0].description, "Critical issue 1");
  EXPECT_EQ(critical_issues[1].description, "Critical issue 2");
}
