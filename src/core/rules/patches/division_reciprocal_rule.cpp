#include <optiweave/core/patch_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <optiweave/analysis/pattern_names.hpp>
#include <set>

namespace optiweave {
namespace core {

class DivisionReciprocalRule : public PatchRule {
public:
    std::string id() const override { return "division-reciprocal"; }
    std::string display_name() const override { return "Division-to-Reciprocal Hoisting"; }
    std::string description() const override {
        return "Hoists loop-invariant divisors as reciprocals before the loop";
    }
    SafetyTier safety_tier() const override { return SafetyTier::AGGRESSIVE; }

    std::vector<std::string_view> handled_patterns() const override {
        return {analysis::patterns::kDivisionInHotLoop};
    }

    RuleResult try_apply_loop(clang::SourceLocation keyword_loc,
                               clang::Stmt* body,
                               const analysis::OptimizationPattern& pat,
                               clang::Rewriter& rewriter,
                               clang::ASTContext& ctx,
                               const std::vector<std::string>& induction_vars,
                               bool is_c, bool dry_run,
                               int& recip_counter) override {
        auto divisions = rule_utils::collectAllDivisions(body);
        if (divisions.empty()) return RuleResult::skip("no divisions found");

        std::string indent = rule_utils::getIndentation(keyword_loc, ctx);
        bool any_applied = false;
        std::set<std::string> already_hoisted_rhs;
        std::string all_reasons;

        for (auto* div_op : divisions) {
            if (div_op->getOperatorLoc().isMacroID()) continue;

            auto* rhs = div_op->getRHS();
            if (!rhs) continue;

            // Skip integer division
            if (div_op->getLHS()->getType()->isIntegerType() &&
                rhs->getType()->isIntegerType())
                continue;

            std::string rhs_text = rule_utils::getSourceText(rhs->getSourceRange(), ctx);
            if (rhs_text.empty()) continue;
            if (already_hoisted_rhs.count(rhs_text)) continue;

            if (!rule_utils::isLoopInvariant(rhs_text, induction_vars))
                continue;

            recip_counter++;
            std::string recip_name = "__ow_recip_" + std::to_string(recip_counter);
            already_hoisted_rhs.insert(rhs_text);

            std::string type_keyword = is_c ? "const double" : "const auto";
            std::string recip_decl = indent + type_keyword + " " + recip_name +
                                      " = 1.0 / (" + rhs_text + ");" +
                                      " /* OPTIWEAVE: ensure divisor != 0 */\n";

            if (!dry_run) {
                rewriter.InsertTextBefore(keyword_loc, recip_decl);

                if (div_op->getOpcode() == clang::BO_Div) {
                    rewriter.ReplaceText(div_op->getOperatorLoc(), 1, "*");
                    rewriter.ReplaceText(rhs->getSourceRange(), recip_name);
                } else if (div_op->getOpcode() == clang::BO_DivAssign) {
                    rewriter.ReplaceText(div_op->getOperatorLoc(), 2, "*=");
                    rewriter.ReplaceText(rhs->getSourceRange(), recip_name);
                }
            }

            if (!all_reasons.empty()) all_reasons += "; ";
            all_reasons += "hoisted '" + rhs_text + "' as " + recip_name;
            any_applied = true;
        }

        if (!any_applied) return RuleResult::skip("no invariant divisions");

        return RuleResult::success(1.0, SafetyTier::AGGRESSIVE, all_reasons);
    }
};

void register_division_reciprocal_rule(PatchRuleRegistry& reg) {
    reg.register_rule(std::make_unique<DivisionReciprocalRule>());
}

} // namespace core
} // namespace optiweave
