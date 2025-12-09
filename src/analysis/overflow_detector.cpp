#include <optiweave/analysis/overflow_detector.hpp>

#include <clang/AST/ASTContext.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <sstream>

namespace optiweave {
namespace analysis {

OverflowDetector::OverflowDetector(clang::ASTContext& context)
    : context_(context), source_manager_(context.getSourceManager()) {}

bool OverflowDetector::VisitBinaryOperator(clang::BinaryOperator* op) {
    if (!op || source_manager_.isInSystemHeader(op->getBeginLoc())) {
        return true;
    }

    clang::QualType result_type = op->getType();
    clang::QualType lhs_type = op->getLHS()->getType();
    clang::QualType rhs_type = op->getRHS()->getType();

    // Check for mixed signedness
    if (op->isAdditiveOp() || op->isMultiplicativeOp() || op->isRelationalOp()) {
        if (is_signed_integer_type(lhs_type) && is_unsigned_integer_type(rhs_type)) {
            auto issue = create_issue(
                OverflowType::MIXED_SIGNEDNESS,
                OverflowSeverity::INFO,
                op,
                "Mixed signed and unsigned integer operation",
                "Ensure both operands have the same signedness, or cast explicitly"
            );
            issue.operator_str = op->getOpcodeStr().str();
            issue.lhs_type = get_type_name(lhs_type);
            issue.rhs_type = get_type_name(rhs_type);
            add_issue(issue);
        }
    }

    // Check for signed integer overflow
    if (is_signed_integer_type(result_type)) {
        bool could_overflow = false;
        std::string desc, suggestion;

        switch (op->getOpcode()) {
            case clang::BO_Add:
            case clang::BO_AddAssign:
                if (could_overflow_add(op)) {
                    could_overflow = true;
                    desc = "Potential signed integer overflow in addition";
                    suggestion = "Use safer arithmetic (e.g., check before addition, use wider type, or use unsigned)";
                }
                break;

            case clang::BO_Sub:
            case clang::BO_SubAssign:
                if (could_overflow_sub(op)) {
                    could_overflow = true;
                    desc = "Potential signed integer overflow in subtraction";
                    suggestion = "Check operand values before subtraction or use a wider integer type";
                }
                break;

            case clang::BO_Mul:
            case clang::BO_MulAssign:
                if (could_overflow_mul(op)) {
                    could_overflow = true;
                    desc = "Potential signed integer overflow in multiplication";
                    suggestion = "Check operand magnitudes or use a wider type (e.g., int64_t)";
                }
                break;

            case clang::BO_Shl:
            case clang::BO_ShlAssign:
                if (could_overflow_shift(op)) {
                    could_overflow = true;
                    desc = "Potential undefined behavior: left shift of signed integer";
                    suggestion = "Use unsigned integers for bit manipulation, or ensure shift amount is valid";
                }
                break;

            default:
                break;
        }

        if (could_overflow) {
            auto issue = create_issue(
                OverflowType::SIGNED_ADDITION,
                OverflowSeverity::CRITICAL,
                op,
                desc,
                suggestion
            );
            issue.operator_str = op->getOpcodeStr().str();
            issue.lhs_type = get_type_name(lhs_type);
            issue.rhs_type = get_type_name(rhs_type);
            issue.is_constant_expr = is_constant_expression(op);
            issue.is_in_loop = is_in_loop_context(op);
            add_issue(issue);
        }
    }

    // Check for unsigned wraparound (subtraction from zero is common mistake)
    if (is_unsigned_integer_type(result_type)) {
        if (op->getOpcode() == clang::BO_Sub || op->getOpcode() == clang::BO_SubAssign) {
            // Check if LHS could be less than RHS
            auto issue = create_issue(
                OverflowType::UNSIGNED_WRAPAROUND,
                OverflowSeverity::WARNING,
                op,
                "Unsigned integer subtraction may wrap around",
                "Ensure LHS >= RHS before subtraction, or use signed integers if negative results are expected"
            );
            issue.operator_str = op->getOpcodeStr().str();
            issue.lhs_type = get_type_name(lhs_type);
            issue.rhs_type = get_type_name(rhs_type);
            add_issue(issue);
        }
    }

    return true;
}

bool OverflowDetector::VisitUnaryOperator(clang::UnaryOperator* op) {
    if (!op || source_manager_.isInSystemHeader(op->getBeginLoc())) {
        return true;
    }

    // Check for negation of INT_MIN
    if (op->getOpcode() == clang::UO_Minus) {
        clang::QualType type = op->getSubExpr()->getType();
        if (is_signed_integer_type(type)) {
            auto issue = create_issue(
                OverflowType::SIGNED_NEGATION,
                OverflowSeverity::CRITICAL,
                op,
                "Negation of signed integer may cause overflow (e.g., -INT_MIN)",
                "Check if value is INT_MIN before negating, or use unsigned type"
            );
            issue.operator_str = "-";
            issue.lhs_type = get_type_name(type);
            add_issue(issue);
        }
    }

    return true;
}

bool OverflowDetector::VisitCastExpr(clang::CastExpr* cast) {
    if (!cast || source_manager_.isInSystemHeader(cast->getBeginLoc())) {
        return true;
    }

    // Check for narrowing conversions
    clang::QualType src_type = cast->getSubExpr()->getType();
    clang::QualType dst_type = cast->getType();

    if (src_type->isIntegerType() && dst_type->isIntegerType()) {
        unsigned src_width = context_.getIntWidth(src_type);
        unsigned dst_width = context_.getIntWidth(dst_type);

        if (src_width > dst_width) {
            auto issue = create_issue(
                OverflowType::NARROWING_CONVERSION,
                OverflowSeverity::WARNING,
                cast,
                "Narrowing conversion from " + get_type_name(src_type) +
                " to " + get_type_name(dst_type) + " may lose data",
                "Check value range before conversion or use explicit bounds checking"
            );
            issue.lhs_type = get_type_name(src_type);
            issue.rhs_type = get_type_name(dst_type);
            add_issue(issue);
        }
    }

    return true;
}

bool OverflowDetector::VisitForStmt(clang::ForStmt* for_stmt) {
    if (!for_stmt || source_manager_.isInSystemHeader(for_stmt->getBeginLoc())) {
        return true;
    }

    // Check loop condition for potential overflow
    clang::Expr* cond = for_stmt->getCond();
    if (!cond) {
        return true;
    }

    // Look for patterns like: for (int i = large_value; i < even_larger_value; i++)
    // This is a simplified check - a full implementation would need data flow analysis
    if (auto* bin_op = clang::dyn_cast<clang::BinaryOperator>(cond)) {
        if (bin_op->isRelationalOp()) {
            clang::QualType type = bin_op->getLHS()->getType();
            if (is_signed_integer_type(type)) {
                auto issue = create_issue(
                    OverflowType::LOOP_COUNTER_OVERFLOW,
                    OverflowSeverity::WARNING,
                    for_stmt,
                    "Loop counter may overflow causing infinite loop",
                    "Ensure loop bounds don't cause counter overflow, use wider type, or check explicitly"
                );
                issue.is_in_loop = true;
                add_issue(issue);
            }
        }
    }

    return true;
}

bool OverflowDetector::VisitWhileStmt(clang::WhileStmt* while_stmt) {
    // Similar checks for while loops
    return true;
}

// Helper methods

bool OverflowDetector::is_signed_integer_type(clang::QualType type) {
    return type->isSignedIntegerType();
}

bool OverflowDetector::is_unsigned_integer_type(clang::QualType type) {
    return type->isUnsignedIntegerType();
}

bool OverflowDetector::could_overflow_add(clang::BinaryOperator* op) {
    // Conservative: assume any non-constant addition could overflow
    // A more sophisticated implementation would use value range analysis
    return !is_constant_expression(op);
}

bool OverflowDetector::could_overflow_sub(clang::BinaryOperator* op) {
    return !is_constant_expression(op);
}

bool OverflowDetector::could_overflow_mul(clang::BinaryOperator* op) {
    return !is_constant_expression(op);
}

bool OverflowDetector::could_overflow_shift(clang::BinaryOperator* op) {
    // Left shift of signed integer is undefined if result doesn't fit
    return true;
}

bool OverflowDetector::is_in_loop_context(const clang::Stmt* stmt) {
    // Walk up the AST to see if we're inside a loop
    auto& parent_map = context_.getParentMapContext();

    auto parents = parent_map.getParents(*stmt);
    while (!parents.empty()) {
        const clang::Stmt* parent_stmt = parents[0].get<clang::Stmt>();
        if (!parent_stmt) {
            auto parent_node = parents[0];
            parents = parent_map.getParents(parent_node);
            continue;
        }

        if (clang::isa<clang::ForStmt>(parent_stmt) ||
            clang::isa<clang::WhileStmt>(parent_stmt) ||
            clang::isa<clang::DoStmt>(parent_stmt)) {
            return true;
        }

        parents = parent_map.getParents(*parent_stmt);
    }

    return false;
}

bool OverflowDetector::is_constant_expression(const clang::Expr* expr) {
    if (!expr) {
        return false;
    }
    return expr->isConstantInitializer(context_, false);
}

std::string OverflowDetector::get_source_text(const clang::Stmt* stmt) {
    if (!stmt) {
        return "";
    }

    clang::SourceLocation begin = stmt->getBeginLoc();
    clang::SourceLocation end = stmt->getEndLoc();

    if (begin.isInvalid() || end.isInvalid()) {
        return "";
    }

    // Get the source range
    clang::SourceRange range(begin, end);
    clang::CharSourceRange char_range = clang::CharSourceRange::getTokenRange(range);

    // Extract the text
    bool invalid = false;
    llvm::StringRef text = clang::Lexer::getSourceText(char_range, source_manager_, context_.getLangOpts(), &invalid);

    if (invalid) {
        return "";
    }

    return text.str();
}

std::string OverflowDetector::get_type_name(clang::QualType type) {
    return type.getAsString();
}

std::string OverflowDetector::get_function_name(const clang::Stmt* stmt) {
    // Walk up to find enclosing function
    auto& parent_map = context_.getParentMapContext();
    auto parents = parent_map.getParents(*stmt);

    while (!parents.empty()) {
        if (const clang::FunctionDecl* func = parents[0].get<clang::FunctionDecl>()) {
            return func->getNameAsString();
        }

        auto parent_node = parents[0];
        parents = parent_map.getParents(parent_node);
    }

    return "<unknown>";
}

void OverflowDetector::add_issue(const OverflowIssue& issue) {
    issues_.push_back(issue);
    stats_.total_issues++;

    switch (issue.severity) {
        case OverflowSeverity::CRITICAL:
            stats_.critical_issues++;
            break;
        case OverflowSeverity::WARNING:
            stats_.warning_issues++;
            break;
        case OverflowSeverity::INFO:
            stats_.info_issues++;
            break;
    }

    switch (issue.type) {
        case OverflowType::SIGNED_ADDITION:
        case OverflowType::SIGNED_SUBTRACTION:
        case OverflowType::SIGNED_MULTIPLICATION:
        case OverflowType::SIGNED_NEGATION:
        case OverflowType::SIGNED_LEFT_SHIFT:
            stats_.signed_overflows++;
            break;
        case OverflowType::UNSIGNED_WRAPAROUND:
            stats_.unsigned_wraparounds++;
            break;
        case OverflowType::MIXED_SIGNEDNESS:
            stats_.mixed_signedness++;
            break;
        case OverflowType::NARROWING_CONVERSION:
            stats_.narrowing_conversions++;
            break;
        case OverflowType::LOOP_COUNTER_OVERFLOW:
            stats_.loop_overflows++;
            break;
    }

    stats_.issues_per_file[issue.file]++;
    stats_.issues_per_function[issue.function]++;
}

OverflowIssue OverflowDetector::create_issue(
    OverflowType type,
    OverflowSeverity severity,
    const clang::Stmt* stmt,
    const std::string& description,
    const std::string& suggestion) {

    OverflowIssue issue;
    issue.type = type;
    issue.severity = severity;
    issue.description = description;
    issue.suggestion = suggestion;

    if (stmt) {
        clang::SourceLocation loc = stmt->getBeginLoc();
        if (loc.isValid()) {
            clang::PresumedLoc presumed_loc = source_manager_.getPresumedLoc(loc);
            if (presumed_loc.isValid()) {
                issue.file = presumed_loc.getFilename();
                issue.line = presumed_loc.getLine();
                issue.column = presumed_loc.getColumn();
            }
        }

        issue.code_snippet = get_source_text(stmt);
        issue.function = get_function_name(stmt);
    }

    return issue;
}

void OverflowDetector::print_report(llvm::raw_ostream& out, size_t max_issues) const {
    out << "\n╔══════════════════════════════════════════════════════════════╗\n";
    out << "║       OptiWeave Integer Overflow Detection Report           ║\n";
    out << "╚══════════════════════════════════════════════════════════════╝\n\n";

    out << "Total Issues: " << stats_.total_issues << "\n";
    out << "  Critical: " << stats_.critical_issues << " (undefined behavior)\n";
    out << "  Warnings: " << stats_.warning_issues << "\n";
    out << "  Info: " << stats_.info_issues << "\n\n";

    out << "By Type:\n";
    out << "  Signed overflows: " << stats_.signed_overflows << "\n";
    out << "  Unsigned wraparounds: " << stats_.unsigned_wraparounds << "\n";
    out << "  Mixed signedness: " << stats_.mixed_signedness << "\n";
    out << "  Narrowing conversions: " << stats_.narrowing_conversions << "\n";
    out << "  Loop overflows: " << stats_.loop_overflows << "\n\n";

    if (issues_.empty()) {
        out << "✅ No overflow issues detected!\n";
        return;
    }

    // Print critical issues first
    auto critical_issues = get_issues_by_severity(OverflowSeverity::CRITICAL);
    if (!critical_issues.empty()) {
        out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        out << "🔴 CRITICAL ISSUES (Undefined Behavior)\n";
        out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

        size_t count = 0;
        for (const auto& issue : critical_issues) {
            if (max_issues > 0 && count >= max_issues) break;

            out << "[" << (count + 1) << "] " << issue.description << "\n";
            out << "    Location: " << issue.file << ":" << issue.line << " (" << issue.function << ")\n";
            if (!issue.code_snippet.empty()) {
                out << "    Code: " << issue.code_snippet << "\n";
            }
            out << "    Suggestion: " << issue.suggestion << "\n\n";
            count++;
        }
    }

    // Print warnings
    auto warning_issues = get_issues_by_severity(OverflowSeverity::WARNING);
    if (!warning_issues.empty()) {
        out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        out << "⚠️  WARNINGS\n";
        out << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

        size_t count = 0;
        for (const auto& issue : warning_issues) {
            if (max_issues > 0 && count >= max_issues) break;

            out << "[" << (count + 1) << "] " << issue.description << "\n";
            out << "    Location: " << issue.file << ":" << issue.line << " (" << issue.function << ")\n";
            if (!issue.code_snippet.empty()) {
                out << "    Code: " << issue.code_snippet << "\n";
            }
            out << "    Suggestion: " << issue.suggestion << "\n\n";
            count++;
        }
    }
}

void OverflowDetector::export_json(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        llvm::errs() << "Error: Could not open file for writing: " << filename << "\n";
        return;
    }

    file << "{\n";
    file << "  \"statistics\": {\n";
    file << "    \"total_issues\": " << stats_.total_issues << ",\n";
    file << "    \"critical_issues\": " << stats_.critical_issues << ",\n";
    file << "    \"warning_issues\": " << stats_.warning_issues << ",\n";
    file << "    \"info_issues\": " << stats_.info_issues << ",\n";
    file << "    \"signed_overflows\": " << stats_.signed_overflows << ",\n";
    file << "    \"unsigned_wraparounds\": " << stats_.unsigned_wraparounds << ",\n";
    file << "    \"mixed_signedness\": " << stats_.mixed_signedness << ",\n";
    file << "    \"narrowing_conversions\": " << stats_.narrowing_conversions << ",\n";
    file << "    \"loop_overflows\": " << stats_.loop_overflows << "\n";
    file << "  },\n";
    file << "  \"issues\": [\n";

    for (size_t i = 0; i < issues_.size(); ++i) {
        const auto& issue = issues_[i];
        file << "    {\n";
        file << "      \"type\": \"" << overflow_type_to_string(issue.type) << "\",\n";
        file << "      \"severity\": \"" << severity_to_string(issue.severity) << "\",\n";
        file << "      \"file\": \"" << issue.file << "\",\n";
        file << "      \"line\": " << issue.line << ",\n";
        file << "      \"column\": " << issue.column << ",\n";
        file << "      \"function\": \"" << issue.function << "\",\n";
        file << "      \"description\": \"" << issue.description << "\",\n";
        file << "      \"suggestion\": \"" << issue.suggestion << "\"\n";
        file << "    }";
        if (i < issues_.size() - 1) {
            file << ",";
        }
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    file.close();
}

void OverflowDetector::export_text(const std::string& filename) const {
    std::error_code EC;
    llvm::raw_fd_ostream file(filename, EC);
    if (EC) {
        llvm::errs() << "Error: Could not open file for writing: " << filename << "\n";
        return;
    }

    print_report(file, 0);
    file.flush();
}

std::vector<OverflowIssue> OverflowDetector::get_issues_by_severity(OverflowSeverity severity) const {
    std::vector<OverflowIssue> result;
    for (const auto& issue : issues_) {
        if (issue.severity == severity) {
            result.push_back(issue);
        }
    }
    return result;
}

std::vector<OverflowIssue> OverflowDetector::get_issues_by_file(const std::string& file) const {
    std::vector<OverflowIssue> result;
    for (const auto& issue : issues_) {
        if (issue.file == file) {
            result.push_back(issue);
        }
    }
    return result;
}

// String conversion functions

const char* overflow_type_to_string(OverflowType type) {
    switch (type) {
        case OverflowType::SIGNED_ADDITION: return "signed_addition_overflow";
        case OverflowType::SIGNED_SUBTRACTION: return "signed_subtraction_overflow";
        case OverflowType::SIGNED_MULTIPLICATION: return "signed_multiplication_overflow";
        case OverflowType::SIGNED_DIVISION: return "signed_division_overflow";
        case OverflowType::SIGNED_NEGATION: return "signed_negation_overflow";
        case OverflowType::SIGNED_LEFT_SHIFT: return "signed_left_shift_overflow";
        case OverflowType::UNSIGNED_WRAPAROUND: return "unsigned_wraparound";
        case OverflowType::MIXED_SIGNEDNESS: return "mixed_signedness";
        case OverflowType::NARROWING_CONVERSION: return "narrowing_conversion";
        case OverflowType::LOOP_COUNTER_OVERFLOW: return "loop_counter_overflow";
        default: return "unknown";
    }
}

const char* severity_to_string(OverflowSeverity severity) {
    switch (severity) {
        case OverflowSeverity::CRITICAL: return "critical";
        case OverflowSeverity::WARNING: return "warning";
        case OverflowSeverity::INFO: return "info";
        default: return "unknown";
    }
}

} // namespace analysis
} // namespace optiweave
