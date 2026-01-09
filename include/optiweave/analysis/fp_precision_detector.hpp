#ifndef OPTIWEAVE_ANALYSIS_FP_PRECISION_DETECTOR_HPP
#define OPTIWEAVE_ANALYSIS_FP_PRECISION_DETECTOR_HPP

#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Type.h>
#include <clang/Basic/SourceLocation.h>
#include <llvm/Support/raw_ostream.h>

#include <string>
#include <vector>
#include <memory>

namespace optiweave {
namespace analysis {

/**
 * Types of floating-point precision issues that can be detected
 */
enum class FPPrecisionType {
    // Direct equality/inequality comparison of floating-point values
    FP_EQUALITY_COMPARISON,

    // Subtracting nearly equal floating-point values (catastrophic cancellation)
    CATASTROPHIC_CANCELLATION,

    // Converting from double to float (precision loss)
    DOUBLE_TO_FLOAT_CONVERSION,

    // Converting from long double to double or float
    LONG_DOUBLE_CONVERSION,

    // Accumulation in loops without compensation (Kahan summation)
    ACCUMULATION_WITHOUT_COMPENSATION,

    // Division by very small floating-point value
    DIVISION_BY_SMALL_VALUE,

    // Multiplication overflow (result becomes infinity)
    FP_MULTIPLICATION_OVERFLOW,

    // Mixed precision arithmetic (double + float)
    MIXED_PRECISION_ARITHMETIC,

    // Comparison with exact zero when epsilon comparison would be better
    EXACT_ZERO_COMPARISON,

    // Loss of significance in addition/subtraction of vastly different magnitudes
    MAGNITUDE_DISPARITY
};

/**
 * Severity levels for floating-point precision issues
 */
enum class FPPrecisionSeverity {
    CRITICAL,  // Highly likely to cause incorrect results
    WARNING,   // May cause precision issues in certain scenarios
    INFO       // Best practice violation or potential issue
};

/**
 * Represents a detected floating-point precision issue
 */
struct FPPrecisionIssue {
    FPPrecisionType type;
    FPPrecisionSeverity severity;

    std::string file;
    unsigned int line;
    unsigned int column;
    std::string function_name;

    std::string description;
    std::string suggestion;

    // Additional context information
    std::string left_operand_type;
    std::string right_operand_type;
    std::string expression_text;
};

/**
 * AST visitor for detecting floating-point precision issues
 */
class FPPrecisionDetector : public clang::RecursiveASTVisitor<FPPrecisionDetector> {
public:
    explicit FPPrecisionDetector(clang::ASTContext& context);

    // Visit binary operators (comparisons, arithmetic)
    bool VisitBinaryOperator(clang::BinaryOperator* op);

    // Visit cast expressions (type conversions)
    bool VisitCastExpr(clang::CastExpr* cast);

    // Visit implicit cast expressions
    bool VisitImplicitCastExpr(clang::ImplicitCastExpr* cast);

    // Visit for loops (accumulation patterns)
    bool VisitForStmt(clang::ForStmt* for_stmt);

    // Visit while loops
    bool VisitWhileStmt(clang::WhileStmt* while_stmt);

    // Visit compound assignment operators (+=, -=, etc.)
    bool VisitCompoundAssignOperator(clang::CompoundAssignOperator* op);

    // Get all detected issues
    const std::vector<FPPrecisionIssue>& get_issues() const { return issues_; }

    // Export results
    void export_json(const std::string& filename) const;
    void export_text(const std::string& filename) const;

    // Print report to stream
    void print_report(llvm::raw_ostream& os, unsigned int indent = 0) const;

private:
    clang::ASTContext& context_;
    std::vector<FPPrecisionIssue> issues_;

    // Helper functions for type checking
    bool is_floating_point_type(clang::QualType type) const;
    bool is_float_type(clang::QualType type) const;
    bool is_double_type(clang::QualType type) const;
    bool is_long_double_type(clang::QualType type) const;

    // Helper functions for detecting specific patterns
    bool is_equality_comparison(clang::BinaryOperator* op) const;
    bool is_zero_literal(clang::Expr* expr) const;
    bool could_be_catastrophic_cancellation(clang::BinaryOperator* op) const;
    bool is_accumulation_pattern(clang::CompoundAssignOperator* op) const;
    bool has_magnitude_disparity(clang::BinaryOperator* op) const;

    // Helper functions for issue creation
    FPPrecisionIssue create_issue(
        FPPrecisionType type,
        FPPrecisionSeverity severity,
        clang::Expr* expr,
        const std::string& description,
        const std::string& suggestion
    ) const;

    void add_issue(const FPPrecisionIssue& issue);

    // Helper to get source location info
    std::string get_filename(clang::SourceLocation loc) const;
    unsigned int get_line(clang::SourceLocation loc) const;
    unsigned int get_column(clang::SourceLocation loc) const;
    std::string get_function_name(clang::Expr* expr) const;
    std::string get_expression_text(clang::Expr* expr) const;
    std::string get_type_name(clang::QualType type) const;

    // Convert enums to strings
    std::string type_to_string(FPPrecisionType type) const;
    std::string severity_to_string(FPPrecisionSeverity severity) const;

    // Statistics
    unsigned int count_by_severity(FPPrecisionSeverity severity) const;
};

} // namespace analysis
} // namespace optiweave

#endif // OPTIWEAVE_ANALYSIS_FP_PRECISION_DETECTOR_HPP
