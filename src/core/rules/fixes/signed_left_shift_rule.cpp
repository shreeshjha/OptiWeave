#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>

namespace optiweave {
namespace core {

class SignedLeftShiftRule : public FixRule {
public:
    std::string id() const override { return "signed-left-shift"; }
    std::string display_name() const override { return "Signed Left Shift Cast"; }
    std::string description() const override {
        return "Casts LHS of signed left shift to unsigned: x << n -> ((unsigned)(x)) << n";
    }
    SafetyTier safety_tier() const override { return SafetyTier::MOSTLY_SAFE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::BinaryOperator};
    }

    bool has_legacy_kind() const override { return true; }
    analysis::BugFixKind mapped_bug_fix_kind() const override {
        return analysis::BugFixKind::SignedLeftShift;
    }

    RuleResult try_apply(clang::BinaryOperator* op,
                          const analysis::BugFixIssue& issue,
                          clang::Rewriter& rewriter,
                          clang::ASTContext& ctx,
                          bool is_c, bool dry_run) override {
        if (op->getOpcode() != clang::BO_Shl)
            return RuleResult::skip();

        std::string lhs_text = rule_utils::getSourceText(op->getLHS()->getSourceRange(), ctx);
        if (lhs_text.empty()) return RuleResult::skip("empty LHS text");

        std::string cast_expr;
        if (is_c)
            cast_expr = "((unsigned)(" + lhs_text + "))";
        else
            cast_expr = "(static_cast<unsigned>(" + lhs_text + "))";

        if (!dry_run) {
            rewriter.ReplaceText(op->getLHS()->getSourceRange(), cast_expr);
        }

        auto& sm = ctx.getSourceManager();
        int actual_line = sm.getPresumedLoc(op->getOperatorLoc()).getLine();
        double conf = rule_utils::computeLocationConfidence(actual_line, issue.line);

        return RuleResult::success(conf, SafetyTier::MOSTLY_SAFE,
                                    "cast LHS to unsigned",
                                    lhs_text, cast_expr);
    }
};

void register_signed_left_shift_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<SignedLeftShiftRule>());
}

} // namespace core
} // namespace optiweave
