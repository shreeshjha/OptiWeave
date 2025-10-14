#include "optiweave/analysis/fp_precision_detector.hpp"

#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <sstream>

namespace optiweave {
namespace analysis {

FPPrecisionDetector::FPPrecisionDetector(clang::ASTContext& context)
    : context_(context) {}

// ============================================================================
// Type checking helpers
// ============================================================================

bool FPPrecisionDetector::is_floating_point_type(clang::QualType type) const {
    return type->isFloatingType();
}

bool FPPrecisionDetector::is_float_type(clang::QualType type) const {
    const auto* builtin = type->getAs<clang::BuiltinType>();
    return builtin && builtin->getKind() == clang::BuiltinType::Float;
}

bool FPPrecisionDetector::is_double_type(clang::QualType type) const {
    const auto* builtin = type->getAs<clang::BuiltinType>();
    return builtin && builtin->getKind() == clang::BuiltinType::Double;
}

bool FPPrecisionDetector::is_long_double_type(clang::QualType type) const {
    const auto* builtin = type->getAs<clang::BuiltinType>();
    return builtin && builtin->getKind() == clang::BuiltinType::LongDouble;
}

// ============================================================================
// Pattern detection helpers
// ============================================================================

bool FPPrecisionDetector::is_equality_comparison(clang::BinaryOperator* op) const {
    return op->getOpcode() == clang::BO_EQ || op->getOpcode() == clang::BO_NE;
}

bool FPPrecisionDetector::is_zero_literal(clang::Expr* expr) const {
    if (!expr) return false;

    // Strip implicit casts
    expr = expr->IgnoreImpCasts();

    // Check for floating-point literal zero
    if (auto* fl = llvm::dyn_cast<clang::FloatingLiteral>(expr)) {
        return fl->getValue().isZero();
    }

    // Check for integer literal zero (implicitly converted)
    if (auto* il = llvm::dyn_cast<clang::IntegerLiteral>(expr)) {
        return il->getValue() == 0;
    }

    return false;
}

bool FPPrecisionDetector::could_be_catastrophic_cancellation(clang::BinaryOperator* op) const {
    // Catastrophic cancellation occurs when subtracting nearly equal values
    // We can't determine this statically in general, but we can flag suspicious patterns:
    // 1. Subtracting variables with the same name prefix (e.g., a1 - a2)
    // 2. Subtracting expressions that are syntactically similar

    if (op->getOpcode() != clang::BO_Sub && op->getOpcode() != clang::BO_SubAssign) {
        return false;
    }

    // For now, flag all floating-point subtractions as potential issues
    // More sophisticated analysis would require value range analysis
    return true;
}

bool FPPrecisionDetector::is_accumulation_pattern(clang::CompoundAssignOperator* op) const {
    // Check if this is a += or -= operation on a floating-point variable
    // Common pattern: sum += value in a loop
    return (op->getOpcode() == clang::BO_AddAssign ||
            op->getOpcode() == clang::BO_SubAssign) &&
           is_floating_point_type(op->getType());
}

bool FPPrecisionDetector::has_magnitude_disparity(clang::BinaryOperator* op) const {
    // We can't determine actual magnitudes statically, but we can detect patterns like:
    // - Adding/subtracting double and float
    // - Operations involving constants with very different magnitudes

    // For now, return false - this would require constant propagation analysis
    return false;
}

// ============================================================================
// Source location helpers
// ============================================================================

std::string FPPrecisionDetector::get_filename(clang::SourceLocation loc) const {
    if (loc.isInvalid()) return "<unknown>";

    const clang::SourceManager& sm = context_.getSourceManager();
    clang::PresumedLoc ploc = sm.getPresumedLoc(loc);
    if (ploc.isInvalid()) return "<unknown>";

    return ploc.getFilename();
}

unsigned int FPPrecisionDetector::get_line(clang::SourceLocation loc) const {
    if (loc.isInvalid()) return 0;

    const clang::SourceManager& sm = context_.getSourceManager();
    return sm.getSpellingLineNumber(loc);
}

unsigned int FPPrecisionDetector::get_column(clang::SourceLocation loc) const {
    if (loc.isInvalid()) return 0;

    const clang::SourceManager& sm = context_.getSourceManager();
    return sm.getSpellingColumnNumber(loc);
}

std::string FPPrecisionDetector::get_function_name(clang::Expr* expr) const {
    if (!expr) return "<unknown>";

    // Walk up the AST to find the enclosing function
    auto parents = context_.getParents(*expr);
    while (!parents.empty()) {
        const clang::FunctionDecl* func = parents[0].get<clang::FunctionDecl>();
        if (func) {
            return func->getNameAsString();
        }
        parents = context_.getParents(parents[0]);
    }

    return "<unknown>";
}

std::string FPPrecisionDetector::get_expression_text(clang::Expr* expr) const {
    if (!expr) return "";

    const clang::SourceManager& sm = context_.getSourceManager();
    clang::SourceRange range = expr->getSourceRange();

    if (range.isInvalid()) return "";

    clang::CharSourceRange char_range = clang::CharSourceRange::getTokenRange(range);
    return clang::Lexer::getSourceText(char_range, sm, context_.getLangOpts()).str();
}

std::string FPPrecisionDetector::get_type_name(clang::QualType type) const {
    return type.getAsString();
}

// ============================================================================
// Issue creation and management
// ============================================================================

FPPrecisionIssue FPPrecisionDetector::create_issue(
    FPPrecisionType type,
    FPPrecisionSeverity severity,
    clang::Expr* expr,
    const std::string& description,
    const std::string& suggestion
) const {
    FPPrecisionIssue issue;
    issue.type = type;
    issue.severity = severity;
    issue.file = get_filename(expr->getBeginLoc());
    issue.line = get_line(expr->getBeginLoc());
    issue.column = get_column(expr->getBeginLoc());
    issue.function_name = get_function_name(expr);
    issue.description = description;
    issue.suggestion = suggestion;
    issue.expression_text = get_expression_text(expr);

    return issue;
}

void FPPrecisionDetector::add_issue(const FPPrecisionIssue& issue) {
    issues_.push_back(issue);
}

// ============================================================================
// AST Visitor methods
// ============================================================================

bool FPPrecisionDetector::VisitBinaryOperator(clang::BinaryOperator* op) {
    if (!op) return true;

    clang::QualType lhs_type = op->getLHS()->getType();
    clang::QualType rhs_type = op->getRHS()->getType();
    clang::QualType result_type = op->getType();

    // Check for floating-point equality comparisons
    if (is_equality_comparison(op) && is_floating_point_type(result_type)) {
        if (is_floating_point_type(lhs_type) || is_floating_point_type(rhs_type)) {
            // Check if comparing with zero
            bool is_zero_comp = is_zero_literal(op->getLHS()) || is_zero_literal(op->getRHS());

            auto issue = create_issue(
                is_zero_comp ? FPPrecisionType::EXACT_ZERO_COMPARISON
                             : FPPrecisionType::FP_EQUALITY_COMPARISON,
                FPPrecisionSeverity::WARNING,
                op,
                is_zero_comp
                    ? "Direct comparison of floating-point value with zero"
                    : "Direct equality comparison of floating-point values",
                is_zero_comp
                    ? "Use epsilon comparison: std::abs(value) < epsilon"
                    : "Use epsilon comparison: std::abs(a - b) < epsilon"
            );
            issue.left_operand_type = get_type_name(lhs_type);
            issue.right_operand_type = get_type_name(rhs_type);
            add_issue(issue);
        }
    }

    // Check for catastrophic cancellation (subtraction of floating-point values)
    if ((op->getOpcode() == clang::BO_Sub) &&
        is_floating_point_type(lhs_type) &&
        is_floating_point_type(rhs_type)) {

        auto issue = create_issue(
            FPPrecisionType::CATASTROPHIC_CANCELLATION,
            FPPrecisionSeverity::INFO,
            op,
            "Subtraction of floating-point values may lose precision if operands are nearly equal",
            "Consider restructuring computation to avoid subtracting nearly equal values"
        );
        issue.left_operand_type = get_type_name(lhs_type);
        issue.right_operand_type = get_type_name(rhs_type);
        add_issue(issue);
    }

    // Check for mixed precision arithmetic (double + float)
    if (op->isAdditiveOp() || op->isMultiplicativeOp()) {
        bool lhs_is_float = is_float_type(lhs_type);
        bool rhs_is_float = is_float_type(rhs_type);
        bool lhs_is_double = is_double_type(lhs_type);
        bool rhs_is_double = is_double_type(rhs_type);

        if ((lhs_is_float && rhs_is_double) || (lhs_is_double && rhs_is_float)) {
            auto issue = create_issue(
                FPPrecisionType::MIXED_PRECISION_ARITHMETIC,
                FPPrecisionSeverity::INFO,
                op,
                "Mixed precision arithmetic between float and double",
                "Consider using consistent precision (all float or all double)"
            );
            issue.left_operand_type = get_type_name(lhs_type);
            issue.right_operand_type = get_type_name(rhs_type);
            add_issue(issue);
        }
    }

    // Check for division by potentially small values
    if (op->getOpcode() == clang::BO_Div && is_floating_point_type(result_type)) {
        // We can't determine the actual value statically, but we flag divisions
        // involving variables that might be small
        auto issue = create_issue(
            FPPrecisionType::DIVISION_BY_SMALL_VALUE,
            FPPrecisionSeverity::INFO,
            op,
            "Division by floating-point value - ensure divisor is not too small",
            "Add bounds checking or epsilon comparison before division"
        );
        issue.left_operand_type = get_type_name(lhs_type);
        issue.right_operand_type = get_type_name(rhs_type);
        add_issue(issue);
    }

    // Check for multiplication that could overflow
    if (op->getOpcode() == clang::BO_Mul && is_floating_point_type(result_type)) {
        auto issue = create_issue(
            FPPrecisionType::FP_MULTIPLICATION_OVERFLOW,
            FPPrecisionSeverity::INFO,
            op,
            "Floating-point multiplication - result may overflow to infinity",
            "Check operand magnitudes or use std::isfinite() after computation"
        );
        issue.left_operand_type = get_type_name(lhs_type);
        issue.right_operand_type = get_type_name(rhs_type);
        add_issue(issue);
    }

    return true;
}

bool FPPrecisionDetector::VisitCastExpr(clang::CastExpr* cast) {
    if (!cast) return true;

    clang::QualType src_type = cast->getSubExpr()->getType();
    clang::QualType dst_type = cast->getType();

    // Check for double to float conversion
    if (is_double_type(src_type) && is_float_type(dst_type)) {
        auto issue = create_issue(
            FPPrecisionType::DOUBLE_TO_FLOAT_CONVERSION,
            FPPrecisionSeverity::WARNING,
            cast,
            "Converting from double to float loses precision",
            "Use double throughout if precision is important, or explicitly document precision loss"
        );
        issue.left_operand_type = get_type_name(src_type);
        issue.right_operand_type = get_type_name(dst_type);
        add_issue(issue);
    }

    // Check for long double to double/float conversion
    if (is_long_double_type(src_type) &&
        (is_double_type(dst_type) || is_float_type(dst_type))) {
        auto issue = create_issue(
            FPPrecisionType::LONG_DOUBLE_CONVERSION,
            FPPrecisionSeverity::WARNING,
            cast,
            "Converting from long double to smaller floating-point type loses precision",
            "Use long double throughout if extended precision is needed"
        );
        issue.left_operand_type = get_type_name(src_type);
        issue.right_operand_type = get_type_name(dst_type);
        add_issue(issue);
    }

    return true;
}

bool FPPrecisionDetector::VisitImplicitCastExpr(clang::ImplicitCastExpr* cast) {
    // Delegate to VisitCastExpr
    return VisitCastExpr(cast);
}

bool FPPrecisionDetector::VisitCompoundAssignOperator(clang::CompoundAssignOperator* op) {
    if (!op) return true;

    // Check for accumulation pattern (sum += value)
    if (is_accumulation_pattern(op)) {
        auto issue = create_issue(
            FPPrecisionType::ACCUMULATION_WITHOUT_COMPENSATION,
            FPPrecisionSeverity::INFO,
            op,
            "Floating-point accumulation without compensation may lose precision",
            "Consider using Kahan summation or other compensated summation algorithms"
        );
        issue.left_operand_type = get_type_name(op->getLHS()->getType());
        issue.right_operand_type = get_type_name(op->getRHS()->getType());
        add_issue(issue);
    }

    return true;
}

bool FPPrecisionDetector::VisitForStmt(clang::ForStmt* for_stmt) {
    // Check for accumulation patterns in for loops
    // This is a simplified check - full analysis would track variables across iterations
    return true;
}

bool FPPrecisionDetector::VisitWhileStmt(clang::WhileStmt* while_stmt) {
    // Check for accumulation patterns in while loops
    return true;
}

// ============================================================================
// Enum to string conversion
// ============================================================================

std::string FPPrecisionDetector::type_to_string(FPPrecisionType type) const {
    switch (type) {
        case FPPrecisionType::FP_EQUALITY_COMPARISON:
            return "fp_equality_comparison";
        case FPPrecisionType::CATASTROPHIC_CANCELLATION:
            return "catastrophic_cancellation";
        case FPPrecisionType::DOUBLE_TO_FLOAT_CONVERSION:
            return "double_to_float_conversion";
        case FPPrecisionType::LONG_DOUBLE_CONVERSION:
            return "long_double_conversion";
        case FPPrecisionType::ACCUMULATION_WITHOUT_COMPENSATION:
            return "accumulation_without_compensation";
        case FPPrecisionType::DIVISION_BY_SMALL_VALUE:
            return "division_by_small_value";
        case FPPrecisionType::FP_MULTIPLICATION_OVERFLOW:
            return "fp_multiplication_overflow";
        case FPPrecisionType::MIXED_PRECISION_ARITHMETIC:
            return "mixed_precision_arithmetic";
        case FPPrecisionType::EXACT_ZERO_COMPARISON:
            return "exact_zero_comparison";
        case FPPrecisionType::MAGNITUDE_DISPARITY:
            return "magnitude_disparity";
    }
    return "unknown";
}

std::string FPPrecisionDetector::severity_to_string(FPPrecisionSeverity severity) const {
    switch (severity) {
        case FPPrecisionSeverity::CRITICAL:
            return "critical";
        case FPPrecisionSeverity::WARNING:
            return "warning";
        case FPPrecisionSeverity::INFO:
            return "info";
    }
    return "unknown";
}

// ============================================================================
// Statistics
// ============================================================================

unsigned int FPPrecisionDetector::count_by_severity(FPPrecisionSeverity severity) const {
    unsigned int count = 0;
    for (const auto& issue : issues_) {
        if (issue.severity == severity) {
            ++count;
        }
    }
    return count;
}

// ============================================================================
// Export functions
// ============================================================================

void FPPrecisionDetector::export_json(const std::string& filename) const {
    std::error_code EC;
    llvm::raw_fd_ostream file(filename, EC);
    if (EC) {
        llvm::errs() << "Error: Could not open file for writing: " << filename << "\n";
        return;
    }

    llvm::json::Object root;
    root["total_issues"] = static_cast<int64_t>(issues_.size());

    llvm::json::Array issues_array;
    for (const auto& issue : issues_) {
        llvm::json::Object issue_obj;
        issue_obj["type"] = type_to_string(issue.type);
        issue_obj["severity"] = severity_to_string(issue.severity);
        issue_obj["file"] = issue.file;
        issue_obj["line"] = static_cast<int64_t>(issue.line);
        issue_obj["column"] = static_cast<int64_t>(issue.column);
        issue_obj["function"] = issue.function_name;
        issue_obj["description"] = issue.description;
        issue_obj["suggestion"] = issue.suggestion;

        if (!issue.expression_text.empty()) {
            issue_obj["expression"] = issue.expression_text;
        }
        if (!issue.left_operand_type.empty()) {
            issue_obj["left_type"] = issue.left_operand_type;
        }
        if (!issue.right_operand_type.empty()) {
            issue_obj["right_type"] = issue.right_operand_type;
        }

        issues_array.push_back(std::move(issue_obj));
    }
    root["issues"] = std::move(issues_array);

    file << llvm::formatv("{0:2}", llvm::json::Value(std::move(root))) << "\n";
    file.flush();
}

void FPPrecisionDetector::export_text(const std::string& filename) const {
    std::error_code EC;
    llvm::raw_fd_ostream file(filename, EC);
    if (EC) {
        llvm::errs() << "Error: Could not open file for writing: " << filename << "\n";
        return;
    }

    print_report(file, 0);
    file.flush();
}

void FPPrecisionDetector::print_report(llvm::raw_ostream& os, unsigned int indent) const {
    std::string indent_str(indent, ' ');

    os << indent_str << "=== Floating-Point Precision Warning Report ===\n";
    os << indent_str << "\n";
    os << indent_str << "Total issues found: " << issues_.size() << "\n";
    os << indent_str << "\n";
    os << indent_str << "Critical: " << count_by_severity(FPPrecisionSeverity::CRITICAL) << "\n";
    os << indent_str << "Warning: " << count_by_severity(FPPrecisionSeverity::WARNING) << "\n";
    os << indent_str << "Info: " << count_by_severity(FPPrecisionSeverity::INFO) << "\n";
    os << indent_str << "\n";

    for (const auto& issue : issues_) {
        os << indent_str << "[" << severity_to_string(issue.severity) << "] "
           << type_to_string(issue.type) << "\n";
        os << indent_str << "  Location: " << issue.file << ":" << issue.line
           << ":" << issue.column << " (in " << issue.function_name << ")\n";
        os << indent_str << "  Description: " << issue.description << "\n";
        os << indent_str << "  Suggestion: " << issue.suggestion << "\n";

        if (!issue.expression_text.empty()) {
            os << indent_str << "  Expression: " << issue.expression_text << "\n";
        }
        if (!issue.left_operand_type.empty() && !issue.right_operand_type.empty()) {
            os << indent_str << "  Types: " << issue.left_operand_type
               << " op " << issue.right_operand_type << "\n";
        }

        os << indent_str << "\n";
    }
}

} // namespace analysis
} // namespace optiweave
