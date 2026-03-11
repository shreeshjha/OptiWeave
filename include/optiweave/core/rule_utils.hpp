#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <llvm/Support/Path.h>
#include <set>
#include <string>
#include <vector>

namespace optiweave {
namespace core {
namespace rule_utils {

/// Compare filenames: exact match preferred, fall back to basename if one is bare.
inline bool filesMatch(const std::string& a, const std::string& b) {
    if (a == b) return true;
    bool a_bare = (a.find('/') == std::string::npos);
    bool b_bare = (b.find('/') == std::string::npos);
    if (a_bare || b_bare)
        return llvm::sys::path::filename(a) == llvm::sys::path::filename(b);
    return false;
}

/// Get source text for a token range.
inline std::string getSourceText(clang::SourceRange range,
                                  clang::ASTContext& ctx) {
    auto& sm = ctx.getSourceManager();
    auto& lo = ctx.getLangOpts();
    auto char_range = clang::CharSourceRange::getTokenRange(range);
    return clang::Lexer::getSourceText(char_range, sm, lo).str();
}

/// Get indentation (whitespace from line start) at a source location.
inline std::string getIndentation(clang::SourceLocation loc,
                                   clang::ASTContext& ctx) {
    if (!loc.isValid()) return "";
    auto& sm = ctx.getSourceManager();
    auto file_id = sm.getFileID(sm.getSpellingLoc(loc));
    bool invalid = false;
    auto buf = sm.getBufferData(file_id, &invalid);
    if (invalid || buf.empty()) return "";

    unsigned offset = sm.getFileOffset(sm.getSpellingLoc(loc));
    unsigned line_start = offset;
    while (line_start > 0 && buf[line_start - 1] != '\n')
        line_start--;

    std::string indent;
    for (unsigned i = line_start; i < offset && i < buf.size(); i++) {
        if (buf[i] == ' ' || buf[i] == '\t')
            indent += buf[i];
        else
            break;
    }
    return indent;
}

/// Check if a source location is inside a macro expansion.
inline bool isInMacro(clang::SourceLocation loc) {
    return loc.isValid() && loc.isMacroID();
}

/// Inject `#include <header>` if not already present in the file.
/// Returns true if injected. Uses injected_set for idempotency across calls.
inline bool injectIncludeIfMissing(clang::FileID file_id,
                                    const std::string& header,
                                    clang::Rewriter& rewriter,
                                    clang::ASTContext& ctx,
                                    std::set<std::string>& injected_set,
                                    bool dry_run,
                                    std::vector<std::string>* log = nullptr) {
    if (injected_set.count(header)) return false;

    auto& sm = ctx.getSourceManager();
    bool invalid = false;
    auto buf = sm.getBufferData(file_id, &invalid);
    if (invalid || buf.empty()) return false;

    if (buf.find(header) != llvm::StringRef::npos) {
        injected_set.insert(header);
        return false;
    }

    if (dry_run) {
        if (log) log->push_back("WOULD INJECT: #include " + header);
        injected_set.insert(header);
        return true;
    }

    std::string buf_str = buf.str();
    size_t last_include_end = std::string::npos;
    size_t pos = 0;
    while ((pos = buf_str.find("#include", pos)) != std::string::npos) {
        size_t end_of_line = buf_str.find('\n', pos);
        if (end_of_line != std::string::npos)
            last_include_end = end_of_line + 1;
        else
            last_include_end = buf_str.size();
        pos++;
    }

    std::string inject = "#include " + header + "\n";
    clang::SourceLocation insert_loc;
    if (last_include_end != std::string::npos)
        insert_loc = sm.getLocForStartOfFile(file_id)
                       .getLocWithOffset(static_cast<int>(last_include_end));
    else
        insert_loc = sm.getLocForStartOfFile(file_id);

    rewriter.InsertTextBefore(insert_loc, inject);
    injected_set.insert(header);
    return true;
}

/// Compute confidence based on line-match fuzziness.
/// Exact match = 1.0, decays 0.1 per line of offset.
inline double computeLocationConfidence(int actual_line, int expected_line) {
    int delta = std::abs(actual_line - expected_line);
    if (delta == 0) return 1.0;
    if (delta > 3) return 0.0; // outside fuzzy window
    return 1.0 - delta * 0.1;
}

/// Check if a #pragma already exists within N lines above loc.
inline bool checkExistingPragma(clang::SourceLocation loc,
                                 clang::ASTContext& ctx,
                                 int lines_to_check = 3) {
    if (!loc.isValid()) return false;
    auto& sm = ctx.getSourceManager();
    auto file_id = sm.getFileID(sm.getSpellingLoc(loc));
    bool invalid = false;
    auto buf = sm.getBufferData(file_id, &invalid);
    if (invalid || buf.empty()) return false;

    unsigned offset = sm.getFileOffset(sm.getSpellingLoc(loc));
    int lines_scanned = 0;
    unsigned pos = offset;
    while (pos > 0 && lines_scanned < lines_to_check + 1) {
        pos--;
        if (buf[pos] == '\n') lines_scanned++;
    }

    if (pos < offset) {
        auto region = buf.substr(pos, offset - pos);
        if (region.find("#pragma clang loop vectorize") != llvm::StringRef::npos ||
            region.find("#pragma GCC ivdep") != llvm::StringRef::npos)
            return true;
    }
    return false;
}

/// Get the induction variable name from a for-loop init statement.
inline std::string getLoopInductionVar(clang::ForStmt* loop) {
    if (auto* init = loop->getInit()) {
        if (auto* decl_stmt = llvm::dyn_cast<clang::DeclStmt>(init)) {
            if (decl_stmt->isSingleDecl()) {
                if (auto* var = llvm::dyn_cast<clang::VarDecl>(decl_stmt->getSingleDecl()))
                    return var->getNameAsString();
            }
        }
    }
    return "";
}

/// Collect ALL division operators in a statement tree.
inline std::vector<clang::BinaryOperator*> collectAllDivisions(clang::Stmt* stmt) {
    std::vector<clang::BinaryOperator*> result;
    if (!stmt) return result;
    if (auto* bin_op = llvm::dyn_cast<clang::BinaryOperator>(stmt)) {
        if (bin_op->getOpcode() == clang::BO_Div ||
            bin_op->getOpcode() == clang::BO_DivAssign)
            result.push_back(bin_op);
    }
    for (auto* child : stmt->children()) {
        auto child_divs = collectAllDivisions(child);
        result.insert(result.end(), child_divs.begin(), child_divs.end());
    }
    return result;
}

/// Check if expression text is loop-invariant (doesn't reference any induction var).
inline bool isLoopInvariant(const std::string& expr_text,
                             const std::vector<std::string>& induction_vars) {
    for (const auto& var : induction_vars) {
        if (!var.empty() && expr_text.find(var) != std::string::npos)
            return false;
    }
    return true;
}

/// Collect ALL multiplication operators in a statement tree.
inline std::vector<clang::BinaryOperator*> collectAllMultiplications(clang::Stmt* stmt) {
    std::vector<clang::BinaryOperator*> result;
    if (!stmt) return result;
    if (auto* bin_op = llvm::dyn_cast<clang::BinaryOperator>(stmt)) {
        if (bin_op->getOpcode() == clang::BO_Mul ||
            bin_op->getOpcode() == clang::BO_MulAssign)
            result.push_back(bin_op);
    }
    for (auto* child : stmt->children()) {
        auto child_muls = collectAllMultiplications(child);
        result.insert(result.end(), child_muls.begin(), child_muls.end());
    }
    return result;
}

/// Collect ALL array subscript expressions in a statement tree.
inline std::vector<clang::ArraySubscriptExpr*> collectArraySubscripts(clang::Stmt* stmt) {
    std::vector<clang::ArraySubscriptExpr*> result;
    if (!stmt) return result;
    if (auto* sub = llvm::dyn_cast<clang::ArraySubscriptExpr>(stmt)) {
        result.push_back(sub);
    }
    for (auto* child : stmt->children()) {
        auto child_subs = collectArraySubscripts(child);
        result.insert(result.end(), child_subs.begin(), child_subs.end());
    }
    return result;
}

} // namespace rule_utils
} // namespace core
} // namespace optiweave
