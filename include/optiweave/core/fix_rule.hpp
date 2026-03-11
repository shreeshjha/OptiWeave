#pragma once

#include <optiweave/core/safety_tier.hpp>
#include <optiweave/core/rule_result.hpp>
#include <optiweave/core/rule_config.hpp>
#include <optiweave/analysis/bug_fix_issue.hpp>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Decl.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <string>
#include <vector>

namespace optiweave {
namespace core {

/// AST node kinds that fix rules can subscribe to
enum class ASTNodeKind {
    BinaryOperator,
    UnaryOperator,
    VarDecl,
    CallExpr,
    SwitchStmt,
    IfStmt
};

/// Abstract base class for auto-fix rules.
/// Each rule is self-contained with its own safety tier, confidence scoring,
/// and transformation logic.
class FixRule {
public:
    virtual ~FixRule() = default;

    /// Unique rule identifier (e.g., "unsigned-wraparound")
    virtual std::string id() const = 0;

    /// Human-readable display name
    virtual std::string display_name() const = 0;

    /// Short description of what the rule does
    virtual std::string description() const = 0;

    /// Safety classification
    virtual SafetyTier safety_tier() const = 0;

    /// Which AST node kinds this rule wants to visit
    virtual std::vector<ASTNodeKind> subscribed_nodes() const = 0;

    // --- Issue-driven overloads (matched against BugFixIssue from analyzers) ---

    virtual RuleResult try_apply(clang::BinaryOperator* op,
                                  const analysis::BugFixIssue& issue,
                                  clang::Rewriter& rewriter,
                                  clang::ASTContext& ctx,
                                  bool is_c, bool dry_run) {
        (void)op; (void)issue; (void)rewriter; (void)ctx; (void)is_c; (void)dry_run;
        return RuleResult::skip();
    }

    virtual RuleResult try_apply(clang::UnaryOperator* op,
                                  const analysis::BugFixIssue& issue,
                                  clang::Rewriter& rewriter,
                                  clang::ASTContext& ctx,
                                  bool is_c, bool dry_run) {
        (void)op; (void)issue; (void)rewriter; (void)ctx; (void)is_c; (void)dry_run;
        return RuleResult::skip();
    }

    virtual RuleResult try_apply(clang::VarDecl* decl,
                                  const analysis::BugFixIssue& issue,
                                  clang::Rewriter& rewriter,
                                  clang::ASTContext& ctx,
                                  bool is_c, bool dry_run) {
        (void)decl; (void)issue; (void)rewriter; (void)ctx; (void)is_c; (void)dry_run;
        return RuleResult::skip();
    }

    // --- Standalone overloads (self-matching, no pre-existing BugFixIssue) ---

    virtual RuleResult try_apply_standalone(clang::BinaryOperator* op,
                                             clang::Rewriter& rewriter,
                                             clang::ASTContext& ctx,
                                             bool is_c, bool dry_run) {
        (void)op; (void)rewriter; (void)ctx; (void)is_c; (void)dry_run;
        return RuleResult::skip();
    }

    virtual RuleResult try_apply_standalone(clang::UnaryOperator* op,
                                             clang::Rewriter& rewriter,
                                             clang::ASTContext& ctx,
                                             bool is_c, bool dry_run) {
        (void)op; (void)rewriter; (void)ctx; (void)is_c; (void)dry_run;
        return RuleResult::skip();
    }

    virtual RuleResult try_apply_standalone(clang::VarDecl* decl,
                                             clang::Rewriter& rewriter,
                                             clang::ASTContext& ctx,
                                             bool is_c, bool dry_run) {
        (void)decl; (void)rewriter; (void)ctx; (void)is_c; (void)dry_run;
        return RuleResult::skip();
    }

    virtual RuleResult try_apply_standalone(clang::CallExpr* call,
                                             clang::Rewriter& rewriter,
                                             clang::ASTContext& ctx,
                                             bool is_c, bool dry_run) {
        (void)call; (void)rewriter; (void)ctx; (void)is_c; (void)dry_run;
        return RuleResult::skip();
    }

    virtual RuleResult try_apply_standalone(clang::SwitchStmt* stmt,
                                             clang::Rewriter& rewriter,
                                             clang::ASTContext& ctx,
                                             bool is_c, bool dry_run) {
        (void)stmt; (void)rewriter; (void)ctx; (void)is_c; (void)dry_run;
        return RuleResult::skip();
    }

    virtual RuleResult try_apply_standalone(clang::IfStmt* stmt,
                                             clang::Rewriter& rewriter,
                                             clang::ASTContext& ctx,
                                             bool is_c, bool dry_run) {
        (void)stmt; (void)rewriter; (void)ctx; (void)is_c; (void)dry_run;
        return RuleResult::skip();
    }

    /// Whether this rule needs a pre-existing BugFixIssue or can self-match
    virtual bool is_standalone() const { return false; }

    /// Map from legacy fix-kind name to this rule's id
    virtual analysis::BugFixKind mapped_bug_fix_kind() const {
        return analysis::BugFixKind::UnsignedWraparound; // default; override in subclass
    }

    /// Whether this rule maps to a legacy BugFixKind
    virtual bool has_legacy_kind() const { return false; }
};

} // namespace core
} // namespace optiweave
