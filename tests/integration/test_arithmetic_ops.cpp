/**
 * @file test_arithmetic_ops.cpp
 * @brief Integration tests for arithmetic, assignment, and comparison operator transformation
 * 
 * TDD test file for OptiWeave operator instrumentation.
 * Tests cover arithmetic (+, -, *, /, %), assignment (=, +=, etc.), and comparison (<, >, etc.) operators.
 */

#include "optiweave/core/ast_visitor.hpp"
#include "optiweave/core/rewriter.hpp"
#include <gtest/gtest.h>

#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>

#include <memory>
#include <string>

using namespace optiweave::core;
using namespace clang;
using namespace clang::tooling;

class ArithmeticOpsTransformationTest : public ::testing::Test {
public:
  /**
   * @brief Custom frontend action for testing
   */
  class TransformationAction : public ASTFrontendAction {
  public:
    explicit TransformationAction(const TransformationConfig &config,
                                  std::string &output_code,
                                  TransformationStats &output_stats)
        : config_(config), output_code_(output_code), output_stats_(output_stats) {}

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                   StringRef file) override {
      rewriter_.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
      auto consumer = std::make_unique<TransformationConsumer>(
          rewriter_, CI.getASTContext(), config_);
      consumer_ = consumer.get();  // Store raw pointer for stats access
      return consumer;  // Transfer ownership to caller
    }

    void EndSourceFileAction() override {
      // Capture the transformed code
      auto &source_manager = rewriter_.getSourceMgr();
      auto main_file_id = source_manager.getMainFileID();

      if (auto buffer = rewriter_.getRewriteBufferFor(main_file_id)) {
        output_code_ = std::string(buffer->begin(), buffer->end());
      } else {
        // No changes were made, return original
        auto original_buffer = source_manager.getBufferData(main_file_id);
        output_code_ = original_buffer.str();
      }
      
      // Copy stats before action is destroyed
      if (consumer_) {
        output_stats_ = consumer_->getStats();
      }
    }

  private:
    TransformationConfig config_;
    Rewriter rewriter_;
    TransformationConsumer* consumer_ = nullptr;
    std::string &output_code_;
    TransformationStats &output_stats_;
  };

protected:
  void SetUp() override {
    // Common setup for tests
  }

  void TearDown() override {
    // Cleanup
  }

  /**
   * @brief Helper to run transformation on source code
   */
  std::string transformSource(const std::string &source_code,
                              const TransformationConfig &config = {}) {
    // Create a unique filename for this test
    std::string filename =
        "/tmp/test_arith_" + std::to_string(++test_counter_) + ".cpp";

    // Output variables that will be populated by the action
    std::string transformed_code;
    TransformationStats stats;
    
    // Create the action with reference parameters
    auto action = std::make_unique<TransformationAction>(config, transformed_code, stats);
    
    auto result = runToolOnCode(std::move(action), source_code, filename);
    
    // Store stats for later access
    last_stats_ = stats;
    
    if (result) {
      return transformed_code;
    } else {
      return source_code; // Return original if transformation failed
    }
  }

  /**
   * @brief Get stats from last transformation
   */
  const TransformationStats& getLastStats() const {
    return last_stats_;
  }

private:
  static int test_counter_;
  TransformationStats last_stats_;
};

int ArithmeticOpsTransformationTest::test_counter_ = 0;

// ============================================================================
// Arithmetic Operator Tests (+, -, *, /, %)
// ============================================================================

TEST_F(ArithmeticOpsTransformationTest, AdditionOperator) {
  std::string source = R"(
int main() {
    int a = 5, b = 3;
    int c = a + b;
    return c;
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the addition was transformed
  EXPECT_TRUE(result.find("ow_add") != std::string::npos ||
              result.find("__primop_add") != std::string::npos)
      << "Addition operator should be transformed. Result:\n" << result;
  
  // Original operator should be wrapped
  EXPECT_TRUE(result.find("a + b") == std::string::npos ||
              result.find("ow_add(a, b)") != std::string::npos)
      << "Original 'a + b' should be replaced with ow_add. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, SubtractionOperator) {
  std::string source = R"(
int main() {
    int a = 10, b = 4;
    int c = a - b;
    return c;
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the subtraction was transformed
  EXPECT_TRUE(result.find("ow_sub") != std::string::npos ||
              result.find("__primop_sub") != std::string::npos)
      << "Subtraction operator should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, MultiplicationOperator) {
  std::string source = R"(
int main() {
    int a = 6, b = 7;
    int c = a * b;
    return c;
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the multiplication was transformed
  EXPECT_TRUE(result.find("ow_mul") != std::string::npos ||
              result.find("__primop_mul") != std::string::npos)
      << "Multiplication operator should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, DivisionOperator) {
  std::string source = R"(
int main() {
    int a = 20, b = 4;
    int c = a / b;
    return c;
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the division was transformed
  EXPECT_TRUE(result.find("ow_div") != std::string::npos ||
              result.find("__primop_div") != std::string::npos)
      << "Division operator should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, ModuloOperator) {
  std::string source = R"(
int main() {
    int a = 17, b = 5;
    int c = a % b;
    return c;
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the modulo was transformed
  EXPECT_TRUE(result.find("ow_rem") != std::string::npos ||
              result.find("__primop_rem") != std::string::npos)
      << "Modulo operator should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, ChainedArithmeticOperations) {
  std::string source = R"(
int main() {
    int a = 2, b = 3, c = 4;
    int result = a + b * c;
    return result;
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Both add and mul should be transformed
  EXPECT_TRUE((result.find("ow_add") != std::string::npos ||
               result.find("__primop_add") != std::string::npos) &&
              (result.find("ow_mul") != std::string::npos ||
               result.find("__primop_mul") != std::string::npos))
      << "Chained operations should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, FloatingPointArithmetic) {
  std::string source = R"(
int main() {
    double a = 3.14, b = 2.0;
    double c = a * b;
    return static_cast<int>(c);
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that floating-point multiplication was transformed
  EXPECT_TRUE(result.find("ow_mul") != std::string::npos ||
              result.find("__primop_mul") != std::string::npos)
      << "Floating-point multiplication should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, MixedTypeArithmetic) {
  std::string source = R"(
int main() {
    int a = 5;
    double b = 2.5;
    double c = a + b;
    return static_cast<int>(c);
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that mixed-type addition was transformed
  EXPECT_TRUE(result.find("ow_add") != std::string::npos ||
              result.find("__primop_add") != std::string::npos)
      << "Mixed-type addition should be transformed. Result:\n" << result;
}

// ============================================================================
// Assignment Operator Tests (=, +=, -=, *=, /=, %=)
// ============================================================================

TEST_F(ArithmeticOpsTransformationTest, SimpleAssignment) {
  std::string source = R"(
int main() {
    int a;
    a = 42;
    return a;
}
)";

  TransformationConfig config;
  config.transform_assignment_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the assignment was transformed
  EXPECT_TRUE(result.find("ow_assign") != std::string::npos ||
              result.find("__primop_assign") != std::string::npos)
      << "Simple assignment should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, AddAssignment) {
  std::string source = R"(
int main() {
    int a = 10;
    a += 5;
    return a;
}
)";

  TransformationConfig config;
  config.transform_assignment_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the add-assign was transformed
  EXPECT_TRUE(result.find("ow_add_assign") != std::string::npos ||
              result.find("__primop_add_assign") != std::string::npos)
      << "Add-assignment should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, SubAssignment) {
  std::string source = R"(
int main() {
    int a = 10;
    a -= 3;
    return a;
}
)";

  TransformationConfig config;
  config.transform_assignment_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the sub-assign was transformed
  EXPECT_TRUE(result.find("ow_sub_assign") != std::string::npos ||
              result.find("__primop_sub_assign") != std::string::npos)
      << "Sub-assignment should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, MulAssignment) {
  std::string source = R"(
int main() {
    int a = 10;
    a *= 2;
    return a;
}
)";

  TransformationConfig config;
  config.transform_assignment_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the mul-assign was transformed
  EXPECT_TRUE(result.find("ow_mul_assign") != std::string::npos ||
              result.find("__primop_mul_assign") != std::string::npos)
      << "Mul-assignment should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, DivAssignment) {
  std::string source = R"(
int main() {
    int a = 20;
    a /= 4;
    return a;
}
)";

  TransformationConfig config;
  config.transform_assignment_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the div-assign was transformed
  EXPECT_TRUE(result.find("ow_div_assign") != std::string::npos ||
              result.find("__primop_div_assign") != std::string::npos)
      << "Div-assignment should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, ModAssignment) {
  std::string source = R"(
int main() {
    int a = 17;
    a %= 5;
    return a;
}
)";

  TransformationConfig config;
  config.transform_assignment_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the mod-assign was transformed
  EXPECT_TRUE(result.find("ow_rem_assign") != std::string::npos ||
              result.find("ow_mod_assign") != std::string::npos ||
              result.find("__primop_rem_assign") != std::string::npos)
      << "Mod-assignment should be transformed. Result:\n" << result;
}

// ============================================================================
// Comparison Operator Tests (<, >, <=, >=, ==, !=)
// ============================================================================

TEST_F(ArithmeticOpsTransformationTest, EqualityComparison) {
  std::string source = R"(
int main() {
    int a = 5, b = 5;
    if (a == b) return 1;
    return 0;
}
)";

  TransformationConfig config;
  config.transform_comparisons_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the equality comparison was transformed
  EXPECT_TRUE(result.find("ow_eq") != std::string::npos ||
              result.find("__primop_eq") != std::string::npos)
      << "Equality comparison should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, InequalityComparison) {
  std::string source = R"(
int main() {
    int a = 5, b = 3;
    if (a != b) return 1;
    return 0;
}
)";

  TransformationConfig config;
  config.transform_comparisons_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the inequality comparison was transformed
  EXPECT_TRUE(result.find("ow_ne") != std::string::npos ||
              result.find("__primop_ne") != std::string::npos)
      << "Inequality comparison should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, LessThanComparison) {
  std::string source = R"(
int main() {
    int a = 3, b = 5;
    if (a < b) return 1;
    return 0;
}
)";

  TransformationConfig config;
  config.transform_comparisons_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the less-than comparison was transformed
  EXPECT_TRUE(result.find("ow_lt") != std::string::npos ||
              result.find("__primop_lt") != std::string::npos)
      << "Less-than comparison should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, GreaterThanComparison) {
  std::string source = R"(
int main() {
    int a = 7, b = 3;
    if (a > b) return 1;
    return 0;
}
)";

  TransformationConfig config;
  config.transform_comparisons_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the greater-than comparison was transformed
  EXPECT_TRUE(result.find("ow_gt") != std::string::npos ||
              result.find("__primop_gt") != std::string::npos)
      << "Greater-than comparison should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, LessEqualComparison) {
  std::string source = R"(
int main() {
    int a = 5, b = 5;
    if (a <= b) return 1;
    return 0;
}
)";

  TransformationConfig config;
  config.transform_comparisons_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the less-equal comparison was transformed
  EXPECT_TRUE(result.find("ow_le") != std::string::npos ||
              result.find("__primop_le") != std::string::npos)
      << "Less-equal comparison should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, GreaterEqualComparison) {
  std::string source = R"(
int main() {
    int a = 7, b = 7;
    if (a >= b) return 1;
    return 0;
}
)";

  TransformationConfig config;
  config.transform_comparisons_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Check that the greater-equal comparison was transformed
  EXPECT_TRUE(result.find("ow_ge") != std::string::npos ||
              result.find("__primop_ge") != std::string::npos)
      << "Greater-equal comparison should be transformed. Result:\n" << result;
}

// ============================================================================
// Combined Tests (Multiple Operator Types)
// ============================================================================

TEST_F(ArithmeticOpsTransformationTest, ArithmeticWithComparison) {
  std::string source = R"(
int main() {
    int a = 5, b = 3;
    if (a + b > 7) return 1;
    return 0;
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_comparisons_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Both arithmetic and comparison should be transformed
  EXPECT_TRUE((result.find("ow_add") != std::string::npos ||
               result.find("__primop_add") != std::string::npos) &&
              (result.find("ow_gt") != std::string::npos ||
               result.find("__primop_gt") != std::string::npos))
      << "Both arithmetic and comparison should be transformed. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, LoopWithArithmetic) {
  std::string source = R"(
int main() {
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += i;
    }
    return sum;
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_assignment_operators = true;
  config.transform_comparisons_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Multiple operations should be transformed
  // The loop contains: i < 10 (comparison), i++ (could be unary), sum += i (compound assignment)
  EXPECT_TRUE(result.find("ow_lt") != std::string::npos ||
              result.find("ow_add_assign") != std::string::npos ||
              result.find("__primop_") != std::string::npos)
      << "Loop operators should be transformed. Result:\n" << result;
}

// ============================================================================
// Edge Cases and Disabled Transformation Tests
// ============================================================================

TEST_F(ArithmeticOpsTransformationTest, DisabledTransformationNoChange) {
  std::string source = R"(
int main() {
    int a = 5, b = 3;
    int c = a + b;
    return c;
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = false;  // Disabled
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Original code should remain unchanged
  EXPECT_TRUE(result.find("a + b") != std::string::npos)
      << "Disabled transformation should not modify code. Result:\n" << result;
  EXPECT_TRUE(result.find("ow_add") == std::string::npos)
      << "No transformation should be applied when disabled. Result:\n" << result;
}

TEST_F(ArithmeticOpsTransformationTest, OnlyArithmeticEnabled) {
  std::string source = R"(
int main() {
    int a = 5, b = 3;
    int c = a + b;
    if (c > 0) {
        c = c * 2;
    }
    return c;
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_comparisons_operators = false;  // Comparisons disabled
  config.transform_assignment_operators = false;   // Assignments disabled
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Arithmetic should be transformed
  EXPECT_TRUE(result.find("ow_add") != std::string::npos ||
              result.find("ow_mul") != std::string::npos ||
              result.find("__primop_add") != std::string::npos ||
              result.find("__primop_mul") != std::string::npos)
      << "Arithmetic operators should be transformed. Result:\n" << result;
  
  // Comparisons should NOT be transformed
  EXPECT_TRUE(result.find("c > 0") != std::string::npos ||
              result.find("ow_gt") == std::string::npos)
      << "Comparison operators should NOT be transformed when disabled.";
}

// ============================================================================
// Template-Dependent Type Tests
// ============================================================================

TEST_F(ArithmeticOpsTransformationTest, TemplateArithmetic) {
  std::string source = R"(
template<typename T>
T add(T a, T b) {
    return a + b;
}

int main() {
    return add(5, 3);
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);

  // Template-dependent types should use __maybe_primop_ or similar
  EXPECT_TRUE(result.find("ow_add") != std::string::npos ||
              result.find("__maybe_primop_add") != std::string::npos ||
              result.find("__primop_add") != std::string::npos)
      << "Template arithmetic should be transformed with SFINAE handling. Result:\n" << result;
}

// ============================================================================
// Transformation Statistics Tests
// ============================================================================

TEST_F(ArithmeticOpsTransformationTest, TransformationCountsCorrectly) {
  std::string source = R"(
int main() {
    int a = 1, b = 2, c = 3;
    int x = a + b;
    int y = b * c;
    int z = x - y;
    return z;
}
)";

  TransformationConfig config;
  config.transform_arithmetic_operators = true;
  config.transform_array_subscripts = false;

  std::string result = transformSource(source, config);
  const auto& stats = getLastStats();

  // Should have transformed 3 arithmetic operations
  EXPECT_EQ(stats.arithmetic_ops_transformed, 3u)
      << "Should have transformed exactly 3 arithmetic operations";
}
