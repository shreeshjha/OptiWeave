#pragma once

#include <optiweave/analysis/optimization_pattern.hpp>
#include <optiweave/core/rule_config.hpp>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTContext.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <set>
#include <string>
#include <vector>

namespace optiweave {
namespace core {

class PatchVisitor : public clang::RecursiveASTVisitor<PatchVisitor> {
public:
    PatchVisitor(clang::Rewriter& rewriter,
                 clang::ASTContext& context,
                 const std::vector<analysis::OptimizationPattern>& suggestions,
                 bool dry_run = false,
                 bool is_c_language = false,
                 const RuleConfig& config = RuleConfig{},
                 bool verbose_rules = false);

    bool VisitForStmt(clang::ForStmt* stmt);
    bool VisitWhileStmt(clang::WhileStmt* stmt);
    bool VisitDoStmt(clang::DoStmt* stmt);
    bool VisitBinaryOperator(clang::BinaryOperator* op);
    bool VisitFunctionDecl(clang::FunctionDecl* func);

    size_t patches_applied() const { return applied_; }
    size_t advisories_applied() const { return advisories_; }
    size_t skipped_count() const { return skipped_; }
    const std::vector<std::string>& patch_log() const { return log_; }

private:
    clang::Rewriter& rewriter_;
    clang::ASTContext& context_;
    const std::vector<analysis::OptimizationPattern>& suggestions_;
    bool dry_run_;
    bool is_c_language_;
    size_t applied_ = 0;
    size_t advisories_ = 0;
    size_t skipped_ = 0;
    std::vector<std::string> log_;
    int recip_counter_ = 0;
    RuleConfig config_;
    bool verbose_rules_;

    /// Dedup tracker keyed by "file:line:pattern"
    std::set<std::string> applied_locations_;

    /// Tracks ALL enclosing loop induction variables for invariance checks
    std::vector<std::string> enclosing_induction_vars_;

    // --- Location matching ---
    bool filesMatch(const std::string& a, const std::string& b) const;
    const analysis::OptimizationPattern* findMatch(
        clang::SourceLocation loc, const std::string& pattern_name) const;
    const analysis::OptimizationPattern* findMatchByLocation(
        clang::SourceLocation loc) const;

    // --- Shared loop patch logic ---
    void handleLoopPatches(clang::Stmt* body,
                           clang::SourceLocation keyword_loc,
                           clang::Stmt* loop_stmt);
};

} // namespace core
} // namespace optiweave
