#pragma once

#include <optiweave/analysis/optimization_pattern.hpp>
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
                 bool is_c_language = false);

    bool VisitForStmt(clang::ForStmt* stmt);
    bool VisitWhileStmt(clang::WhileStmt* stmt);
    bool VisitDoStmt(clang::DoStmt* stmt);
    bool VisitBinaryOperator(clang::BinaryOperator* op);

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

    /// Dedup tracker keyed by "file:line:pattern"
    std::set<std::string> applied_locations_;

    /// Tracks ALL enclosing loop induction variables for invariance checks
    std::vector<std::string> enclosing_induction_vars_;

    // --- Location matching ---

    /// Compare filenames using basename equality
    bool filesMatch(const std::string& a, const std::string& b) const;

    /// Find a matching suggestion for a given source location and pattern name.
    /// Uses basename equality and fuzzy line matching (±3 lines, closest wins).
    const analysis::OptimizationPattern* findMatch(
        clang::SourceLocation loc, const std::string& pattern_name) const;

    /// Find a matching suggestion for a given source location (any pattern)
    const analysis::OptimizationPattern* findMatchByLocation(
        clang::SourceLocation loc) const;

    // --- Helpers ---

    /// Get source text for a source range
    std::string getSourceText(clang::SourceRange range) const;

    /// Get the induction variable name from a for-loop init statement
    std::string getLoopInductionVar(clang::ForStmt* loop) const;

    /// Collect ALL division operators in a statement (not just the first)
    std::vector<clang::BinaryOperator*> collectAllDivisions(clang::Stmt* stmt) const;

    /// Check if a divisor expression is loop-invariant (not referencing any enclosing loop var)
    bool isLoopInvariant(const std::string& expr_text) const;

    /// Get indentation string for a source location (spaces/tabs from line start)
    std::string getIndentation(clang::SourceLocation loc) const;

    /// Check if a #pragma clang loop vectorize already exists within 3 lines above loc
    bool checkExistingPragma(clang::SourceLocation loc) const;

    // --- Shared loop patch logic ---

    /// Handle patches for any loop type (for/while/do).
    /// keyword_loc is the location of the loop keyword, body is the loop body,
    /// loop_stmt is the full loop statement for advisory placement.
    void handleLoopPatches(clang::Stmt* body,
                           clang::SourceLocation keyword_loc,
                           clang::Stmt* loop_stmt);

    // --- Concrete patch generators ---

    bool applyDivisionPatch(clang::SourceLocation insert_loc,
                            clang::Stmt* body,
                            const analysis::OptimizationPattern& pat);
    bool applyVectorizationPatch(clang::SourceLocation keyword_loc,
                                  const analysis::OptimizationPattern& pat);
    bool applyAdvisoryComment(clang::SourceLocation loc,
                               const analysis::OptimizationPattern& pat);
};

} // namespace core
} // namespace optiweave
