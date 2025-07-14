#include "optiweave/analysis/template_analyzer.hpp"
#include <gtest/gtest.h>

using namespace optiweave::analysis;

class TemplateHandlingTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Test setup
  }
};

TEST_F(TemplateHandlingTest, TemplateStatisticsInitialization) {
  TemplateStatistics stats;
  
  EXPECT_EQ(stats.total_template_functions, 0u);
  EXPECT_EQ(stats.total_template_classes, 0u);
  EXPECT_EQ(stats.total_template_instantiations, 0u);
  EXPECT_EQ(stats.dependent_operator_usages, 0u);
  EXPECT_EQ(stats.sfinae_candidates, 0u);
  EXPECT_TRUE(stats.template_name_counts.empty());
  EXPECT_TRUE(stats.argument_type_counts.empty());
}

TEST_F(TemplateHandlingTest, TemplateStatisticsReset) {
  TemplateStatistics stats;
  stats.total_template_functions = 5;
  stats.total_template_classes = 3;
  stats.dependent_operator_usages = 7;
  
  stats.reset();
  
  EXPECT_EQ(stats.total_template_functions, 0u);
  EXPECT_EQ(stats.total_template_classes, 0u);
  EXPECT_EQ(stats.dependent_operator_usages, 0u);
}

TEST_F(TemplateHandlingTest, TemplateUsageStructure) {
  TemplateUsage usage;
  
  // Test default values
  EXPECT_FALSE(usage.is_dependent);
  EXPECT_FALSE(usage.is_instantiation);
  EXPECT_FALSE(usage.has_operator_usage);
  EXPECT_TRUE(usage.template_name.empty());
  EXPECT_TRUE(usage.template_arguments.empty());
  EXPECT_TRUE(usage.context_info.empty());
}

TEST_F(TemplateHandlingTest, TransformationStrategy) {
  // Test transformation strategy enumeration
  auto strategy = TemplateTransformationStrategy::CompileTime;
  EXPECT_EQ(strategy, TemplateTransformationStrategy::CompileTime);
  
  strategy = TemplateTransformationStrategy::RuntimeCheck;
  EXPECT_EQ(strategy, TemplateTransformationStrategy::RuntimeCheck);
  
  strategy = TemplateTransformationStrategy::SfinaeDetection;
  EXPECT_EQ(strategy, TemplateTransformationStrategy::SfinaeDetection);
  
  strategy = TemplateTransformationStrategy::Hybrid;
  EXPECT_EQ(strategy, TemplateTransformationStrategy::Hybrid);
}

TEST_F(TemplateHandlingTest, TemplateTransformationRecommendation) {
  TemplateTransformationRecommendation rec;
  
  // Test default values
  EXPECT_EQ(rec.confidence_score, 0.0);
  EXPECT_TRUE(rec.template_name.empty());
  EXPECT_TRUE(rec.required_traits.empty());
  EXPECT_TRUE(rec.generated_code.empty());
  EXPECT_TRUE(rec.rationale.empty());
}

TEST_F(TemplateHandlingTest, TemplateAnalysisResult) {
  TemplateAnalysisResult result;
  
  EXPECT_FALSE(result.success);
  EXPECT_TRUE(result.error_message.empty());
  EXPECT_TRUE(result.all_usages.empty());
  EXPECT_TRUE(result.sfinae_candidates.empty());
  EXPECT_TRUE(result.dependent_operators.empty());
  EXPECT_TRUE(result.recommendations.empty());
}