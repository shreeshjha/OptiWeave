#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>

namespace optiweave {
namespace core {

class FPEqualityRule : public FixRule {
public:
    std::string id() const override { return "fp-equality"; }
    std::string display_name() const override { return "FP Equality Epsilon"; }
    std::string description() const override {
        return "Replaces floating-point == with epsilon comparison: a == b -> fabs(a-b) < eps";
    }
    SafetyTier safety_tier() const override { return SafetyTier::AGGRESSIVE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::BinaryOperator};
    }

    bool has_legacy_kind() const override { return true; }
    analysis::BugFixKind mapped_bug_fix_kind() const override {
        return analysis::BugFixKind::FPEqualityComparison;
    }

    RuleResult try_apply(clang::BinaryOperator* op,
                          const analysis::BugFixIssue& issue,
                          clang::Rewriter& rewriter,
                          clang::ASTContext& ctx,
                          bool is_c, bool dry_run) override {
        if (op->getOpcode() != clang::BO_EQ && op->getOpcode() != clang::BO_NE)
            return RuleResult::skip();

        std::string lhs_text = rule_utils::getSourceText(op->getLHS()->getSourceRange(), ctx);
        std::string rhs_text = rule_utils::getSourceText(op->getRHS()->getSourceRange(), ctx);
        if (lhs_text.empty() || rhs_text.empty())
            return RuleResult::skip("empty operand text");

        clang::QualType lhs_qt = op->getLHS()->getType().getUnqualifiedType();
        clang::QualType rhs_qt = op->getRHS()->getType().getUnqualifiedType();

        auto isFloat = [](clang::QualType qt) {
            auto* bt = qt->getAs<clang::BuiltinType>();
            return bt && bt->getKind() == clang::BuiltinType::Float;
        };
        auto isLongDouble = [](clang::QualType qt) {
            auto* bt = qt->getAs<clang::BuiltinType>();
            return bt && bt->getKind() == clang::BuiltinType::LongDouble;
        };

        std::string epsilon_val, fabs_fn, header;
        if (isFloat(lhs_qt) && isFloat(rhs_qt)) {
            epsilon_val = "1e-6f";
            fabs_fn = is_c ? "fabsf" : "std::fabsf";
            header = is_c ? "<math.h>" : "<cmath>";
        } else if (isLongDouble(lhs_qt) || isLongDouble(rhs_qt)) {
            epsilon_val = "1e-18L";
            fabs_fn = is_c ? "fabsl" : "std::fabsl";
            header = is_c ? "<math.h>" : "<cmath>";
        } else {
            epsilon_val = "1e-9";
            fabs_fn = is_c ? "fabs" : "std::fabs";
            header = is_c ? "<math.h>" : "<cmath>";
        }

        bool is_not_equal = (op->getOpcode() == clang::BO_NE);
        std::string replacement;
        if (is_not_equal)
            replacement = fabs_fn + "((" + lhs_text + ") - (" + rhs_text + ")) >= " + epsilon_val;
        else
            replacement = fabs_fn + "((" + lhs_text + ") - (" + rhs_text + ")) < " + epsilon_val;

        if (!dry_run) {
            rewriter.ReplaceText(op->getSourceRange(), replacement);
        }

        auto& sm = ctx.getSourceManager();
        auto file_id = sm.getFileID(sm.getSpellingLoc(op->getOperatorLoc()));
        rule_utils::injectIncludeIfMissing(file_id, header, rewriter, ctx,
                                            injected_includes_, dry_run);

        int actual_line = sm.getPresumedLoc(op->getOperatorLoc()).getLine();
        double conf = rule_utils::computeLocationConfidence(actual_line, issue.line);

        return RuleResult::success(conf, SafetyTier::AGGRESSIVE,
                                    "replaced with " + fabs_fn + " epsilon=" + epsilon_val,
                                    lhs_text + (is_not_equal ? " != " : " == ") + rhs_text,
                                    replacement, header);
    }

private:
    std::set<std::string> injected_includes_;
};

void register_fp_equality_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<FPEqualityRule>());
}

} // namespace core
} // namespace optiweave
