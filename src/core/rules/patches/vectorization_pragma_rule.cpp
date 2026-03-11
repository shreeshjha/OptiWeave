#include <optiweave/core/patch_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <optiweave/analysis/pattern_names.hpp>

namespace optiweave {
namespace core {

class VectorizationPragmaRule : public PatchRule {
public:
    std::string id() const override { return "vectorization-pragma"; }
    std::string display_name() const override { return "Vectorization Pragma"; }
    std::string description() const override {
        return "Inserts compiler-guarded vectorization pragma before loops";
    }
    SafetyTier safety_tier() const override { return SafetyTier::SAFE; }

    std::vector<std::string_view> handled_patterns() const override {
        return {analysis::patterns::kSIMDVectorization};
    }

    RuleResult try_apply_loop(clang::SourceLocation keyword_loc,
                               clang::Stmt* body,
                               const analysis::OptimizationPattern& pat,
                               clang::Rewriter& rewriter,
                               clang::ASTContext& ctx,
                               const std::vector<std::string>& induction_vars,
                               bool is_c, bool dry_run,
                               int& recip_counter) override {
        (void)body; (void)induction_vars; (void)is_c; (void)recip_counter;

        if (rule_utils::checkExistingPragma(keyword_loc, ctx))
            return RuleResult::skip("pragma already present");

        std::string indent = rule_utils::getIndentation(keyword_loc, ctx);
        std::string pragma =
            indent + "#if defined(__clang__)\n" +
            indent + "#pragma clang loop vectorize(enable)\n" +
            indent + "#elif defined(__GNUC__)\n" +
            indent + "#pragma GCC ivdep\n" +
            indent + "#endif\n";

        if (!dry_run) {
            rewriter.InsertTextBefore(keyword_loc, pragma);
        }

        return RuleResult::success(1.0, SafetyTier::SAFE, "inserted vectorization pragma");
    }
};

void register_vectorization_pragma_rule(PatchRuleRegistry& reg) {
    reg.register_rule(std::make_unique<VectorizationPragmaRule>());
}

} // namespace core
} // namespace optiweave
