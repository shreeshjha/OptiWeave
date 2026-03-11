#pragma once

#include <optiweave/analysis/bug_fix_issue.hpp>
#include <optiweave/core/rule_config.hpp>
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
                  std::set<analysis::BugFixKind> enabled_kinds = {},
                  const RuleConfig& config = RuleConfig{},
                  bool verbose_rules = false);

    bool VisitBinaryOperator(clang::BinaryOperator* op);
    bool VisitUnaryOperator(clang::UnaryOperator* op);
    bool VisitVarDecl(clang::VarDecl* decl);
    bool VisitCallExpr(clang::CallExpr* call);
    bool VisitSwitchStmt(clang::SwitchStmt* stmt);
    bool VisitIfStmt(clang::IfStmt* stmt);

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
    RuleConfig config_;
    bool verbose_rules_;

    /// Dedup tracker keyed by "file:line:ruleId"
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
    bool isInMacro(clang::SourceLocation loc) const;
    bool isKindEnabled(analysis::BugFixKind kind) const;
    bool isRuleEnabled(const std::string& rule_id) const;
    std::string kindToString(analysis::BugFixKind kind) const;
};

} // namespace core
} // namespace optiweave
