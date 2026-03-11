#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>

namespace optiweave {
namespace core {

class DivisionByZeroRule : public FixRule {
public:
    std::string id() const override { return "division-by-zero"; }
    std::string display_name() const override { return "Division-by-Zero Guard"; }
    std::string description() const override {
        return "Guards division against zero divisor: a / b -> (b != 0 ? a / b : 0)";
    }
    SafetyTier safety_tier() const override { return SafetyTier::MOSTLY_SAFE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::BinaryOperator};
    }

    bool is_standalone() const override { return true; }

    RuleResult try_apply_standalone(clang::BinaryOperator* op,
                                     clang::Rewriter& rewriter,
                                     clang::ASTContext& ctx,
                                     bool is_c, bool dry_run) override {
        (void)is_c;
        if (op->getOpcode() != clang::BO_Div && op->getOpcode() != clang::BO_Rem)
            return RuleResult::skip();

        // Only guard integer division (FP division by zero is well-defined as inf/nan)
        if (!op->getLHS()->getType()->isIntegerType())
            return RuleResult::skip();

        std::string lhs = rule_utils::getSourceText(op->getLHS()->getSourceRange(), ctx);
        std::string rhs = rule_utils::getSourceText(op->getRHS()->getSourceRange(), ctx);
        if (lhs.empty() || rhs.empty()) return RuleResult::skip();

        // Skip if divisor is a literal (compiler already handles this)
        if (llvm::isa<clang::IntegerLiteral>(op->getRHS()->IgnoreParenImpCasts()))
            return RuleResult::skip();

        std::string op_str = (op->getOpcode() == clang::BO_Div) ? "/" : "%";
        std::string replacement = "((" + rhs + ") != 0 ? (" + lhs + ") " +
                                  op_str + " (" + rhs + ") : 0)";

        if (!dry_run)
            rewriter.ReplaceText(op->getSourceRange(), replacement);

        return RuleResult::success(0.9, SafetyTier::MOSTLY_SAFE,
                                    "added zero-divisor guard",
                                    lhs + " " + op_str + " " + rhs, replacement);
    }
};

void register_division_by_zero_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<DivisionByZeroRule>());
}

} // namespace core
} // namespace optiweave
