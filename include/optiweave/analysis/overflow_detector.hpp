#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace optiweave {
namespace analysis {

/**
 * @brief Types of integer overflow issues
 */
enum class OverflowType {
    SIGNED_ADDITION,
    SIGNED_SUBTRACTION,
    SIGNED_MULTIPLICATION,
    SIGNED_DIVISION,
    SIGNED_NEGATION,
    SIGNED_LEFT_SHIFT,
    UNSIGNED_WRAPAROUND,
    MIXED_SIGNEDNESS,
    NARROWING_CONVERSION,
    LOOP_COUNTER_OVERFLOW
};

/**
 * @brief Severity of overflow issue
 */
enum class OverflowSeverity {
    CRITICAL,  // Undefined behavior (signed overflow)
    WARNING,   // Likely unintended (unsigned wraparound, narrowing)
    INFO       // Potential issue (mixed signedness)
};

/**
 * @brief Information about a detected overflow risk
 */
struct OverflowIssue {
    OverflowType type;
    OverflowSeverity severity;
    std::string file;
    unsigned line;
    unsigned column;
    std::string function;
    std::string description;
    std::string code_snippet;
    std::string suggestion;

    // Operator details
    std::string operator_str;
    std::string lhs_type;
    std::string rhs_type;
    bool is_constant_expr;
    bool is_in_loop;

    OverflowIssue() = default;
};

/**
 * @brief Statistics about overflow detection
 */
struct OverflowStatistics {
    size_t total_issues = 0;
    size_t critical_issues = 0;
    size_t warning_issues = 0;
    size_t info_issues = 0;

    // By type
    size_t signed_overflows = 0;
    size_t unsigned_wraparounds = 0;
    size_t mixed_signedness = 0;
    size_t narrowing_conversions = 0;
    size_t loop_overflows = 0;

    // By location
    std::unordered_map<std::string, size_t> issues_per_file;
    std::unordered_map<std::string, size_t> issues_per_function;
};

/**
 * @brief Detector for integer overflow issues
 *
 * This class performs static analysis to detect potential integer overflow
 * issues in C++ code, including:
 * - Signed integer overflow (undefined behavior)
 * - Unsigned integer wraparound (defined but often unintended)
 * - Mixed signed/unsigned operations
 * - Narrowing conversions
 * - Loop counter overflow risks
 */
class OverflowDetector : public clang::RecursiveASTVisitor<OverflowDetector> {
public:
    explicit OverflowDetector(clang::ASTContext& context);

    // AST Visitor methods
    bool VisitBinaryOperator(clang::BinaryOperator* op);
    bool VisitUnaryOperator(clang::UnaryOperator* op);
    bool VisitCastExpr(clang::CastExpr* cast);
    bool VisitForStmt(clang::ForStmt* for_stmt);
    bool VisitWhileStmt(clang::WhileStmt* while_stmt);

    /**
     * @brief Get all detected overflow issues
     */
    const std::vector<OverflowIssue>& get_issues() const { return issues_; }

    /**
     * @brief Get detection statistics
     */
    const OverflowStatistics& get_statistics() const { return stats_; }

    /**
     * @brief Print overflow issues to stream
     */
    void print_report(llvm::raw_ostream& out, size_t max_issues = 0) const;

    /**
     * @brief Export issues to JSON
     */
    void export_json(const std::string& filename) const;

    /**
     * @brief Export issues to text file
     */
    void export_text(const std::string& filename) const;

    /**
     * @brief Get issues by severity
     */
    std::vector<OverflowIssue> get_issues_by_severity(OverflowSeverity severity) const;

    /**
     * @brief Get issues by file
     */
    std::vector<OverflowIssue> get_issues_by_file(const std::string& file) const;

private:
    clang::ASTContext& context_;
    clang::SourceManager& source_manager_;
    std::vector<OverflowIssue> issues_;
    OverflowStatistics stats_;

    // Helper methods
    bool is_signed_integer_type(clang::QualType type);
    bool is_unsigned_integer_type(clang::QualType type);
    bool could_overflow_add(clang::BinaryOperator* op);
    bool could_overflow_sub(clang::BinaryOperator* op);
    bool could_overflow_mul(clang::BinaryOperator* op);
    bool could_overflow_shift(clang::BinaryOperator* op);
    bool is_in_loop_context(const clang::Stmt* stmt);
    bool is_constant_expression(const clang::Expr* expr);
    std::string get_source_text(const clang::Stmt* stmt);
    std::string get_type_name(clang::QualType type);
    std::string get_function_name(const clang::Stmt* stmt);

    void add_issue(const OverflowIssue& issue);
    OverflowIssue create_issue(OverflowType type, OverflowSeverity severity,
                               const clang::Stmt* stmt, const std::string& description,
                               const std::string& suggestion);
};

/**
 * @brief Get string representation of overflow type
 */
const char* overflow_type_to_string(OverflowType type);

/**
 * @brief Get string representation of severity
 */
const char* severity_to_string(OverflowSeverity severity);

} // namespace analysis
} // namespace optiweave
