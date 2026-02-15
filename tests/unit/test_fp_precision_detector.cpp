#include "optiweave/analysis/fp_precision_detector.hpp"
#include <gtest/gtest.h>

using namespace optiweave::analysis;

class FPPrecisionDetectorTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Test setup
  }
};

// Test FPPrecisionType enum values
TEST_F(FPPrecisionDetectorTest, FPPrecisionTypeEnumValues) {
  FPPrecisionType fp_equality = FPPrecisionType::FP_EQUALITY_COMPARISON;
  FPPrecisionType catastrophic = FPPrecisionType::CATASTROPHIC_CANCELLATION;
  FPPrecisionType double_to_float = FPPrecisionType::DOUBLE_TO_FLOAT_CONVERSION;
  FPPrecisionType long_double = FPPrecisionType::LONG_DOUBLE_CONVERSION;
  FPPrecisionType accumulation = FPPrecisionType::ACCUMULATION_WITHOUT_COMPENSATION;
  FPPrecisionType division = FPPrecisionType::DIVISION_BY_SMALL_VALUE;
  FPPrecisionType multiplication = FPPrecisionType::FP_MULTIPLICATION_OVERFLOW;
  FPPrecisionType mixed_precision = FPPrecisionType::MIXED_PRECISION_ARITHMETIC;
  FPPrecisionType exact_zero = FPPrecisionType::EXACT_ZERO_COMPARISON;
  FPPrecisionType magnitude = FPPrecisionType::MAGNITUDE_DISPARITY;

  // Verify assignment and comparison
  EXPECT_EQ(fp_equality, FPPrecisionType::FP_EQUALITY_COMPARISON);
  EXPECT_EQ(catastrophic, FPPrecisionType::CATASTROPHIC_CANCELLATION);
  EXPECT_NE(fp_equality, catastrophic);
}

// Test FPPrecisionSeverity enum values
TEST_F(FPPrecisionDetectorTest, FPPrecisionSeverityEnumValues) {
  FPPrecisionSeverity critical = FPPrecisionSeverity::CRITICAL;
  FPPrecisionSeverity warning = FPPrecisionSeverity::WARNING;
  FPPrecisionSeverity info = FPPrecisionSeverity::INFO;

  EXPECT_EQ(critical, FPPrecisionSeverity::CRITICAL);
  EXPECT_EQ(warning, FPPrecisionSeverity::WARNING);
  EXPECT_EQ(info, FPPrecisionSeverity::INFO);
  EXPECT_NE(critical, warning);
}

// Test FPPrecisionIssue structure initialization
TEST_F(FPPrecisionDetectorTest, FPPrecisionIssueStructure) {
  FPPrecisionIssue issue{};  // Use aggregate initialization to zero-initialize

  // Test default values (empty strings, zero integers)
  EXPECT_TRUE(issue.file.empty());
  EXPECT_EQ(issue.line, 0u);
  EXPECT_EQ(issue.column, 0u);
  EXPECT_TRUE(issue.function_name.empty());
  EXPECT_TRUE(issue.description.empty());
  EXPECT_TRUE(issue.suggestion.empty());
  EXPECT_TRUE(issue.left_operand_type.empty());
  EXPECT_TRUE(issue.right_operand_type.empty());
  EXPECT_TRUE(issue.expression_text.empty());
}

// Test FPPrecisionIssue with values
TEST_F(FPPrecisionDetectorTest, FPPrecisionIssueWithValues) {
  FPPrecisionIssue issue;
  issue.type = FPPrecisionType::FP_EQUALITY_COMPARISON;
  issue.severity = FPPrecisionSeverity::CRITICAL;
  issue.file = "math.cpp";
  issue.line = 100;
  issue.column = 15;
  issue.function_name = "calculate";
  issue.description = "Floating-point equality comparison";
  issue.suggestion = "Use epsilon comparison instead";
  issue.left_operand_type = "double";
  issue.right_operand_type = "double";
  issue.expression_text = "a == b";

  EXPECT_EQ(issue.type, FPPrecisionType::FP_EQUALITY_COMPARISON);
  EXPECT_EQ(issue.severity, FPPrecisionSeverity::CRITICAL);
  EXPECT_EQ(issue.file, "math.cpp");
  EXPECT_EQ(issue.line, 100u);
  EXPECT_EQ(issue.column, 15u);
  EXPECT_EQ(issue.function_name, "calculate");
  EXPECT_EQ(issue.description, "Floating-point equality comparison");
  EXPECT_EQ(issue.suggestion, "Use epsilon comparison instead");
  EXPECT_EQ(issue.left_operand_type, "double");
  EXPECT_EQ(issue.right_operand_type, "double");
  EXPECT_EQ(issue.expression_text, "a == b");
}

// Test issue collection
TEST_F(FPPrecisionDetectorTest, IssueCollection) {
  std::vector<FPPrecisionIssue> issues;

  FPPrecisionIssue issue1;
  issue1.type = FPPrecisionType::FP_EQUALITY_COMPARISON;
  issue1.severity = FPPrecisionSeverity::CRITICAL;
  issue1.line = 10;

  FPPrecisionIssue issue2;
  issue2.type = FPPrecisionType::CATASTROPHIC_CANCELLATION;
  issue2.severity = FPPrecisionSeverity::WARNING;
  issue2.line = 20;

  FPPrecisionIssue issue3;
  issue3.type = FPPrecisionType::MIXED_PRECISION_ARITHMETIC;
  issue3.severity = FPPrecisionSeverity::INFO;
  issue3.line = 30;

  issues.push_back(issue1);
  issues.push_back(issue2);
  issues.push_back(issue3);

  EXPECT_EQ(issues.size(), 3u);
  EXPECT_EQ(issues[0].type, FPPrecisionType::FP_EQUALITY_COMPARISON);
  EXPECT_EQ(issues[1].type, FPPrecisionType::CATASTROPHIC_CANCELLATION);
  EXPECT_EQ(issues[2].type, FPPrecisionType::MIXED_PRECISION_ARITHMETIC);
}

// Test filtering issues by severity
TEST_F(FPPrecisionDetectorTest, FilterBySeverity) {
  std::vector<FPPrecisionIssue> all_issues;

  FPPrecisionIssue critical1;
  critical1.severity = FPPrecisionSeverity::CRITICAL;
  critical1.description = "Critical FP issue";
  all_issues.push_back(critical1);

  FPPrecisionIssue warning1;
  warning1.severity = FPPrecisionSeverity::WARNING;
  warning1.description = "Warning FP issue";
  all_issues.push_back(warning1);

  FPPrecisionIssue info1;
  info1.severity = FPPrecisionSeverity::INFO;
  info1.description = "Info FP issue";
  all_issues.push_back(info1);

  FPPrecisionIssue critical2;
  critical2.severity = FPPrecisionSeverity::CRITICAL;
  critical2.description = "Another critical FP issue";
  all_issues.push_back(critical2);

  // Count by severity
  size_t critical_count = 0;
  size_t warning_count = 0;
  size_t info_count = 0;

  for (const auto& issue : all_issues) {
    switch (issue.severity) {
      case FPPrecisionSeverity::CRITICAL:
        critical_count++;
        break;
      case FPPrecisionSeverity::WARNING:
        warning_count++;
        break;
      case FPPrecisionSeverity::INFO:
        info_count++;
        break;
    }
  }

  EXPECT_EQ(critical_count, 2u);
  EXPECT_EQ(warning_count, 1u);
  EXPECT_EQ(info_count, 1u);
}

// Test filtering by type
TEST_F(FPPrecisionDetectorTest, FilterByType) {
  std::vector<FPPrecisionIssue> all_issues;

  FPPrecisionIssue eq1;
  eq1.type = FPPrecisionType::FP_EQUALITY_COMPARISON;
  all_issues.push_back(eq1);

  FPPrecisionIssue cat1;
  cat1.type = FPPrecisionType::CATASTROPHIC_CANCELLATION;
  all_issues.push_back(cat1);

  FPPrecisionIssue eq2;
  eq2.type = FPPrecisionType::FP_EQUALITY_COMPARISON;
  all_issues.push_back(eq2);

  // Filter equality comparisons
  std::vector<FPPrecisionIssue> equality_issues;
  for (const auto& issue : all_issues) {
    if (issue.type == FPPrecisionType::FP_EQUALITY_COMPARISON) {
      equality_issues.push_back(issue);
    }
  }

  EXPECT_EQ(equality_issues.size(), 2u);
}

// Test type-severity relationship
TEST_F(FPPrecisionDetectorTest, TypeSeverityRelationship) {
  // FP equality comparison should typically be CRITICAL
  FPPrecisionIssue eq_issue;
  eq_issue.type = FPPrecisionType::FP_EQUALITY_COMPARISON;
  eq_issue.severity = FPPrecisionSeverity::CRITICAL;
  EXPECT_EQ(eq_issue.severity, FPPrecisionSeverity::CRITICAL);

  // Double-to-float conversion might be WARNING
  FPPrecisionIssue conv_issue;
  conv_issue.type = FPPrecisionType::DOUBLE_TO_FLOAT_CONVERSION;
  conv_issue.severity = FPPrecisionSeverity::WARNING;
  EXPECT_EQ(conv_issue.severity, FPPrecisionSeverity::WARNING);

  // Mixed precision might be INFO
  FPPrecisionIssue mixed_issue;
  mixed_issue.type = FPPrecisionType::MIXED_PRECISION_ARITHMETIC;
  mixed_issue.severity = FPPrecisionSeverity::INFO;
  EXPECT_EQ(mixed_issue.severity, FPPrecisionSeverity::INFO);
}

// Test issue context information
TEST_F(FPPrecisionDetectorTest, IssueContext) {
  FPPrecisionIssue issue;
  issue.file = "physics_sim.cpp";
  issue.line = 250;
  issue.column = 8;
  issue.function_name = "simulate_particle";
  issue.expression_text = "velocity == 0.0";
  issue.left_operand_type = "double";
  issue.right_operand_type = "double";

  // Verify all context is captured
  EXPECT_EQ(issue.file, "physics_sim.cpp");
  EXPECT_EQ(issue.line, 250u);
  EXPECT_EQ(issue.column, 8u);
  EXPECT_EQ(issue.function_name, "simulate_particle");
  EXPECT_EQ(issue.expression_text, "velocity == 0.0");
  EXPECT_FALSE(issue.left_operand_type.empty());
  EXPECT_FALSE(issue.right_operand_type.empty());
}
