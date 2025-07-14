#pragma once

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <cmath>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace optiweave::core {

/**
    @brief Configuration for AST transformation
*/
struct TransformationConfig {
  bool transform_array_subscripts = true;
  bool transform_arithmetic_operators = false;
  bool transform_assignment_operators = false;
  bool transform_comparisons_operators = false; // Fixed typo
  bool preserve_templates = true;
  bool skip_system_headers = true;
  std::string prelude_path;
  std::vector<std::string> include_paths;
};

/**
    @brief Statistics collected during transformation
*/
struct TransformationStats {
  size_t array_subscripts_transformed = 0;
  size_t arithmetic_ops_transformed = 0;
  size_t template_instantiations_skipped = 0;
  size_t errors_encountered = 0;

  void reset() { *this = TransformationStats{}; }

  void print(llvm::raw_ostream &os) const;
};

/**
    @brief AST visitor for operator instrumentation
    This visitor implements a post-order traversal to ensure inner expressions
   are processed before outer ones
*/

class ModernASTVisitor : public clang::RecursiveASTVisitor<ModernASTVisitor> {
public:
  explicit ModernASTVisitor(clang::Rewriter &rewriter,
                            clang::ASTContext &context,
                            const TransformationConfig &config = {});
  ~ModernASTVisitor() = default;

  // Disable copy/move to avoid issues with references
  ModernASTVisitor(const ModernASTVisitor &) = delete;
  ModernASTVisitor &operator=(const ModernASTVisitor &) = delete;
  ModernASTVisitor(ModernASTVisitor &&) = delete;
  ModernASTVisitor &operator=(ModernASTVisitor &&) = delete;

  /**
      @brief Configure post-order traversal
      @return true to enable post-order traversal
  */

  bool shouldTraversePostOrder() const { return true; }
  /**
      @brief Visit array subscript expressions
      @param expr The array subscript expression
      @return true to continue traversal
  */

  bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr *expr);

  /**
      @brief Visit binary operators (arithmetic, assignment, etc. )
      @param expr The binary operator expression
      @return true to continue traversal
  */

  bool VisitBinaryOperator(clang::BinaryOperator *expr);

  /**
      @brief Visit unary operators
      @param expr The unary operator expression
      @return true to continue traversal
  */

  bool VisitUnaryOperator(clang::UnaryOperator *expr);
  /**
      @brief Visit C++ operator call expressions (overloaded operators)
      @param expr The operator call expression
      @return true to continue traversal
  */

  bool VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *expr);

  /**
      @brief Get transformation Statistics
      @return const reference to stats
  */

  const TransformationStats &getStats() const { return stats_; }

  /**
      @brief Reset transformation statistics
  */

  void resetStats() { stats_.reset(); }

private:
  clang::Rewriter &rewriter_;
  clang::ASTContext &context_;
  TransformationConfig config_;
  TransformationStats stats_;

  // Track processed source ranges to avoid double-processing
  std::set<std::pair<unsigned, unsigned>> processed_ranges_;

  /**
      @brief Check if we should skip this expression based on context
      @param expr The expression to check
      @return true if should skip
  */

  bool shouldSkipExpression(const clang::Expr *expr) const;

  /**
      @brief Check if expression is in a system header
      @param expr The expression to check
      @return true if in system header
  */

  bool isInSystemHeader(const clang::Expr *expr) const;

  /**
      @brief Check if expression is already processed
      @param expr The expression to check
      @return true if already processed
  */

  bool isAlreadyProcessed(const clang::Expr *expr);

  /**
      @brief Mark expression as processed
      @param expr The expression to mark
  */

  void markAsProcessed(const clang::Expr *expr);

  /**
      @brief Transform array subscript expression
      @param expr The array subscript expression
      @return true on success
  */

  bool transformArraySubscript(clang::ArraySubscriptExpr *expr);

  /**
      @brief Transform binary operator expression
      @param expr The binary operator expression
      @return true on success
  */

  bool transformBinaryOperator(clang::BinaryOperator *expr);

  /**
      @brief Generate instrumentation code for array subscript
      @param lhs_type The type of the left-hand side
      @param lhs_text The text of the left-hand side
      @param rhs_text The text of the right-hand side
      @return Generated instrumentation code
  */
  std::string
  generateArraySubscriptInstrumentation(clang::QualType lhs_type,
                                        llvm::StringRef lhs_text,
                                        llvm::StringRef rhs_text) const;

  /**
      @brief Generate instrumentation code for binary operator
      @param op The binary operator kind
      @param lhs_type The type of the left-hand side
      @param rhs_type The type of the right-hand side
      @param lhs_text The text of the left-hand side
      @param rhs_text The text of the right-hand side
      @return Generated instrumentation code
   */

  std::string generateBinaryOperatorInstrumentation(
      clang::BinaryOperatorKind op, clang::QualType lhs_type,
      clang::QualType rhs_type, llvm::StringRef lhs_text,
      llvm::StringRef rhs_text) const;

  /**
    @brief Check if type is template-dependent
    @param type The type to check
    @return true if dependent
   */

  bool isTemplateDependentType(clang::QualType type) const;

  /**
    @brief Get safe replacement text for source range
    @param range The source range
    @return Text content
   */
  std::string getSourceText(clang::SourceRange range) const;
};

/**
  @brief AST Consumer that owns and manages the visitor
 */
class TransformationConsumer : public clang::ASTConsumer {
public:
  explicit TransformationConsumer(clang::Rewriter &rewriter,
                                  clang::ASTContext &context,
                                  const TransformationConfig &config = {});

  void HandleTranslationUnit(clang::ASTContext &context) override;

  const TransformationStats &getStats() const;

private:
  std::unique_ptr<ModernASTVisitor> visitor_;
  clang::ASTContext &context_;
};

} // namespace optiweave::core