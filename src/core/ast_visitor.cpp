#include "../../include/optiweave/core/ast_visitor.hpp"
#include <clang/AST/ParentMapContext.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/raw_ostream.h>

#include <sstream>

namespace optiweave::core {
namespace {
/**
    @brief Get the string representation of a binary operator
*/
const char *getBinaryOperatorSpelling(clang::BinaryOperatorKind op) {
  switch (op) {
  case clang::BO_Add:
    return "+";
  case clang::BO_Sub:
    return "-";
  case clang::BO_Mul:
    return "*";
  case clang::BO_Div:
    return "/";
  case clang::BO_Rem:
    return "%";
  case clang::BO_Assign:
    return "=";
  case clang::BO_AddAssign:
    return "+=";
  case clang::BO_SubAssign:
    return "-=";
  case clang::BO_MulAssign:
    return "*=";
  case clang::BO_DivAssign:
    return "/=";
  case clang::BO_RemAssign:
    return "%=";
  case clang::BO_EQ:
    return "==";
  case clang::BO_NE:
    return "!=";
  case clang::BO_LT:
    return "<";
  case clang::BO_GT:
    return ">";
  case clang::BO_LE:
    return "<=";
  case clang::BO_GE:
    return ">=";
  default:
    return "unknown";
  }
}
} // namespace

void TransformationStats::print(llvm::raw_ostream &os) const {
  os << "Transformation Statistics:\n";
  os << "  Array subscripts transformed: " << array_subscripts_transformed
     << "\n";
  os << "  Arithmetic operators transformed: " << arithmetic_ops_transformed
     << "\n";
  os << "  Template instantiations skipped: "
     << template_instantiations_skipped << "\n";
  os << "  Errors encountered: " << errors_encountered << "\n";
}

ModernASTVisitor::ModernASTVisitor(clang::Rewriter &rewriter,
                                   clang::ASTContext &context,
                                   const TransformationConfig &config)
    : rewriter_(rewriter), context_(context), config_(config) {}

bool ModernASTVisitor::VisitArraySubscriptExpr(clang::ArraySubscriptExpr *expr) {
  if (shouldSkipExpression(expr)) {
    return true;
  }

  if (isAlreadyProcessed(expr)) {
    return true;
  }

  if (config_.transform_array_subscripts) {
    if (transformArraySubscript(expr)) {
      markAsProcessed(expr);
      ++stats_.array_subscripts_transformed;
    } else {
      ++stats_.errors_encountered;
    }
  }
  return true;
}

bool ModernASTVisitor::VisitBinaryOperator(clang::BinaryOperator *expr) {
  if (shouldSkipExpression(expr)) {
    return true;
  }

  if (isAlreadyProcessed(expr)) {
    return true;
  }

  // Check if we should transform this operator type
  bool should_transform = false;
  if (expr->isArithmeticOp() && config_.transform_arithmetic_operators) {
    should_transform = true;
  } else if (expr->isAssignmentOp() &&
             config_.transform_assignment_operators) {
    should_transform = true;
  } else if (expr->isComparisonOp() &&
             config_.transform_comparisons_operators) {
    should_transform = true;
  }

  if (should_transform) {
    if (transformBinaryOperator(expr)) {
      markAsProcessed(expr);
      ++stats_.arithmetic_ops_transformed;
    } else {
      ++stats_.errors_encountered;
    }
  }

  return true;
}

bool ModernASTVisitor::VisitUnaryOperator(clang::UnaryOperator *expr) {
  // TODO: Implement unary operator transformation
  return true;
}

bool ModernASTVisitor::VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *expr) {
  // TODO: Implement overloaded operator transformation
  return true;
}

bool ModernASTVisitor::shouldSkipExpression(const clang::Expr *expr) const {
  // Skip if in system header and configured to do so
  if (config_.skip_system_headers && isInSystemHeader(expr)) {
    return true;
  }

  // Check for problematic contexts (sizeof, alignof, etc.)
  auto parents = context_.getParents(*expr);
  for (const auto &parent_node : parents) {
    if (const auto *stmt = parent_node.get<clang::Stmt>()) {
      // Skip expressions under address-of operator
      if (const auto *unary_op =
              clang::dyn_cast<clang::UnaryOperator>(stmt)) {
        if (unary_op->getOpcode() == clang::UO_AddrOf) {
          return true;
        }
      }

      // Skip expressions under sizeof, alignof, etc.
      if (clang::isa<clang::UnaryExprOrTypeTraitExpr>(stmt)) {
        return true;
      }
    }
  }

  return false;
}

bool ModernASTVisitor::isInSystemHeader(const clang::Expr *expr) const {
  auto &source_manager = context_.getSourceManager();
  auto location = expr->getBeginLoc();
  return source_manager.isInSystemHeader(location);
}

bool ModernASTVisitor::isAlreadyProcessed(const clang::Expr *expr) {
  auto &source_manager = context_.getSourceManager();
  auto begin_offset = source_manager.getFileOffset(expr->getBeginLoc());
  auto end_offset = source_manager.getFileOffset(expr->getEndLoc());

  auto range_key = std::make_pair(begin_offset, end_offset);
  return processed_ranges_.find(range_key) != processed_ranges_.end();
}

void ModernASTVisitor::markAsProcessed(const clang::Expr *expr) {
  auto &source_manager = context_.getSourceManager();
  auto begin_offset = source_manager.getFileOffset(expr->getBeginLoc());
  auto end_offset = source_manager.getFileOffset(expr->getEndLoc());

  processed_ranges_.insert(std::make_pair(begin_offset, end_offset));
}

bool ModernASTVisitor::transformArraySubscript(clang::ArraySubscriptExpr *expr) {
  try {
    auto lhs = expr->getLHS();
    auto rhs = expr->getRHS();

    // Get source text for operands
    std::string lhs_text = getSourceText(lhs->getSourceRange());
    std::string rhs_text = getSourceText(rhs->getSourceRange());

    if (lhs_text.empty() || rhs_text.empty()) {
      llvm::errs()
          << "Warning: Could not extract source text for array subscript\n";
      return false;
    }

    // Generate instrumentation
    std::string instrumentation = generateArraySubscriptInstrumentation(
        lhs->getType(), lhs_text, rhs_text);

    // Apply transformation
    auto source_range = expr->getSourceRange();
    if (rewriter_.ReplaceText(source_range, instrumentation)) {
      llvm::errs()
          << "Error: Failed to apply array subscript transformation\n";
      return false;
    }

    return true;
  } catch (const std::exception &e) {
    llvm::errs() << "Exception in transformArraySubscript: " << e.what()
                 << "\n";
    return false;
  }
}

bool ModernASTVisitor::transformBinaryOperator(clang::BinaryOperator *expr) {
  try {
    auto lhs = expr->getLHS();
    auto rhs = expr->getRHS();

    // Get source text for operands
    std::string lhs_text = getSourceText(lhs->getSourceRange());
    std::string rhs_text = getSourceText(rhs->getSourceRange());

    if (lhs_text.empty() || rhs_text.empty()) {
      llvm::errs()
          << "Warning: Could not extract source text for binary operator\n";
      return false;
    }

    // Generate instrumentation
    std::string instrumentation = generateBinaryOperatorInstrumentation(
        expr->getOpcode(), lhs->getType(), rhs->getType(), lhs_text,
        rhs_text);

    // Apply transformation
    auto source_range = expr->getSourceRange();
    if (rewriter_.ReplaceText(source_range, instrumentation)) {
      llvm::errs()
          << "Error: Failed to apply binary operator transformation\n";
      return false;
    }

    return true;
  } catch (const std::exception &e) {
    llvm::errs() << "Exception in transformBinaryOperator: " << e.what()
                 << "\n";
    return false;
  }
}

std::string ModernASTVisitor::generateArraySubscriptInstrumentation(
    clang::QualType lhs_type, llvm::StringRef lhs_text,
    llvm::StringRef rhs_text) const {

  std::ostringstream oss;

  if (isTemplateDependentType(lhs_type)) {
    // Template-dependent case - use runtime type detection
    oss << "__maybe_primop_subscript<"
        << "decltype(" << lhs_text << "), "
        << "!__has_subscript_overload<decltype(" << lhs_text << ")>::value"
        << ">()(" << lhs_text << ", " << rhs_text << ")";
  } else {
    // Non-template case - use compile-time type
    std::string type_str = lhs_type.getAsString(context_.getPrintingPolicy());
    oss << "__primop_subscript<" << type_str << ">()"
        << "(" << lhs_text << ", " << rhs_text << ")";
  }

  return oss.str();
}

std::string ModernASTVisitor::generateBinaryOperatorInstrumentation(
    clang::BinaryOperatorKind op, clang::QualType lhs_type,
    clang::QualType rhs_type, llvm::StringRef lhs_text,
    llvm::StringRef rhs_text) const {

  std::ostringstream oss;
  const char *op_name = getBinaryOperatorSpelling(op);

  if (isTemplateDependentType(lhs_type) ||
      isTemplateDependentType(rhs_type)) {
    // Template-dependent case
    oss << "__maybe_primop_" << op_name << "<"
        << "decltype(" << lhs_text << "), "
        << "decltype(" << rhs_text << ")"
        << ">()(" << lhs_text << ", " << rhs_text << ")";
  } else {
    // Non-template case
    std::string lhs_type_str =
        lhs_type.getAsString(context_.getPrintingPolicy());
    std::string rhs_type_str =
        rhs_type.getAsString(context_.getPrintingPolicy());
    oss << "__primop_" << op_name << "<" << lhs_type_str << ", "
        << rhs_type_str << ">()"
        << "(" << lhs_text << ", " << rhs_text << ")";
  }

  return oss.str();
}

bool ModernASTVisitor::isTemplateDependentType(clang::QualType type) const {
  return type->isDependentType() || type->isInstantiationDependentType() ||
         type->isTemplateTypeParmType() || type->isUndeducedType();
}

std::string ModernASTVisitor::getSourceText(clang::SourceRange range) const {
  auto &source_manager = context_.getSourceManager();
  auto &lang_opts = context_.getLangOpts();

  // Use Clang's Lexer to get the exact source text
  auto char_range = clang::CharSourceRange::getTokenRange(range);
  bool invalid = false;
  auto text = clang::Lexer::getSourceText(char_range, source_manager,
                                          lang_opts, &invalid);

  if (invalid) {
    llvm::errs() << "Warning: Could not get source text for range\n";
    return "";
  }

  return text.str();
}

// TransformationConsumer implementation
TransformationConsumer::TransformationConsumer(
    clang::Rewriter &rewriter, clang::ASTContext &context,
    const TransformationConfig &config)
    : context_(context) {
  visitor_ = std::make_unique<ModernASTVisitor>(rewriter, context, config);
}

void TransformationConsumer::HandleTranslationUnit(clang::ASTContext &context) {
  // Set traversal scope to the entire translation unit
  context.setTraversalScope({context.getTranslationUnitDecl()});

  // Traverse the AST
  visitor_->TraverseDecl(context.getTranslationUnitDecl());

  // Print statistics
  llvm::errs() << "=== Transformation Complete ===\n";
  visitor_->getStats().print(llvm::errs());
}

const TransformationStats &TransformationConsumer::getStats() const {
  return visitor_->getStats();
}

} // namespace optiweave::core