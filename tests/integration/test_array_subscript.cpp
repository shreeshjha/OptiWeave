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

class ArraySubscriptTransformationTest : public ::testing::Test {
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
  bool transformSource(const std::string &source_code,
                              const TransformationConfig &config = {}) {
    // Create a unique filename for this test
    std::string filename =
        "/tmp/test_" + std::to_string(++test_counter_) + ".cpp";

    // Run transformation
    auto result = runToolOnCode(std::make_unique<TransformationAction>(config),
                                source_code, filename);

    return result;
  }

  /**
   * @brief Custom frontend action for testing
   */
  class TransformationAction : public ASTFrontendAction {
  public:
    explicit TransformationAction(const TransformationConfig &config)
        : config_(config) {}

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                   StringRef file) override {
      rewriter_.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
      consumer_ = std::make_unique<TransformationConsumer>(
          rewriter_, CI.getASTContext(), config_);
      return std::unique_ptr<ASTConsumer>(consumer_.get());
    }

    void EndSourceFileAction() override {
      // Capture the transformed code
      auto &source_manager = rewriter_.getSourceMgr();
      auto main_file_id = source_manager.getMainFileID();

      if (auto buffer = rewriter_.getRewriteBufferFor(main_file_id)) {
        transformed_code_ = std::string(buffer->begin(), buffer->end());
      } else {
        // No changes were made, return original
        auto file_entry = source_manager.getFileEntryForID(main_file_id);
        if (file_entry) {
          auto buffer = source_manager.getBufferData(main_file_id);
          transformed_code_ = buffer->getBufferData().str();
        }
      }
    }

    const std::string &getTransformedCode() const { return transformed_code_; }

    const TransformationStats &getStats() const {
      return consumer_->getStats();
    }

  private:
    TransformationConfig config_;
    Rewriter rewriter_;
    std::unique_ptr<TransformationConsumer> consumer_;
    std::string transformed_code_;
  };

private:
  static int test_counter_;
};

int ArraySubscriptTransformationTest::test_counter_ = 0;

TEST_F(ArraySubscriptTransformationTest, BasicArrayAccess) {
  std::string source = R"(
int main() {
    int arr[10];
    return arr[5];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Check that the array access was transformed
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  EXPECT_TRUE(result.find("arr[5]") == std::string::npos);
}

TEST_F(ArraySubscriptTransformationTest, PointerAccess) {
  std::string source = R"(
int main() {
    int* ptr = nullptr;
    return ptr[3];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Check transformation
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  EXPECT_TRUE(result.find("ptr[3]") == std::string::npos);
}

TEST_F(ArraySubscriptTransformationTest, MultiDimensionalArray) {
  std::string source = R"(
int main() {
    int matrix[5][5];
    return matrix[2][3];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should transform both subscript operations
  size_t primop_count = 0;
  size_t pos = 0;
  while ((pos = result.find("__primop_subscript", pos)) != std::string::npos) {
    ++primop_count;
    ++pos;
  }

  EXPECT_EQ(primop_count, 2) << "Should transform both subscript operations";
}

TEST_F(ArraySubscriptTransformationTest, TemplateArrayAccess) {
  std::string source = R"(
template<typename T>
T access_element(T* arr, int index) {
    return arr[index];
}

int main() {
    int arr[10];
    return access_element(arr, 5);
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Template-dependent array access should use maybe_primop
  EXPECT_TRUE(result.find("__maybe_primop_subscript") != std::string::npos);
}

TEST_F(ArraySubscriptTransformationTest, OverloadedSubscriptOperator) {
  std::string source = R"(
class MyArray {
public:
    int& operator[](int index) { return data[index]; }
private:
    int data[100];
};

int main() {
    MyArray arr;
    return arr[10];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should NOT transform overloaded operator calls (they're already
  // instrumented) This should remain as arr[10] or be handled differently The
  // exact behavior depends on how we handle CXXOperatorCallExpr
}

TEST_F(ArraySubscriptTransformationTest, NestedExpressions) {
  std::string source = R"(
int main() {
    int arr[10];
    int indices[5] = {1, 2, 3, 4, 5};
    return arr[indices[2]];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Both array accesses should be transformed
  size_t primop_count = 0;
  size_t pos = 0;
  while ((pos = result.find("__primop_subscript", pos)) != std::string::npos) {
    ++primop_count;
    ++pos;
  }

  EXPECT_EQ(primop_count, 2);
}

TEST_F(ArraySubscriptTransformationTest, DisabledTransformation) {
  std::string source = R"(
int main() {
    int arr[10];
    return arr[5];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = false; // Disabled

  bool result = transformSource(source, config);

  // Should not be transformed
  EXPECT_TRUE(result.find("__primop_subscript") == std::string::npos);
  EXPECT_TRUE(result.find("arr[5]") != std::string::npos);
}

TEST_F(ArraySubscriptTransformationTest, SystemHeadersSkipped) {
  std::string source = R"(
#include <vector>
int main() {
    std::vector<int> vec = {1, 2, 3};
    int arr[3] = {1, 2, 3};
    return arr[1] + vec[1];  // Only arr[1] should be transformed
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;
  config.skip_system_headers = true;

  bool result = transformSource(source, config);

  // Should transform local array access but not std::vector
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  // The vec[1] might still be visible depending on how includes are handled
}

TEST_F(ArraySubscriptTransformationTest, AddressOfExpression) {
  std::string source = R"(
int main() {
    int arr[10];
    int* ptr = &arr[5];  // Should NOT be transformed (addr-of context)
    return arr[3];       // Should be transformed
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should only transform the second array access
  size_t primop_count = 0;
  size_t pos = 0;
  while ((pos = result.find("__primop_subscript", pos)) != std::string::npos) {
    ++primop_count;
    ++pos;
  }

  EXPECT_EQ(primop_count, 1) << "Should only transform arr[3], not &arr[5]";
}

TEST_F(ArraySubscriptTransformationTest, SizeofExpression) {
  std::string source = R"(
int main() {
    int arr[10];
    size_t size = sizeof(arr[0]);  // Should NOT be transformed
    return arr[1];                 // Should be transformed
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should only transform the second array access
  size_t primop_count = 0;
  size_t pos = 0;
  while ((pos = result.find("__primop_subscript", pos)) != std::string::npos) {
    ++primop_count;
    ++pos;
  }

  EXPECT_EQ(primop_count, 1)
      << "Should only transform arr[1], not sizeof(arr[0])";
}

// Performance and stress tests
TEST_F(ArraySubscriptTransformationTest, LargeNumberOfArrayAccesses) {
  std::string source = R"(
int main() {
    int arr[1000];
    int sum = 0;
    for (int i = 0; i < 100; ++i) {
        sum += arr[i];
    }
    return sum;
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should transform the array access in the loop
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  EXPECT_TRUE(result.find("arr[i]") == std::string::npos);
}

TEST_F(ArraySubscriptTransformationTest, ComplexExpressionAsIndex) {
  std::string source = R"(
int main() {
    int arr[100];
    int x = 5, y = 10;
    return arr[x * y + 2];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should preserve the complex index expression
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  EXPECT_TRUE(result.find("x * y + 2") != std::string::npos);
}

TEST_F(ArraySubscriptTransformationTest, ArrayOfPointers) {
  std::string source = R"(
int main() {
    int a = 1, b = 2, c = 3;
    int* arr[3] = {&a, &b, &c};
    return *arr[1];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should transform arr[1] but not affect the dereference
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  EXPECT_TRUE(result.find("*") != std::string::npos);
}

TEST_F(ArraySubscriptTransformationTest, FunctionReturningArray) {
  std::string source = R"(
int* getArray() {
    static int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    return arr;
}

int main() {
    return getArray()[3];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should transform getArray()[3]
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  EXPECT_TRUE(result.find("getArray()[3]") == std::string::npos);
}

TEST_F(ArraySubscriptTransformationTest, ArrayAccessInMacro) {
  std::string source = R"(
#define GET_ELEMENT(arr, idx) arr[idx]

int main() {
    int arr[10];
    return GET_ELEMENT(arr, 5);
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Macro expansion should be handled correctly
  // The exact behavior depends on whether we see the expanded or unexpanded
  // form
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
}

// Error handling tests
TEST_F(ArraySubscriptTransformationTest, InvalidArrayAccess) {
  std::string source = R"(
int main() {
    int arr[10];
    return arr[15];  // Out of bounds, but should still be transformed
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should transform even invalid accesses
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  EXPECT_TRUE(result.find("arr[15]") == std::string::npos);
}

TEST_F(ArraySubscriptTransformationTest, NullPointerAccess) {
  std::string source = R"(
int main() {
    int* ptr = nullptr;
    return ptr[0];  // Dangerous, but should be transformed
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should transform even dangerous accesses
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  EXPECT_TRUE(result.find("ptr[0]") == std::string::npos);
}

// Test with various type combinations
TEST_F(ArraySubscriptTransformationTest, DifferentIndexTypes) {
  std::string source = R"(
int main() {
    int arr[100];
    short s = 10;
    long l = 20;
    size_t sz = 30;
    return arr[s] + arr[l] + arr[sz];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // All three array accesses should be transformed
  size_t primop_count = 0;
  size_t pos = 0;
  while ((pos = result.find("__primop_subscript", pos)) != std::string::npos) {
    ++primop_count;
    ++pos;
  }

  EXPECT_EQ(primop_count, 3);
}

TEST_F(ArraySubscriptTransformationTest, ConstArrayAccess) {
  std::string source = R"(
int main() {
    const int arr[5] = {1, 2, 3, 4, 5};
    return arr[2];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should handle const arrays correctly
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  EXPECT_TRUE(result.find("arr[2]") == std::string::npos);
}

TEST_F(ArraySubscriptTransformationTest, VolatileArrayAccess) {
  std::string source = R"(
int main() {
    volatile int arr[5] = {1, 2, 3, 4, 5};
    return arr[2];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should handle volatile arrays correctly
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  EXPECT_TRUE(result.find("arr[2]") == std::string::npos);
}

// Integration with other language features
TEST_F(ArraySubscriptTransformationTest, ArrayAccessInLambda) {
  std::string source = R"(
int main() {
    int arr[10];
    auto lambda = [&](int index) {
        return arr[index];
    };
    return lambda(5);
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  bool result = transformSource(source, config);

  // Should transform array access inside lambda
  EXPECT_TRUE(result.find("__primop_subscript") != std::string::npos);
  EXPECT_TRUE(result.find("arr[index]") == std::string::npos);
}

// Performance test with statistics
TEST_F(ArraySubscriptTransformationTest, TransformationStatistics) {
  std::string source = R"(
int main() {
    int arr1[10], arr2[20];
    int* ptr = arr1;
    return arr1[1] + arr2[2] + ptr[3];
}
)";

  TransformationConfig config;
  config.transform_array_subscripts = true;

  // We need to access the transformation action to get statistics
  // This would require modifying the transformSource helper
  // For now, just verify the transformation occurred
  bool result = transformSource(source, config);

  size_t primop_count = 0;
  size_t pos = 0;
  while ((pos = result.find("__primop_subscript", pos)) != std::string::npos) {
    ++primop_count;
    ++pos;
  }

  EXPECT_EQ(primop_count, 3) << "Should transform all three array accesses";
}
