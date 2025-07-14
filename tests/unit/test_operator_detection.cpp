#include "optiweave/analysis/operator_detector.hpp"
#include <gtest/gtest.h>

using namespace optiweave::analysis;

class OperatorDetectionTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Test setup
  }
};

TEST_F(OperatorDetectionTest, StatisticsInitialization) {
  OperatorStatistics stats;
  
  EXPECT_EQ(stats.total_array_subscripts, 0u);
  EXPECT_EQ(stats.total_arithmetic_ops, 0u);
  EXPECT_EQ(stats.total_assignment_ops, 0u);
  EXPECT_EQ(stats.total_comparison_ops, 0u);
  EXPECT_EQ(stats.overloaded_operators, 0u);
  EXPECT_EQ(stats.template_dependent_ops, 0u);
  EXPECT_EQ(stats.system_header_ops, 0u);
}

TEST_F(OperatorDetectionTest, StatisticsReset) {
  OperatorStatistics stats;
  stats.total_array_subscripts = 5;
  stats.total_arithmetic_ops = 3;
  stats.overloaded_operators = 2;
  
  stats.reset();
  
  EXPECT_EQ(stats.total_array_subscripts, 0u);
  EXPECT_EQ(stats.total_arithmetic_ops, 0u);
  EXPECT_EQ(stats.overloaded_operators, 0u);
}

TEST_F(OperatorDetectionTest, OperatorUsageStructure) {
  OperatorUsage usage;
  
  // Test default values
  EXPECT_FALSE(usage.is_overloaded);
  EXPECT_FALSE(usage.is_template_dependent);
  EXPECT_FALSE(usage.in_system_header);
  EXPECT_TRUE(usage.operator_name.empty());
  EXPECT_TRUE(usage.lhs_type.empty());
  EXPECT_TRUE(usage.rhs_type.empty());
}

TEST_F(OperatorDetectionTest, AnalysisResultStructure) {
  OperatorAnalysisResult result;
  
  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.error_message.empty());
  EXPECT_TRUE(result.all_usages.empty());
  EXPECT_TRUE(result.transformation_candidates.empty());
  EXPECT_TRUE(result.recommendations.empty());
}