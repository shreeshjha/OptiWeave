#include "../../include/optiweave/core/ast_visitor.hpp"
#include "../../include/optiweave/runtime/loop_info_serializer.hpp"
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

const char *getBinaryOperatorTemplateName(clang::BinaryOperatorKind op) {
  switch (op) {
  case clang::BO_Add:
    return "add";
  case clang::BO_Sub:
    return "sub";
  case clang::BO_Mul:
    return "mul";
  case clang::BO_Div:
    return "div";
  case clang::BO_Rem:
    return "rem";
  case clang::BO_Assign:
    return "assign";
  case clang::BO_AddAssign:
    return "add_assign";
  case clang::BO_SubAssign:
    return "sub_assign";
  case clang::BO_MulAssign:
    return "mul_assign";
  case clang::BO_DivAssign:
    return "div_assign";
  case clang::BO_RemAssign:
    return "rem_assign";
  case clang::BO_EQ:
    return "eq";
  case clang::BO_NE:
    return "ne";
  case clang::BO_LT:
    return "lt";
  case clang::BO_GT:
    return "gt";
  case clang::BO_LE:
    return "le";
  case clang::BO_GE:
    return "ge";
  default:
    return "unknown";
  }
}

// Helper functions to check operator types - fixed for LLVM 17
bool isArithmeticOp(clang::BinaryOperatorKind op) {
    return op == clang::BO_Add || op == clang::BO_Sub || 
           op == clang::BO_Mul || op == clang::BO_Div || 
           op == clang::BO_Rem;
}

bool isAssignmentOp(clang::BinaryOperatorKind op) {
    return op == clang::BO_Assign || op == clang::BO_AddAssign || 
           op == clang::BO_SubAssign || op == clang::BO_MulAssign || 
           op == clang::BO_DivAssign || op == clang::BO_RemAssign;
}

bool isComparisonOp(clang::BinaryOperatorKind op) {
    return op == clang::BO_EQ || op == clang::BO_NE || 
           op == clang::BO_LT || op == clang::BO_GT || 
           op == clang::BO_LE || op == clang::BO_GE;
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

bool ModernASTVisitor::VisitArraySubscriptExpr(clang::ArraySubscriptExpr *
                                               expr) {
  if (shouldSkipExpression(expr)) {
    return true;
  }

  if (isAlreadyProcessed(expr)) {
    return true;
  }

  if (!config_.transform_array_subscripts) {
    return true;
  }

  // NOTE: LHS assignments with array subscripts are now handled in TraverseBinaryOperator
  // This visitor only handles regular READ contexts: x = arr[i], arr[i] + 1, etc.

  // Regular READ context: x = arr[i]
  if (transformArraySubscript(expr)) {
    markAsProcessed(expr);
    ++stats_.array_subscripts_transformed;
  } else {
    ++stats_.errors_encountered;
  }

  return true;
}

bool ModernASTVisitor::TraverseUnaryOperator(clang::UnaryOperator *expr) {
  // IMPORTANT: Skip array subscripts that are operands of increment/decrement
  // These operations require lvalue references which C doesn't support
  if (!shouldSkipExpression(expr) && !isAlreadyProcessed(expr)) {
    auto opcode = expr->getOpcode();
    if (opcode == clang::UO_PreInc || opcode == clang::UO_PreDec ||
        opcode == clang::UO_PostInc || opcode == clang::UO_PostDec) {
      if (auto *sub_expr = clang::dyn_cast<clang::ArraySubscriptExpr>(expr->getSubExpr())) {
        // This is an increment/decrement on an array subscript: arr[i]++, ++arr[i], etc.
        // Mark it as processed so we don't try to transform it
        markAsProcessed(sub_expr);
        markAsProcessed(expr);
        // Don't count as error - this is an expected limitation
        return true; // Skip this entire subtree
      }
    }
  }

  // Default traversal for all other cases
  return RecursiveASTVisitor::TraverseUnaryOperator(expr);
}

bool ModernASTVisitor::TraverseBinaryOperator(clang::BinaryOperator *expr) {
  // IMPORTANT: Handle assignments with array subscripts on LHS FIRST, before children are visited
  // This prevents the RHS from being corrupted when we try to extract the assignment text
  if (!shouldSkipExpression(expr) && !isAlreadyProcessed(expr)) {
    // Skip compound assignments on array subscripts - C doesn't support lvalue references
    if (config_.transform_array_subscripts && expr->isCompoundAssignmentOp()) {
      if (auto *lhs_subscript = clang::dyn_cast<clang::ArraySubscriptExpr>(expr->getLHS())) {
        // This is a compound assignment with array subscript on LHS: arr[i] += val
        // Mark it as processed so we don't try to transform it
        markAsProcessed(lhs_subscript);
        markAsProcessed(expr);
        // Don't count as error - this is an expected limitation
        return RecursiveASTVisitor::TraverseBinaryOperator(expr); // Continue with default traversal
      }
    }

    if (config_.transform_array_subscripts && expr->isAssignmentOp() && !expr->isCompoundAssignmentOp()) {
      if (auto *lhs_subscript = clang::dyn_cast<clang::ArraySubscriptExpr>(expr->getLHS())) {
        // This is an assignment with array subscript on LHS: arr[i] = ...
        // Transform the ENTIRE assignment before visiting children
        if (transformAssignmentWithArraySubscript(lhs_subscript)) {
          markAsProcessed(lhs_subscript);  // Mark LHS subscript as processed
          markAsProcessed(expr);           // Mark assignment as processed
          ++stats_.array_subscripts_transformed;
          return true; // Don't traverse children - we already handled the whole assignment
        } else {
          ++stats_.errors_encountered;
          return true; // Continue but don't visit children
        }
      }
    }
  }

  // Default traversal for all other cases
  return RecursiveASTVisitor::TraverseBinaryOperator(expr);
}

bool ModernASTVisitor::VisitBinaryOperator(clang::BinaryOperator *expr) {
  if (shouldSkipExpression(expr)) {
    return true;
  }

  if (isAlreadyProcessed(expr)) {
    return true;
  }

  // NOTE: LHS assignments with array subscripts are now handled in TraverseBinaryOperator

  // Check if we should transform this operator type
  bool should_transform = false;
  auto opcode = expr->getOpcode();
  if (isArithmeticOp(opcode) && config_.transform_arithmetic_operators) {
    should_transform = true;
  } else if (isAssignmentOp(opcode) &&
             config_.transform_assignment_operators) {
    should_transform = true;
  } else if (isComparisonOp(opcode) &&
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

bool ModernASTVisitor::VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *
                                                expr) {
  // TODO: Implement overloaded operator transformation
  return true;
}

bool ModernASTVisitor::shouldSkipExpression(const clang::Expr *expr) const {
  // Skip if in system header and configured to do so
  if (config_.skip_system_headers && isInSystemHeader(expr)) {
    return true;
  }

  // Skip if in OptiWeave prelude/templates (to avoid transforming our own instrumentation code)
  auto &source_manager = context_.getSourceManager();
  auto location = expr->getBeginLoc();
  if (location.isValid()) {
    auto file_entry = source_manager.getFileEntryRefForID(source_manager.getFileID(location));
    if (file_entry) {
      llvm::StringRef filename = file_entry->getName();
      // Skip if the file is in templates/ directory or is named prelude.hpp
      if (filename.contains("/templates/") || filename.ends_with("prelude.hpp") ||
          filename.ends_with("optiweave/prelude.hpp") ||
          filename.ends_with("optiweave/prelude_c.h") ||
          filename.contains("/optiweave/")) {
        return true;
      }
    }
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
        // Skip array subscripts under increment/decrement operators
        // These require lvalue references which C doesn't support
        if (unary_op->getOpcode() == clang::UO_PreInc ||
            unary_op->getOpcode() == clang::UO_PreDec ||
            unary_op->getOpcode() == clang::UO_PostInc ||
            unary_op->getOpcode() == clang::UO_PostDec) {
          return true;
        }
      }

      // Skip array subscripts on LHS of compound assignments
      // These also require lvalue references which C doesn't support
      if (const auto *bin_op = clang::dyn_cast<clang::BinaryOperator>(stmt)) {
        if (bin_op->isCompoundAssignmentOp()) {
          // Check if this expression is the LHS of the compound assignment
          if (bin_op->getLHS() == expr) {
            return true;
          }
        }
      }

      // Skip array subscripts followed by member access (arr[i].field or arr[i]->field)
      // Transformation returns a value, not an lvalue, so member access on LHS won't work
      if (const auto *member_expr = clang::dyn_cast<clang::MemberExpr>(stmt)) {
        // Check if this array subscript is the base of the member access
        const clang::Expr *base = member_expr->getBase();
        // Strip away implicit casts and parentheses
        while (base) {
          if (auto *ice = clang::dyn_cast<clang::ImplicitCastExpr>(base)) {
            base = ice->getSubExpr();
          } else if (auto *pe = clang::dyn_cast<clang::ParenExpr>(base)) {
            base = pe->getSubExpr();
          } else {
            break;
          }
        }
        if (base == expr) {
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

bool ModernASTVisitor::isArraySubscriptOnLHSOfAssignment(const clang::ArraySubscriptExpr *expr) const {
  // Check if this array subscript is the direct LHS of an assignment operator
  // Example: arr[i] = 5;  <- we want to detect this pattern

  auto parents = context_.getParents(*expr);
  if (parents.empty()) {
    return false;
  }

  // Check immediate parent
  for (const auto &parent_node : parents) {
    if (const auto *binary_op = parent_node.get<clang::BinaryOperator>()) {
      // Check if this is an assignment operator
      if (binary_op->isAssignmentOp()) {
        // Check if our array subscript is the LHS of this assignment
        if (binary_op->getLHS() == expr) {
          return true; // Found it: arr[i] = value
        }
      }
    }
  }

  return false;
}

bool ModernASTVisitor::isPartOfAssignmentWithArraySubscriptLHS(const clang::ArraySubscriptExpr *expr) const {
  // Check if this subscript is part of an assignment whose LHS has an array subscript
  // Example: arr[i] = brr[j];  <- for brr[j], we want to return true
  // This helps us avoid transforming RHS subscripts when we'll handle the whole assignment

  auto parents = context_.getParents(*expr);
  if (parents.empty()) {
    return false;
  }

  // Look for parent binary operator (assignment)
  for (const auto &parent_node : parents) {
    if (const auto *binary_op = parent_node.get<clang::BinaryOperator>()) {
      if (binary_op->isAssignmentOp()) {
        // Check if the LHS of this assignment is an array subscript
        if (clang::isa<clang::ArraySubscriptExpr>(binary_op->getLHS())) {
          return true; // This subscript is part of an assignment with array subscript on LHS
        }
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

bool ModernASTVisitor::isInSystemHeader(const clang::Stmt *stmt) const {
  auto &source_manager = context_.getSourceManager();
  auto location = stmt->getBeginLoc();
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

bool ModernASTVisitor::transformArraySubscript(clang::ArraySubscriptExpr *
                                               expr) {
  try {
    // NOTE: LHS assignments are now handled in VisitBinaryOperator
    // This function only handles READ contexts: x = arr[i], arr[i] + 1, etc.

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

    // Generate instrumentation (pass expr for source location)
    std::string instrumentation = generateArraySubscriptInstrumentation(
        expr, lhs->getType(), lhs_text, rhs_text);

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

bool ModernASTVisitor::transformAssignmentWithArraySubscript(clang::ArraySubscriptExpr *subscript_expr) {
  // Find the parent assignment operator
  auto parents = context_.getParents(*subscript_expr);
  if (parents.empty()) {
    return false;
  }

  const clang::BinaryOperator *assignment = nullptr;
  for (const auto &parent_node : parents) {
    if (const auto *binop = parent_node.get<clang::BinaryOperator>()) {
      if (binop->isAssignmentOp() && binop->getLHS() == subscript_expr) {
        assignment = binop;
        break;
      }
    }
  }

  if (!assignment) {
    return false; // Couldn't find parent assignment
  }

  // Get the source text for the entire assignment
  std::string assignment_text = getSourceText(assignment->getSourceRange());
  if (assignment_text.empty()) {
    llvm::errs() << "Warning: Could not extract assignment text\n";
    return false;
  }

  // DEBUG: Check if this assignment is already instrumented
  if (assignment_text.find("__optiweave_record_subscript") != std::string::npos ||
      assignment_text.find("__ow_subscript_impl") != std::string::npos) {
    llvm::errs() << "Warning: Skipping already-instrumented assignment\n";
    return false;
  }

  auto& SM = context_.getSourceManager();
  unsigned line = SM.getExpansionLineNumber(assignment->getBeginLoc());

  // FIX: Use block wrapper instead of comma operator to avoid semicolon issues.
  // The comma operator doesn't work well with statement-level assignments because
  // the semicolon becomes part of the expression, causing syntax errors.
  //
  // Solution: Wrap in a block: { record(); assignment; }
  // This works correctly whether the assignment is a statement or expression.

  // Just replace the assignment expression itself (don't touch the semicolon)
  clang::SourceRange replace_range = assignment->getSourceRange();

  // Generate instrumented code as a block
  std::ostringstream instrumented;
  instrumented << "({ __optiweave_record_subscript("
               << getSourceLocationLiterals(subscript_expr->getExprLoc())
               << "); " << assignment_text << "; })";  // GNU statement expression

  // Replace with instrumented version
  if (rewriter_.ReplaceText(replace_range, instrumented.str())) {
    llvm::errs() << "Error: Failed to apply assignment transformation at line " << line << "\n";
    return false;
  }

  return true;
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

std::string ModernASTVisitor::getSourceLocationLiterals(clang::SourceLocation loc) const {
  auto &source_manager = context_.getSourceManager();

  // Get the presumed location (handles #line directives)
  auto presumed_loc = source_manager.getPresumedLoc(loc);
  if (!presumed_loc.isValid()) {
    return "\"<unknown>\", 0, \"<unknown>\"";
  }

  // Extract file, line, and function name
  std::string filename = presumed_loc.getFilename();
  unsigned line = presumed_loc.getLine();

  // Escape backslashes and quotes in filename for C++ string literal
  std::string escaped_filename;
  for (char c : filename) {
    if (c == '\\' || c == '"') {
      escaped_filename += '\\';
    }
    escaped_filename += c;
  }

  std::ostringstream oss;
  oss << "\"" << escaped_filename << "\", " << line << ", __FUNCTION__";
  return oss.str();
}

std::string ModernASTVisitor::generateArraySubscriptInstrumentation(
    const clang::ArraySubscriptExpr *expr, clang::QualType lhs_type,
    llvm::StringRef lhs_text, llvm::StringRef rhs_text) const {

  // Compact helper when evaluation-safe wrappers are enabled
  // Instead of using the ow_subscript macro (which captures wrong location),
  // we directly call __ow_subscript_impl with source location literals
  if (config_.evaluation_safe_wrappers) {
    std::ostringstream helper;
    helper << getFunctionPrefix() << "__ow_subscript_impl("
           << lhs_text.str() << ", " << rhs_text.str() << ", "
           << getSourceLocationLiterals(expr->getExprLoc()) << ")";
    return helper.str();
  }

  std::ostringstream call;

  std::string prefix = getFunctionPrefix();
  if (isTemplateDependentType(lhs_type)) {
    // Template-dependent case
    call << prefix << "__maybe_primop_subscript<"
         << "decltype(__ow_lhs), "
         << "!" << prefix << "has_subscript_overload<decltype(__ow_lhs)>::value"
         << ">()(__ow_lhs, __ow_rhs)";
  } else {
    // Non-template case - use compile-time type
    std::string type_str = lhs_type.getAsString(context_.getPrintingPolicy());
    call << prefix << "__primop_subscript<" << type_str << ">()"
         << "(__ow_lhs, __ow_rhs)";
  }

  if (config_.evaluation_safe_wrappers) {
    std::ostringstream wrapped;
    wrapped << "([&]() -> decltype(auto) { "
            << "auto&& __ow_lhs = (" << lhs_text.str() << "); "
            << "auto&& __ow_rhs = (" << rhs_text.str() << "); "
            << "return " << call.str() << "; })()";
    return wrapped.str();
  }

  // Fallback: direct call without wrappers
  std::ostringstream direct;
  std::string prefix2 = getFunctionPrefix();
  if (isTemplateDependentType(lhs_type)) {
    direct << prefix2 << "__maybe_primop_subscript<"
           << "decltype(" << lhs_text.str() << "), "
           << "!" << prefix2 << "has_subscript_overload<decltype(" << lhs_text.str() << ")>::value"
           << ">()(" << lhs_text.str() << ", " << rhs_text.str() << ")";
  } else {
    std::string type_str = lhs_type.getAsString(context_.getPrintingPolicy());
    direct << prefix2 << "__primop_subscript<" << type_str << ">()"
           << "(" << lhs_text.str() << ", " << rhs_text.str() << ")";
  }
  return direct.str();
}

std::string ModernASTVisitor::generateBinaryOperatorInstrumentation(
    clang::BinaryOperatorKind op, clang::QualType lhs_type,
    clang::QualType rhs_type, llvm::StringRef lhs_text,
    llvm::StringRef rhs_text) const {

  std::string prefix = getFunctionPrefix();

  // Compact helpers for arithmetic operators when evaluation-safe wrappers are enabled
  if (config_.evaluation_safe_wrappers && isArithmeticOp(op)) {
    const char *fname = nullptr;
    switch (op) {
    case clang::BO_Add:
      fname = "ow_add";
      break;
    case clang::BO_Sub:
      fname = "ow_sub";
      break;
    case clang::BO_Mul:
      fname = "ow_mul";
      break;
    case clang::BO_Div:
      fname = "ow_div";
      break;
    case clang::BO_Rem:
      fname = "ow_rem";
      break;
    default:
      break;
    }
    if (fname) {
      std::ostringstream helper;
      helper << prefix << fname << "(" << lhs_text.str() << ", "
             << rhs_text.str() << ")";
      return helper.str();
    }
  }

  // Compact helpers for assignment operators
  if (config_.evaluation_safe_wrappers && config_.transform_assignment_operators &&
      isAssignmentOp(op)) {
    const char *fname = nullptr;
    switch (op) {
    case clang::BO_Assign:
      fname = "ow_assign";
      break;
    case clang::BO_AddAssign:
      fname = "ow_add_assign";
      break;
    case clang::BO_SubAssign:
      fname = "ow_sub_assign";
      break;
    case clang::BO_MulAssign:
      fname = "ow_mul_assign";
      break;
    case clang::BO_DivAssign:
      fname = "ow_div_assign";
      break;
    case clang::BO_RemAssign:
      fname = "ow_rem_assign";
      break;
    default:
      break;
    }
    if (fname) {
      std::ostringstream helper;
      helper << prefix << fname << "(" << lhs_text.str() << ", "
             << rhs_text.str() << ")";
      return helper.str();
    }
  }

  // Compact helpers for comparison operators
  if (config_.evaluation_safe_wrappers && config_.transform_comparisons_operators &&
      isComparisonOp(op)) {
    const char *fname = nullptr;
    switch (op) {
    case clang::BO_EQ:
      fname = "ow_eq";
      break;
    case clang::BO_NE:
      fname = "ow_ne";
      break;
    case clang::BO_LT:
      fname = "ow_lt";
      break;
    case clang::BO_GT:
      fname = "ow_gt";
      break;
    case clang::BO_LE:
      fname = "ow_le";
      break;
    case clang::BO_GE:
      fname = "ow_ge";
      break;
    default:
      break;
    }
    if (fname) {
      std::ostringstream helper;
      helper << prefix << fname << "(" << lhs_text.str() << ", "
             << rhs_text.str() << ")";
      return helper.str();
    }
  }

  std::ostringstream call;
  const char *op_template_name = getBinaryOperatorTemplateName(op);

  if (isTemplateDependentType(lhs_type) || isTemplateDependentType(rhs_type)) {
    // Template-dependent case
    call << "optiweave::__maybe_primop_" << op_template_name << "<"
         << "decltype(__ow_lhs), decltype(__ow_rhs)>()(__ow_lhs, __ow_rhs)";
  } else {
    // Non-template case
    std::string lhs_type_str = lhs_type.getAsString(context_.getPrintingPolicy());
    std::string rhs_type_str = rhs_type.getAsString(context_.getPrintingPolicy());
    call << "optiweave::__primop_" << op_template_name << "<" << lhs_type_str
         << ", " << rhs_type_str << ">()(__ow_lhs, __ow_rhs)";
  }

  if (config_.evaluation_safe_wrappers) {
    std::ostringstream wrapped;
    wrapped << "([&]() -> decltype(auto) { "
            << "auto&& __ow_lhs = (" << lhs_text.str() << "); "
            << "auto&& __ow_rhs = (" << rhs_text.str() << "); "
            << "return " << call.str() << "; })()";
    return wrapped.str();
  }

  // Fallback: direct call without wrappers
  std::ostringstream direct;
  if (isTemplateDependentType(lhs_type) || isTemplateDependentType(rhs_type)) {
    direct << "optiweave::__maybe_primop_" << op_template_name << "<"
           << "decltype(" << lhs_text.str() << "), decltype(" << rhs_text.str()
           << ")>()(" << lhs_text.str() << ", " << rhs_text.str() << ")";
  } else {
    std::string lhs_type_str = lhs_type.getAsString(context_.getPrintingPolicy());
    std::string rhs_type_str = rhs_type.getAsString(context_.getPrintingPolicy());
    direct << "optiweave::__primop_" << op_template_name << "<" << lhs_type_str
           << ", " << rhs_type_str << ">()(" << lhs_text.str() << ", "
           << rhs_text.str() << ")";
  }
  return direct.str();
}

bool ModernASTVisitor::isTemplateDependentType(clang::QualType type) const {
  return type->isDependentType() || type->isInstantiationDependentType() ||
         type->isTemplateTypeParmType() || type->isUndeducedType();
}

std::string ModernASTVisitor::getSourceText(clang::SourceRange range) const {
  // Prefer the current rewritten text (captures inner transformations),
  // then fall back to original source via Lexer if unavailable.
  // Note: Rewriter::getRewrittenText returns the edited text for the range
  // if any rewrites have been applied, otherwise the original text.
  std::string rewritten = rewriter_.getRewrittenText(range);
  if (!rewritten.empty()) {
    return rewritten;
  }

  auto &source_manager = context_.getSourceManager();
  auto &lang_opts = context_.getLangOpts();

  auto char_range = clang::CharSourceRange::getTokenRange(range);
  bool invalid = false;
  auto text =
      clang::Lexer::getSourceText(char_range, source_manager, lang_opts, &invalid);

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
    : rewriter_(rewriter), context_(context), config_(config) {
  visitor_ = std::make_unique<ModernASTVisitor>(rewriter, context, config);

  // Create dependency graph if enabled
  if (config_.enable_dependency_graph) {
    dependency_graph_ = std::make_unique<optiweave::analysis::DependencyGraph>();
  }
}

// ============================================================================
// Loop Analysis Methods
// ============================================================================

bool ModernASTVisitor::VisitForStmt(clang::ForStmt *stmt) {
  if (isInSystemHeader(stmt)) return true;

  current_loop_nesting_++;
  analyzeLoop(stmt->getBody(), stmt->getBeginLoc());
  current_loop_nesting_--;

  return true;
}

bool ModernASTVisitor::VisitWhileStmt(clang::WhileStmt *stmt) {
  if (isInSystemHeader(stmt)) return true;

  current_loop_nesting_++;
  analyzeLoop(stmt->getBody(), stmt->getBeginLoc());
  current_loop_nesting_--;

  return true;
}

bool ModernASTVisitor::VisitDoStmt(clang::DoStmt *stmt) {
  if (isInSystemHeader(stmt)) return true;

  current_loop_nesting_++;
  analyzeLoop(stmt->getBody(), stmt->getBeginLoc());
  current_loop_nesting_--;

  return true;
}

void ModernASTVisitor::analyzeLoop(clang::Stmt *loop_body, clang::SourceLocation loc) {
  if (!loop_body) return;

  optiweave::analysis::LoopInfo info;

  // Get source location
  auto &sm = context_.getSourceManager();
  auto presumed = sm.getPresumedLoc(loc);
  if (presumed.isValid()) {
    info.location.file = presumed.getFilename();
    info.location.line = presumed.getLine();
    info.location.function = "<unknown>";  // Would need more complex analysis
  }

  // Get line range of loop body
  if (loop_body) {
    auto body_start = sm.getPresumedLoc(loop_body->getBeginLoc());
    auto body_end = sm.getPresumedLoc(loop_body->getEndLoc());
    if (body_start.isValid() && body_end.isValid()) {
      info.line_start = body_start.getLine();
      info.line_end = body_end.getLine();
    }
  }

  info.nesting_level = current_loop_nesting_;
  info.has_divisions = containsDivisions(loop_body);
  info.has_strided_access = containsStridedAccess(loop_body);

  // Basic heuristics for vectorization
  info.is_vectorizable = true;  // Assume vectorizable unless proven otherwise
  // Would need more sophisticated dependency analysis

  loop_info_.push_back(info);
}

bool ModernASTVisitor::containsDivisions(clang::Stmt *stmt) const {
  if (!stmt) return false;

  // Check if this statement is a division
  if (auto *binop = llvm::dyn_cast<clang::BinaryOperator>(stmt)) {
    if (binop->getOpcode() == clang::BO_Div ||
        binop->getOpcode() == clang::BO_DivAssign) {
      return true;
    }
  }

  // Recursively check children
  for (auto *child : stmt->children()) {
    if (containsDivisions(child)) {
      return true;
    }
  }

  return false;
}

bool ModernASTVisitor::containsStridedAccess(clang::Stmt *stmt) const {
  if (!stmt) return false;

  // Simple heuristic: look for array subscripts with non-trivial index expressions
  if (auto *subscript = llvm::dyn_cast<clang::ArraySubscriptExpr>(stmt)) {
    // If index is not a simple variable, it might be strided
    auto *idx = subscript->getIdx();
    if (!llvm::isa<clang::DeclRefExpr>(idx)) {
      return true;  // Conservative: non-simple index might be strided
    }
  }

  // Recursively check children
  for (auto *child : stmt->children()) {
    if (containsStridedAccess(child)) {
      return true;
    }
  }

  return false;
}

void TransformationConsumer::HandleTranslationUnit(clang::ASTContext &
                                                   context) {
  // INJECT PRELUDE HEADER BEFORE AST TRAVERSAL
  auto &source_manager = context.getSourceManager();
  auto main_file_id = source_manager.getMainFileID();
  auto start_loc = source_manager.getLocForStartOfFile(main_file_id);

  // Use config to determine C vs C++ (set from main based on -x flag or file extension)
  bool is_cxx = !config_.is_c_language;

  const char *prelude_header = is_cxx ? "optiweave/prelude.hpp" : "optiweave/prelude_c.h";

  // Only inject if not already present
  auto buffer = source_manager.getBufferData(main_file_id);
  if (buffer.find("#include") == llvm::StringRef::npos ||
      (buffer.find("optiweave/prelude.hpp") == llvm::StringRef::npos &&
       buffer.find("optiweave/prelude_c.h") == llvm::StringRef::npos)) {
    std::string include_directive = std::string("#include <") + prelude_header + ">\n";
    rewriter_.InsertText(start_loc, include_directive, true);
  }

  // Set traversal scope to the entire translation unit
  context.setTraversalScope({context.getTranslationUnitDecl()});

  // Traverse the AST
  visitor_->TraverseDecl(context.getTranslationUnitDecl());

  // Serialize loop information for runtime analysis
  const auto& loop_info = visitor_->getLoopInfo();
  if (!loop_info.empty()) {
    std::string loop_info_file = optiweave::serialization::get_loop_info_path();
    if (optiweave::serialization::serialize_loop_info(loop_info, loop_info_file)) {
      llvm::errs() << "Loop information serialized: " << loop_info.size()
                   << " loops -> " << loop_info_file << "\n";
    }
  }

  // Print statistics
  llvm::errs() << "=== Transformation Complete ===\n";
  visitor_->getStats().print(llvm::errs());
}

const TransformationStats &TransformationConsumer::getStats() const {
  return visitor_->getStats();
}

optiweave::analysis::CallGraphBuilder* TransformationConsumer::getCallGraphBuilder() {
  if (!visitor_) {
    return nullptr;
  }
  return &visitor_->getCallGraphBuilder();
}

optiweave::analysis::DependencyGraph* TransformationConsumer::getDependencyGraph() {
  return dependency_graph_.get();
}

// ============================================================================
// Dependency Graph Tracking (Preprocessor Callbacks)
// ============================================================================

// LLVM 21+ added ModuleImported parameter to InclusionDirective
#if LLVM_VERSION_MAJOR >= 21
void DependencyTrackerPPCallbacks::InclusionDirective(
    clang::SourceLocation hash_loc,
    const clang::Token& include_tok,
    llvm::StringRef file_name,
    bool is_angled,
    clang::CharSourceRange filename_range,
    clang::OptionalFileEntryRef file,
    llvm::StringRef search_path,
    llvm::StringRef relative_path,
    const clang::Module* suggested_module,
    bool module_imported,
    clang::SrcMgr::CharacteristicKind file_type) {
#else
void DependencyTrackerPPCallbacks::InclusionDirective(
    clang::SourceLocation hash_loc,
    const clang::Token& include_tok,
    llvm::StringRef file_name,
    bool is_angled,
    clang::CharSourceRange filename_range,
    clang::OptionalFileEntryRef file,
    llvm::StringRef search_path,
    llvm::StringRef relative_path,
    const clang::Module* imported,
    clang::SrcMgr::CharacteristicKind file_type) {
#endif

  if (!dep_graph_) {
    return;
  }

  // Get the file that contains the #include directive
  auto presumed_loc = source_manager_.getPresumedLoc(hash_loc);
  if (!presumed_loc.isValid()) {
    return;
  }

  std::string from_file = presumed_loc.getFilename();

  // Get the included file path
  std::string to_file;
  if (file) {
    to_file = file->getName().str();
  } else {
    // File not found, use the filename from the directive
    to_file = file_name.str();
  }

  // Determine if it's a system header
  bool is_system_header = (file_type == clang::SrcMgr::C_System ||
                           file_type == clang::SrcMgr::C_ExternCSystem);

  // Add the file and the include relationship
  dep_graph_->add_file(from_file, false);  // Source file is not a system header
  dep_graph_->add_file(to_file, is_system_header);
  dep_graph_->add_include(from_file, to_file);
}

// ============================================================================
// Call Graph Generation Methods
// ============================================================================

bool ModernASTVisitor::VisitFunctionDecl(clang::FunctionDecl *decl) {
  if (!config_.enable_call_graph) {
    return true;
  }

  // Only process function definitions (not just declarations)
  if (!decl->hasBody()) {
    return true;
  }

  // Skip if in system header
  auto &sm = context_.getSourceManager();
  if (config_.skip_system_headers && sm.isInSystemHeader(decl->getLocation())) {
    return true;
  }

  // Get function name
  std::string function_name = decl->getQualifiedNameAsString();

  // Get source location
  auto loc = decl->getLocation();
  auto presumed = sm.getPresumedLoc(loc);

  std::string file = "<unknown>";
  int line = 0;
  if (presumed.isValid()) {
    file = presumed.getFilename();
    line = presumed.getLine();
  }

  // Check if it's a template or virtual function
  bool is_template = decl->getTemplatedKind() != clang::FunctionDecl::TK_NonTemplate;
  bool is_virtual = false;
  if (auto *method = llvm::dyn_cast<clang::CXXMethodDecl>(decl)) {
    is_virtual = method->isVirtual();
  }

  // Enter function context
  call_graph_builder_.enter_function(function_name, file, line, is_template, is_virtual);
  current_function_name_ = function_name;

  return true;
}

bool ModernASTVisitor::VisitCallExpr(clang::CallExpr *expr) {
  if (!config_.enable_call_graph) {
    return true;
  }

  // Skip if we're not in a function
  if (current_function_name_.empty()) {
    return true;
  }

  // Skip if in system header
  if (isInSystemHeader(expr)) {
    return true;
  }

  // Get the callee
  const clang::FunctionDecl *callee = expr->getDirectCallee();
  if (!callee) {
    return true;  // Can't determine callee (e.g., function pointer)
  }

  // Get callee name
  std::string callee_name = callee->getQualifiedNameAsString();

  // Record the call
  call_graph_builder_.record_call(callee_name);

  return true;
}

// ============================================================================
// Data Flow Analysis Methods
// ============================================================================

bool ModernASTVisitor::VisitVarDecl(clang::VarDecl *decl) {
  if (!config_.enable_data_flow_analysis) {
    return true;
  }

  // Skip if in system header
  auto &sm = context_.getSourceManager();
  if (config_.skip_system_headers && sm.isInSystemHeader(decl->getLocation())) {
    return true;
  }

  // Add variable to data flow analysis
  data_flow_analysis_.add_variable(decl, current_function_name_);

  return true;
}

bool ModernASTVisitor::VisitDeclRefExpr(clang::DeclRefExpr *expr) {
  if (!config_.enable_data_flow_analysis) {
    return true;
  }

  // Skip if in system header
  if (isInSystemHeader(expr)) {
    return true;
  }

  // Check if this is a variable reference
  const clang::ValueDecl* decl = expr->getDecl();
  if (!decl || !clang::isa<clang::VarDecl>(decl)) {
    return true;
  }

  // Determine if this is a read or write
  // We need to check the parent context to see if this is an lvalue used for assignment
  bool is_write = false;
  auto parents = context_.getParents(*expr);

  for (const auto &parent_node : parents) {
    // Check if parent is a binary operator with assignment
    if (const auto *binop = parent_node.get<clang::BinaryOperator>()) {
      // If this expr is the LHS of an assignment, it's a write
      if (binop->isAssignmentOp() && binop->getLHS() == expr) {
        is_write = true;
        // Also record the definition
        data_flow_analysis_.record_variable_definition(expr, current_function_name_);
        break;
      }
    }
    // Check if parent is a unary operator (++, --, &, etc.)
    if (const auto *unaryop = parent_node.get<clang::UnaryOperator>()) {
      // ++, --, or & (address-of) can be considered writes/uses
      if (unaryop->isIncrementDecrementOp()) {
        is_write = true;
        data_flow_analysis_.record_variable_definition(expr, current_function_name_);
      }
      // Address-of is a read (we're reading the address)
      // So we don't break here, let it be recorded as a use below
    }
  }

  // Record the use
  bool is_read = !is_write; // If not a write, it's a read
  data_flow_analysis_.record_variable_use(expr, current_function_name_, is_read);

  return true;
}

optiweave::analysis::DataFlowAnalysis* TransformationConsumer::getDataFlowAnalysis() {
  if (!visitor_) {
    return nullptr;
  }
  return &visitor_->getDataFlowAnalysis();
}

// ============================================================================
// Memory Profiling Methods
// ============================================================================

bool ModernASTVisitor::VisitCXXNewExpr(clang::CXXNewExpr *expr) {
  if (!config_.enable_memory_profiling) {
    return true;
  }

  // Skip if in system header
  if (isInSystemHeader(expr)) {
    return true;
  }

  // Create allocation site
  optiweave::analysis::AllocationSite site;

  // Get source location
  auto &sm = context_.getSourceManager();
  auto loc = expr->getBeginLoc();
  auto presumed = sm.getPresumedLoc(loc);

  if (presumed.isValid()) {
    site.file = presumed.getFilename();
    site.line = presumed.getLine();
  }

  site.function = current_function_name_;
  site.is_array = expr->isArray();
  site.allocation_type = site.is_array ? "new[]" : "new";

  // Get allocated type
  if (expr->getAllocatedType().getTypePtrOrNull()) {
    site.element_type = expr->getAllocatedType().getAsString();
  }

  // Add to memory profiler
  memory_profiler_.add_allocation(site);

  return true;
}

bool ModernASTVisitor::VisitCXXDeleteExpr(clang::CXXDeleteExpr *expr) {
  if (!config_.enable_memory_profiling) {
    return true;
  }

  // Skip if in system header
  if (isInSystemHeader(expr)) {
    return true;
  }

  // Create deallocation site
  optiweave::analysis::DeallocationSite site;

  // Get source location
  auto &sm = context_.getSourceManager();
  auto loc = expr->getBeginLoc();
  auto presumed = sm.getPresumedLoc(loc);

  if (presumed.isValid()) {
    site.file = presumed.getFilename();
    site.line = presumed.getLine();
  }

  site.function = current_function_name_;
  site.is_array = expr->isArrayForm();
  site.deallocation_type = site.is_array ? "delete[]" : "delete";

  // Add to memory profiler
  memory_profiler_.add_deallocation(site);

  return true;
}

optiweave::analysis::MemoryProfiler* TransformationConsumer::getMemoryProfiler() {
  if (!visitor_) {
    return nullptr;
  }
  return &visitor_->getMemoryProfiler();
}

} // namespace optiweave::core
