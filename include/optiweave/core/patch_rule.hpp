#pragma once

#include <optiweave/core/safety_tier.hpp>
#include <optiweave/core/rule_result.hpp>
#include <optiweave/core/rule_config.hpp>
#include <optiweave/analysis/optimization_pattern.hpp>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Decl.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <string>
#include <string_view>
#include <vector>

namespace optiweave {
namespace core {

/// Abstract base class for auto-patch rules.
class PatchRule {
public:
    virtual ~PatchRule() = default;

    /// Unique rule identifier (e.g., "division-reciprocal")
    virtual std::string id() const = 0;

    /// Human-readable display name
    virtual std::string display_name() const = 0;

    /// Short description
    virtual std::string description() const = 0;

    /// Safety classification
    virtual SafetyTier safety_tier() const = 0;

    /// Which optimization pattern names this rule handles
    virtual std::vector<std::string_view> handled_patterns() const = 0;

    /// Apply patch to a loop body.
    /// @param keyword_loc  Location of the loop keyword (for, while, do)
    /// @param body         Loop body statement
    /// @param pat          Matched optimization pattern
    /// @param rewriter     Clang rewriter
    /// @param ctx          AST context
    /// @param induction_vars  Enclosing loop induction variable names
    /// @param is_c         Whether source is C (vs C++)
    /// @param dry_run      If true, don't actually modify
    /// @param recip_counter  Mutable counter for unique reciprocal names
    virtual RuleResult try_apply_loop(clang::SourceLocation keyword_loc,
                                       clang::Stmt* body,
                                       const analysis::OptimizationPattern& pat,
                                       clang::Rewriter& rewriter,
                                       clang::ASTContext& ctx,
                                       const std::vector<std::string>& induction_vars,
                                       bool is_c, bool dry_run,
                                       int& recip_counter) {
        (void)keyword_loc; (void)body; (void)pat; (void)rewriter;
        (void)ctx; (void)induction_vars; (void)is_c; (void)dry_run;
        (void)recip_counter;
        return RuleResult::skip();
    }

    /// Apply patch to a function declaration (e.g., restrict qualifier).
    virtual RuleResult try_apply_function(clang::FunctionDecl* func,
                                           const analysis::OptimizationPattern& pat,
                                           clang::Rewriter& rewriter,
                                           clang::ASTContext& ctx,
                                           bool is_c, bool dry_run) {
        (void)func; (void)pat; (void)rewriter; (void)ctx; (void)is_c; (void)dry_run;
        return RuleResult::skip();
    }

    /// Whether this rule operates at the function level (vs loop level)
    virtual bool is_function_level() const { return false; }
};

} // namespace core
} // namespace optiweave
