#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>

namespace optiweave {
namespace core {

class UnsignedWraparoundRule : public FixRule {
public:
    std::string id() const override { return "unsigned-wraparound"; }
    std::string display_name() const override { return "Unsigned Wraparound Guard"; }
    std::string description() const override {
        return "Guards unsigned subtraction against wraparound: a - b -> ((a) >= (b) ? (a) - (b) : 0)";
    }
    SafetyTier safety_tier() const override { return SafetyTier::MOSTLY_SAFE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::BinaryOperator};
    }

    bool has_legacy_kind() const override { return true; }
    analysis::BugFixKind mapped_bug_fix_kind() const override {
        return analysis::BugFixKind::UnsignedWraparound;
    }

    RuleResult try_apply(clang::BinaryOperator* op,
                          const analysis::BugFixIssue& issue,
                          clang::Rewriter& rewriter,
                          clang::ASTContext& ctx,
                          bool is_c, bool dry_run) override {
        (void)is_c;
        if (op->getOpcode() != clang::BO_Sub)
            return RuleResult::skip();

        std::string lhs_text = rule_utils::getSourceText(op->getLHS()->getSourceRange(), ctx);
        std::string rhs_text = rule_utils::getSourceText(op->getRHS()->getSourceRange(), ctx);
        if (lhs_text.empty() || rhs_text.empty())
            return RuleResult::skip("empty operand text");

        std::string replacement = "((" + lhs_text + ") >= (" + rhs_text +
                                  ") ? (" + lhs_text + ") - (" + rhs_text + ") : 0)";

        if (!dry_run) {
            rewriter.ReplaceText(op->getSourceRange(), replacement);
        }

        auto& sm = ctx.getSourceManager();
        int actual_line = sm.getPresumedLoc(op->getOperatorLoc()).getLine();
        double conf = rule_utils::computeLocationConfidence(actual_line, issue.line);

        return RuleResult::success(conf, SafetyTier::MOSTLY_SAFE,
                                    "guarded subtraction",
                                    lhs_text + " - " + rhs_text, replacement);
    }
};

void register_unsigned_wraparound_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<UnsignedWraparoundRule>());
}

} // namespace core
} // namespace optiweave
