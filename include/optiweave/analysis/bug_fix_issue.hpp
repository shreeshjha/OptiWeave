#pragma once

#include <optiweave/analysis/overflow_detector.hpp>
#include <optiweave/analysis/data_flow_analysis.hpp>
#include <optiweave/analysis/fp_precision_detector.hpp>
#include <clang/Basic/SourceManager.h>
#include <string>
#include <vector>

namespace optiweave {
namespace analysis {

/// Auto-fixable bug categories (13 total)
enum class BugFixKind {
    // Original 6
    UnsignedWraparound,      // a - b  →  ((a) >= (b) ? (a) - (b) : 0)
    SignedNegationOverflow,  // -x  →  ((x) == INT_MIN ? INT_MAX : -(x))
    SignedLeftShift,         // x << n  →  ((unsigned)(x)) << n
    UninitializedVariable,   // int x;  →  int x = 0;
    UnusedVariable,          // insert (void)x;
    FPEqualityComparison,    // a == b  →  fabs((a)-(b)) < 1e-9

    // New 7 (standalone — self-matched by rules, no analyzer prerequisite)
    DivisionByZero,          // a / b  →  (b != 0 ? a / b : 0)
    NullDerefGuard,          // insert null check at function entry
    StringOverflow,          // strcpy → strncpy, sprintf → snprintf
    ImplicitFallthrough,     // insert break; before next case
    SizeofPointer,           // malloc(sizeof(ptr)) → malloc(sizeof(*ptr))
    IntegerTruncation,       // add explicit cast on narrowing
    DanglingElse             // wrap single-statement if/else bodies in braces
};

/// Unified issue consumed by BugFixVisitor
struct BugFixIssue {
    BugFixKind kind;
    std::string file;
    unsigned line = 0;
    unsigned column = 0;
    std::string description;

    // For variable-related fixes
    std::string var_name;
    std::string var_type;

    // For expression-related fixes
    std::string expression_text;

    // For overflow: LHS/RHS type info
    std::string lhs_type;
    std::string rhs_type;
};

/// Convert overflow detector issues to auto-fixable BugFixIssues.
/// Only keeps: UNSIGNED_WRAPAROUND (subtraction), SIGNED_NEGATION, SIGNED_LEFT_SHIFT.
inline std::vector<BugFixIssue> convert_overflow_issues(
    const std::vector<OverflowIssue>& issues) {
    std::vector<BugFixIssue> result;
    for (const auto& oi : issues) {
        BugFixIssue bi;
        bi.file = oi.file;
        bi.line = oi.line;
        bi.column = oi.column;
        bi.description = oi.description;
        bi.expression_text = oi.code_snippet;
        bi.lhs_type = oi.lhs_type;
        bi.rhs_type = oi.rhs_type;

        switch (oi.type) {
        case OverflowType::UNSIGNED_WRAPAROUND:
            // Only fix subtraction wraparounds (operator_str contains "-")
            if (oi.operator_str.find('-') != std::string::npos) {
                bi.kind = BugFixKind::UnsignedWraparound;
                result.push_back(bi);
            }
            break;
        case OverflowType::SIGNED_NEGATION:
            bi.kind = BugFixKind::SignedNegationOverflow;
            result.push_back(bi);
            break;
        case OverflowType::SIGNED_LEFT_SHIFT:
            bi.kind = BugFixKind::SignedLeftShift;
            result.push_back(bi);
            break;
        default:
            break; // Not auto-fixable
        }
    }
    return result;
}

/// Convert data-flow refactoring opportunities to BugFixIssues.
/// Only keeps: UninitializedVariable, UnusedVariable.
inline std::vector<BugFixIssue> convert_dataflow_issues(
    const std::vector<RefactoringOpportunity>& opportunities,
    const std::map<std::string, VariableInfo>& variables,
    clang::SourceManager& sm) {
    std::vector<BugFixIssue> result;
    for (const auto& opp : opportunities) {
        BugFixIssue bi;
        bi.description = opp.description;

        // Resolve SourceLocation to file:line:col
        if (opp.location.isValid()) {
            auto presumed = sm.getPresumedLoc(opp.location);
            if (presumed.isValid()) {
                bi.file = presumed.getFilename();
                bi.line = presumed.getLine();
                bi.column = presumed.getColumn();
            }
        }

        // Extract variable name from description (format: "Variable 'X' ...")
        auto pos = opp.description.find('\'');
        if (pos != std::string::npos) {
            auto end = opp.description.find('\'', pos + 1);
            if (end != std::string::npos) {
                bi.var_name = opp.description.substr(pos + 1, end - pos - 1);
            }
        }

        // Look up variable type from the variables map
        for (const auto& [key, vi] : variables) {
            if (vi.name == bi.var_name &&
                vi.function_context == opp.function_context) {
                bi.var_type = vi.type;
                break;
            }
        }

        switch (opp.type) {
        case RefactoringOpportunity::Type::UninitializedVariable:
            bi.kind = BugFixKind::UninitializedVariable;
            result.push_back(bi);
            break;
        case RefactoringOpportunity::Type::UnusedVariable:
            bi.kind = BugFixKind::UnusedVariable;
            result.push_back(bi);
            break;
        default:
            break;
        }
    }
    return result;
}

/// Convert FP precision issues to BugFixIssues.
/// Only keeps: FP_EQUALITY_COMPARISON.
inline std::vector<BugFixIssue> convert_fp_issues(
    const std::vector<FPPrecisionIssue>& issues) {
    std::vector<BugFixIssue> result;
    for (const auto& fi : issues) {
        if (fi.type != FPPrecisionType::FP_EQUALITY_COMPARISON)
            continue;

        BugFixIssue bi;
        bi.kind = BugFixKind::FPEqualityComparison;
        bi.file = fi.file;
        bi.line = fi.line;
        bi.column = fi.column;
        bi.description = fi.description;
        bi.expression_text = fi.expression_text;
        bi.lhs_type = fi.left_operand_type;
        bi.rhs_type = fi.right_operand_type;
        result.push_back(bi);
    }
    return result;
}

} // namespace analysis
} // namespace optiweave
