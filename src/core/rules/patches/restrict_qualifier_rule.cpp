#include <optiweave/core/patch_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <optiweave/analysis/pattern_names.hpp>
#include <clang/AST/DeclCXX.h>
#include <clang/Lex/Lexer.h>

namespace optiweave {
namespace core {

class RestrictQualifierRule : public PatchRule {
public:
    std::string id() const override { return "restrict-qualifier"; }
    std::string display_name() const override { return "Restrict Qualifier"; }
    std::string description() const override {
        return "Adds __restrict to non-aliasing pointer parameters";
    }
    SafetyTier safety_tier() const override { return SafetyTier::AGGRESSIVE; }

    std::vector<std::string_view> handled_patterns() const override {
        return {analysis::patterns::kRestrictQualifier};
    }

    bool is_function_level() const override { return true; }

    RuleResult try_apply_function(clang::FunctionDecl* func,
                                   const analysis::OptimizationPattern& pat,
                                   clang::Rewriter& rewriter,
                                   clang::ASTContext& ctx,
                                   bool is_c, bool dry_run) override {
        (void)pat;
        if (!func->hasBody()) return RuleResult::skip();

        int restricted = 0;
        for (unsigned i = 0; i < func->getNumParams(); i++) {
            auto* param = func->getParamDecl(i);
            if (!param->getType()->isPointerType()) continue;

            // Skip if already restrict
            std::string param_text = rule_utils::getSourceText(
                param->getSourceRange(), ctx);
            if (param_text.find("restrict") != std::string::npos ||
                param_text.find("__restrict") != std::string::npos)
                continue;

            std::string restrict_kw = is_c ? "restrict " : "__restrict ";

            if (!dry_run) {
                // Insert __restrict before the parameter name
                auto name_loc = param->getLocation();
                rewriter.InsertTextBefore(name_loc, restrict_kw);
            }
            restricted++;
        }

        if (restricted == 0) return RuleResult::skip("no eligible pointer params");

        return RuleResult::success(0.7, SafetyTier::AGGRESSIVE,
                                    "added restrict to " + std::to_string(restricted) + " param(s)");
    }
};

void register_restrict_qualifier_rule(PatchRuleRegistry& reg) {
    reg.register_rule(std::make_unique<RestrictQualifierRule>());
}

} // namespace core
} // namespace optiweave
