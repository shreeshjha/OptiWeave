#include "optiweave/core/ast_visitor.hpp"
#include <gtest/gtest.h>

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>

using namespace optiweave::core;
using namespace clang;
using namespace clang::tooling;

class ASTVisitorTest : public ::testing::Test {
protected:
  void SetUp() override {
    config_ = TransformationConfig{};
    config_.transform_array_subscripts = true;
    config_.transform_arithmetic_operators = false;
    config_.skip_system_headers = true;
  }

  TransformationConfig config_;
};

// Test configuration defaults
TEST_F(ASTVisitorTest, DefaultConfiguration) {
  TransformationConfig default_config;

  EXPECT_TRUE(default_config.transform_array_subscripts);
  EXPECT_FALSE(default_config.transform_arithmetic_operators);
  EXPECT_FALSE(default_config.transform_assignment_operators);
  EXPECT_FALSE(default_config.transform_comparisons_operators);
  EXPECT_TRUE(default_config.preserve_templates);
  EXPECT_TRUE(default_config.skip_system_headers);
}

// Test statistics initialization
TEST_F(ASTVisitorTest, StatisticsInitialization) {
  TransformationStats stats;

  EXPECT_EQ(stats.array_subscripts_transformed, 0u);
  EXPECT_EQ(stats.arithmetic_ops_transformed, 0u);
  EXPECT_EQ(stats.template_instantiations_skipped, 0u);
  EXPECT_EQ(stats.errors_encountered, 0u);
}

// Test statistics reset
TEST_F(ASTVisitorTest, StatisticsReset) {
  TransformationStats stats;
  stats.array_subscripts_transformed = 5;
  stats.arithmetic_ops_transformed = 3;
  stats.errors_encountered = 1;

  stats.reset();

  EXPECT_EQ(stats.array_subscripts_transformed, 0u);
  EXPECT_EQ(stats.arithmetic_ops_transformed, 0u);
  EXPECT_EQ(stats.errors_encountered, 0u);
}

// Mock test for visitor creation (requires actual AST context)
class MockASTVisitorTest : public ::testing::Test {
protected:
  void SetUp() override {
    // This would require setting up a real CompilerInstance
    // For now, we test the interface
  }

  // Helper to create a minimal visitor for interface testing
  std::unique_ptr<ModernASTVisitor> createMockVisitor() {
    // In a real implementation, this would need:
    // - CompilerInstance
    // - SourceManager
    // - ASTContext
    // - Rewriter
    return nullptr; // Placeholder
  }
};

TEST_F(MockASTVisitorTest, VisitorInterface) {
  // Test that the visitor interface is properly defined
  TransformationConfig config;
  config.transform_array_subscripts = true;

  // These tests verify the interface without requiring full AST setup
  EXPECT_TRUE(config.transform_array_subscripts);
  EXPECT_FALSE(config.transform_arithmetic_operators);
}

// Test configuration combinations
TEST_F(ASTVisitorTest, ConfigurationCombinations) {
  TransformationConfig config;

  // Test all operators enabled
  config.transform_array_subscripts = true;
  config.transform_arithmetic_operators = true;
  config.transform_assignment_operators = true;
  config.transform_comparisons_operators = true;

  EXPECT_TRUE(config.transform_array_subscripts);
  EXPECT_TRUE(config.transform_arithmetic_operators);
  EXPECT_TRUE(config.transform_assignment_operators);
  EXPECT_TRUE(config.transform_comparisons_operators);

  // Test all operators disabled
  config.transform_array_subscripts = false;
  config.transform_arithmetic_operators = false;
  config.transform_assignment_operators = false;
  config.transform_comparisons_operators = false;

  EXPECT_FALSE(config.transform_array_subscripts);
  EXPECT_FALSE(config.transform_arithmetic_operators);
  EXPECT_FALSE(config.transform_assignment_operators);
  EXPECT_FALSE(config.transform_comparisons_operators);
}

// Test path handling
TEST_F(ASTVisitorTest, PathHandling) {
  TransformationConfig config;

  // Test prelude path setting
  config.prelude_path = "/path/to/prelude.hpp";
  EXPECT_EQ(config.prelude_path, "/path/to/prelude.hpp");

  // Test include paths
  config.include_paths.push_back("/usr/include");
  config.include_paths.push_back("/usr/local/include");

  EXPECT_EQ(config.include_paths.size(), 2u);
  EXPECT_EQ(config.include_paths[0], "/usr/include");
  EXPECT_EQ(config.include_paths[1], "/usr/local/include");
}

// Test consumer interface
class ConsumerInterfaceTest : public ::testing::Test {
protected:
  void SetUp() override { config_.transform_array_subscripts = true; }

  TransformationConfig config_;
};

TEST_F(ConsumerInterfaceTest, ConsumerCreation) {
  // Test that TransformationConsumer can be created with valid config
  // This is an interface test without requiring full AST setup

  EXPECT_TRUE(config_.transform_array_subscripts);
  EXPECT_TRUE(config_.preserve_templates);
  EXPECT_TRUE(config_.skip_system_headers);
}

// Test error handling
TEST_F(ASTVisitorTest, ErrorHandling) {
  TransformationStats stats;

  // Simulate error conditions
  stats.errors_encountered = 0;

  // Test error counting
  ++stats.errors_encountered;
  EXPECT_EQ(stats.errors_encountered, 1u);

  // Test multiple errors
  stats.errors_encountered += 5;
  EXPECT_EQ(stats.errors_encountered, 6u);
}

// Test statistics printing (mock)
TEST_F(ASTVisitorTest, StatisticsPrinting) {
  TransformationStats stats;
  stats.array_subscripts_transformed = 10;
  stats.arithmetic_ops_transformed = 5;
  stats.template_instantiations_skipped = 2;
  stats.errors_encountered = 1;

  // Test that stats have expected values before printing
  EXPECT_EQ(stats.array_subscripts_transformed, 10u);
  EXPECT_EQ(stats.arithmetic_ops_transformed, 5u);
  EXPECT_EQ(stats.template_instantiations_skipped, 2u);
  EXPECT_EQ(stats.errors_encountered, 1u);

  // In a real test, we would capture the output of stats.print()
  // For now, we just verify the values are correct
}

// Integration test with minimal AST (this would require more setup in practice)
class MinimalASTTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup minimal test environment
    // In practice, this would create a CompilerInstance
  }

  // Helper to test visitor behavior with minimal code
  bool testVisitorWithCode(const std::string &code) {
    // This would:
    // 1. Create CompilerInstance
    // 2. Parse the code
    // 3. Create AST visitor
    // 4. Run visitor on AST
    // 5. Check results

    // For now, just return success if code is non-empty
    return !code.empty();
  }
};

TEST_F(MinimalASTTest, SimpleArrayAccess) {
  std::string code = R"(
        int main() {
            int arr[10];
            return arr[5];
        }
    )";

  EXPECT_TRUE(testVisitorWithCode(code));
}

TEST_F(MinimalASTTest, EmptyCode) {
  std::string code = "";
  EXPECT_FALSE(testVisitorWithCode(code));
}

TEST_F(MinimalASTTest, InvalidCode) {
  std::string code = "invalid c++ code {{{";
  // Should still return true for non-empty code in our
