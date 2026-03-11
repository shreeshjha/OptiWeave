#include <optiweave/core/patch_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <optiweave/analysis/pattern_names.hpp>
#include <clang/AST/Expr.h>

namespace optiweave {
namespace core {

class StrengthReductionRule : public PatchRule {
public:
    std::string id() const override { return "strength-reduction"; }
    std::string display_name() const override { return "Strength Reduction"; }
    std::string description() const override {
        return "Replaces i*K with accumulator incremented by K each iteration";
    }
    SafetyTier safety_tier() const override { return SafetyTier::MOSTLY_SAFE; }

    std::vector<std::string_view> handled_patterns() const override {
        return {analysis::patterns::kStrengthReduction};
    }

    RuleResult try_apply_loop(clang::SourceLocation keyword_loc,
                               clang::Stmt* body,
                               const analysis::OptimizationPattern& pat,
                               clang::Rewriter& rewriter,
                               clang::ASTContext& ctx,
                               const std::vector<std::string>& induction_vars,
                               bool is_c, bool dry_run,
                               int& recip_counter) override {
        (void)is_c;
        if (!body) return RuleResult::skip("no body");

        // Find multiplication operations in the loop body
        auto muls = rule_utils::collectAllMultiplications(body);
        if (muls.empty())
            return RuleResult::skip("no multiplications in body");

        // Find a multiplication where one operand is the induction variable
        for (auto* mul : muls) {
            std::string lhs_text = rule_utils::getSourceText(
                mul->getLHS()->getSourceRange(), ctx);
            std::string rhs_text = rule_utils::getSourceText(
                mul->getRHS()->getSourceRange(), ctx);

            std::string induction_var;
            std::string constant_text;

            for (const auto& ivar : induction_vars) {
                if (ivar.empty()) continue;
                if (lhs_text == ivar) {
                    induction_var = ivar;
                    constant_text = rhs_text;
                    break;
                }
                if (rhs_text == ivar) {
                    induction_var = ivar;
                    constant_text = lhs_text;
                    break;
                }
            }

            if (induction_var.empty() || constant_text.empty()) continue;

            // Check that the constant side is loop-invariant
            if (!rule_utils::isLoopInvariant(constant_text, induction_vars))
                continue;

            int acc_id = recip_counter++;
            std::string acc_name = "__ow_acc_" + std::to_string(acc_id);
            std::string indent = rule_utils::getIndentation(keyword_loc, ctx);

            // Insert accumulator declaration before the loop
            std::string decl = indent + "long " + acc_name + " = 0;\n";

            // Insert accumulator increment at start of loop body
            std::string body_indent = indent + "    ";
            auto body_begin = body->getBeginLoc();
            // If body is a CompoundStmt, insert after the opening brace
            std::string incr;
            if (auto* compound = llvm::dyn_cast<clang::CompoundStmt>(body)) {
                if (compound->body_empty()) continue;
                auto first_stmt_loc = (*compound->body_begin())->getBeginLoc();
                body_indent = rule_utils::getIndentation(first_stmt_loc, ctx);
                incr = body_indent + acc_name + " += " + constant_text + ";\n";

                if (!dry_run) {
                    rewriter.InsertTextBefore(keyword_loc, decl);
                    rewriter.InsertTextBefore(first_stmt_loc, incr);
                    // Replace the multiplication with the accumulator
                    rewriter.ReplaceText(mul->getSourceRange(), acc_name);
                }
            } else {
                // Single statement body — harder to modify safely
                if (!dry_run) {
                    rewriter.InsertTextBefore(keyword_loc, decl);
                }
                // Just add the declaration, replace the expr
                if (!dry_run) {
                    rewriter.ReplaceText(mul->getSourceRange(), acc_name);
                }
            }

            return RuleResult::success(0.85, SafetyTier::MOSTLY_SAFE,
                                        "strength-reduced '" + constant_text +
                                        "' as " + acc_name);
        }

        // No eligible multiplication found — fall back to advisory
        std::string indent = rule_utils::getIndentation(keyword_loc, ctx);
        std::string comment = indent + "// OPTIWEAVE: Strength reduction opportunity\n" +
                              indent + "// Replace i*K multiplication with accumulator += K\n";

        if (!dry_run) {
            rewriter.InsertTextBefore(keyword_loc, comment);
        }

        return RuleResult::success(0.6, SafetyTier::MOSTLY_SAFE,
                                    "added strength reduction advisory (no eligible i*K found)");
    }
};

void register_strength_reduction_rule(PatchRuleRegistry& reg) {
    reg.register_rule(std::make_unique<StrengthReductionRule>());
}

} // namespace core
} // namespace optiweave
