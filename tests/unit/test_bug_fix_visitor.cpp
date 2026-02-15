#include "optiweave/analysis/bug_fix_issue.hpp"
#include <gtest/gtest.h>

using namespace optiweave::analysis;

class BugFixIssueTest : public ::testing::Test {
protected:
  void SetUp() override {}
};

// --- BugFixKind enum ---

TEST_F(BugFixIssueTest, BugFixKindEnumValues) {
  BugFixKind k1 = BugFixKind::UnsignedWraparound;
  BugFixKind k2 = BugFixKind::SignedNegationOverflow;
  BugFixKind k3 = BugFixKind::SignedLeftShift;
  BugFixKind k4 = BugFixKind::UninitializedVariable;
  BugFixKind k5 = BugFixKind::UnusedVariable;
  BugFixKind k6 = BugFixKind::FPEqualityComparison;

  EXPECT_NE(k1, k2);
  EXPECT_NE(k3, k4);
  EXPECT_NE(k5, k6);
  EXPECT_EQ(k1, BugFixKind::UnsignedWraparound);
}

// --- BugFixIssue struct ---

TEST_F(BugFixIssueTest, DefaultConstruction) {
  BugFixIssue issue{};
  EXPECT_TRUE(issue.file.empty());
  EXPECT_EQ(issue.line, 0u);
  EXPECT_EQ(issue.column, 0u);
  EXPECT_TRUE(issue.description.empty());
  EXPECT_TRUE(issue.var_name.empty());
  EXPECT_TRUE(issue.var_type.empty());
  EXPECT_TRUE(issue.expression_text.empty());
}

TEST_F(BugFixIssueTest, PopulateFields) {
  BugFixIssue issue;
  issue.kind = BugFixKind::UnsignedWraparound;
  issue.file = "test.c";
  issue.line = 42;
  issue.column = 10;
  issue.description = "unsigned subtraction may wraparound";
  issue.expression_text = "a - b";
  issue.lhs_type = "unsigned int";
  issue.rhs_type = "unsigned int";

  EXPECT_EQ(issue.kind, BugFixKind::UnsignedWraparound);
  EXPECT_EQ(issue.file, "test.c");
  EXPECT_EQ(issue.line, 42u);
  EXPECT_EQ(issue.expression_text, "a - b");
}

// --- convert_overflow_issues ---

TEST_F(BugFixIssueTest, ConvertOverflowUnsignedWraparound) {
  std::vector<OverflowIssue> overflow_issues;

  OverflowIssue oi;
  oi.type = OverflowType::UNSIGNED_WRAPAROUND;
  oi.severity = OverflowSeverity::WARNING;
  oi.file = "foo.c";
  oi.line = 10;
  oi.column = 5;
  oi.operator_str = "-";
  oi.code_snippet = "a - b";
  oi.lhs_type = "unsigned int";
  oi.rhs_type = "unsigned int";
  overflow_issues.push_back(oi);

  auto result = convert_overflow_issues(overflow_issues);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].kind, BugFixKind::UnsignedWraparound);
  EXPECT_EQ(result[0].file, "foo.c");
  EXPECT_EQ(result[0].line, 10u);
}

TEST_F(BugFixIssueTest, ConvertOverflowIgnoresNonSubtraction) {
  std::vector<OverflowIssue> overflow_issues;

  OverflowIssue oi;
  oi.type = OverflowType::UNSIGNED_WRAPAROUND;
  oi.operator_str = "+"; // addition wraparound — not auto-fixable
  overflow_issues.push_back(oi);

  auto result = convert_overflow_issues(overflow_issues);
  EXPECT_TRUE(result.empty());
}

TEST_F(BugFixIssueTest, ConvertOverflowSignedNegation) {
  std::vector<OverflowIssue> overflow_issues;

  OverflowIssue oi;
  oi.type = OverflowType::SIGNED_NEGATION;
  oi.file = "bar.c";
  oi.line = 20;
  overflow_issues.push_back(oi);

  auto result = convert_overflow_issues(overflow_issues);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].kind, BugFixKind::SignedNegationOverflow);
}

TEST_F(BugFixIssueTest, ConvertOverflowSignedLeftShift) {
  std::vector<OverflowIssue> overflow_issues;

  OverflowIssue oi;
  oi.type = OverflowType::SIGNED_LEFT_SHIFT;
  oi.file = "baz.c";
  oi.line = 30;
  overflow_issues.push_back(oi);

  auto result = convert_overflow_issues(overflow_issues);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].kind, BugFixKind::SignedLeftShift);
}

TEST_F(BugFixIssueTest, ConvertOverflowIgnoresUnsupported) {
  std::vector<OverflowIssue> overflow_issues;

  OverflowIssue oi;
  oi.type = OverflowType::SIGNED_ADDITION;
  overflow_issues.push_back(oi);

  oi.type = OverflowType::MIXED_SIGNEDNESS;
  overflow_issues.push_back(oi);

  oi.type = OverflowType::NARROWING_CONVERSION;
  overflow_issues.push_back(oi);

  auto result = convert_overflow_issues(overflow_issues);
  EXPECT_TRUE(result.empty());
}

// --- convert_fp_issues ---

TEST_F(BugFixIssueTest, ConvertFPEqualityComparison) {
  std::vector<FPPrecisionIssue> fp_issues;

  FPPrecisionIssue fi;
  fi.type = FPPrecisionType::FP_EQUALITY_COMPARISON;
  fi.file = "fp.c";
  fi.line = 50;
  fi.column = 8;
  fi.description = "float equality";
  fi.expression_text = "a == b";
  fi.left_operand_type = "double";
  fi.right_operand_type = "double";
  fp_issues.push_back(fi);

  auto result = convert_fp_issues(fp_issues);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].kind, BugFixKind::FPEqualityComparison);
  EXPECT_EQ(result[0].file, "fp.c");
}

TEST_F(BugFixIssueTest, ConvertFPIgnoresNonEquality) {
  std::vector<FPPrecisionIssue> fp_issues;

  FPPrecisionIssue fi;
  fi.type = FPPrecisionType::CATASTROPHIC_CANCELLATION;
  fp_issues.push_back(fi);

  fi.type = FPPrecisionType::DOUBLE_TO_FLOAT_CONVERSION;
  fp_issues.push_back(fi);

  auto result = convert_fp_issues(fp_issues);
  EXPECT_TRUE(result.empty());
}

// --- Multiple issue types mixed ---

TEST_F(BugFixIssueTest, ConvertMixedOverflowIssues) {
  std::vector<OverflowIssue> overflow_issues;

  OverflowIssue oi1;
  oi1.type = OverflowType::UNSIGNED_WRAPAROUND;
  oi1.operator_str = "-";
  oi1.file = "a.c";
  oi1.line = 1;
  overflow_issues.push_back(oi1);

  OverflowIssue oi2;
  oi2.type = OverflowType::SIGNED_NEGATION;
  oi2.file = "a.c";
  oi2.line = 2;
  overflow_issues.push_back(oi2);

  OverflowIssue oi3;
  oi3.type = OverflowType::SIGNED_LEFT_SHIFT;
  oi3.file = "a.c";
  oi3.line = 3;
  overflow_issues.push_back(oi3);

  OverflowIssue oi4; // This one should be filtered
  oi4.type = OverflowType::SIGNED_MULTIPLICATION;
  oi4.file = "a.c";
  oi4.line = 4;
  overflow_issues.push_back(oi4);

  auto result = convert_overflow_issues(overflow_issues);
  EXPECT_EQ(result.size(), 3u);
}
