#pragma once

#include <optiweave/analysis/bug_fix_issue.hpp>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/FileManager.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <set>
#include <string>
#include <vector>

namespace optiweave {
namespace core {

class BugFixVisitor : public clang::RecursiveASTVisitor<BugFixVisitor> {
public:
    BugFixVisitor(clang::Rewriter& rewriter,
                  clang::ASTContext& context,
                  const std::vector<analysis::BugFixIssue>& issues,
                  bool dry_run = false,
                  bool is_c_language = false,
                  std::set<analysis::BugFixKind> enabled_kinds = {});

    bool VisitBinaryOperator(clang::BinaryOperator* op);
    bool VisitUnaryOperator(clang::UnaryOperator* op);
    bool VisitVarDecl(clang::VarDecl* decl);

    size_t fixes_applied() const { return applied_; }
    size_t skipped_count() const { return skipped_; }
    const std::vector<std::string>& fix_log() const { return log_; }

private:
    clang::Rewriter& rewriter_;
    clang::ASTContext& context_;
    const std::vector<analysis::BugFixIssue>& issues_;
    bool dry_run_;
    bool is_c_language_;
    size_t applied_ = 0;
    size_t skipped_ = 0;
    std::vector<std::string> log_;

    /// Dedup tracker keyed by "file:line:kind"
    std::set<std::string> applied_locations_;

    /// Injected include headers (idempotency guard across multiple fixes in one file)
    std::set<std::string> injected_includes_;

    /// If non-empty, only these fix kinds are applied
    std::set<analysis::BugFixKind> enabled_kinds_;

    // --- Location matching ---
    bool filesMatch(const std::string& a, const std::string& b) const;
    const analysis::BugFixIssue* findMatch(
        clang::SourceLocation loc, analysis::BugFixKind kind) const;

    // --- Helpers ---
    std::string getSourceText(clang::SourceRange range) const;
    std::string getIndentation(clang::SourceLocation loc) const;
    bool isInMacro(clang::SourceLocation loc) const;
    std::string kindToString(analysis::BugFixKind kind) const;

    /// Inject `#include <header>` if not already present. Returns true if injected.
    bool injectIncludeIfMissing(clang::FileID file_id, const std::string& header);

    /// Returns true if the given fix kind is enabled (empty set = all enabled)
    bool isKindEnabled(analysis::BugFixKind kind) const;

    // --- Fix applicators ---
    bool applyUnsignedWraparoundFix(clang::BinaryOperator* op,
                                     const analysis::BugFixIssue& issue);
    bool applySignedNegationFix(clang::UnaryOperator* op,
                                 const analysis::BugFixIssue& issue);
    bool applySignedLeftShiftFix(clang::BinaryOperator* op,
                                  const analysis::BugFixIssue& issue);
    bool applyUninitializedVarFix(clang::VarDecl* decl,
                                   const analysis::BugFixIssue& issue);
    bool applyUnusedVarFix(clang::VarDecl* decl,
                            const analysis::BugFixIssue& issue);
    bool applyFPEqualityFix(clang::BinaryOperator* op,
                             const analysis::BugFixIssue& issue);
};

} // namespace core
} // namespace optiweave
