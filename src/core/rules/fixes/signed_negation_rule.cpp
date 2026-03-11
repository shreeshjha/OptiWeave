#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>

namespace optiweave {
namespace core {

class SignedNegationRule : public FixRule {
public:
    std::string id() const override { return "signed-negation"; }
    std::string display_name() const override { return "Signed Negation Overflow Guard"; }
    std::string description() const override {
        return "Guards signed negation against MIN overflow: -x -> ((x) == MIN ? MAX : -(x))";
    }
    SafetyTier safety_tier() const override { return SafetyTier::MOSTLY_SAFE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::UnaryOperator};
    }

    bool has_legacy_kind() const override { return true; }
    analysis::BugFixKind mapped_bug_fix_kind() const override {
        return analysis::BugFixKind::SignedNegationOverflow;
    }

    RuleResult try_apply(clang::UnaryOperator* op,
                          const analysis::BugFixIssue& issue,
                          clang::Rewriter& rewriter,
                          clang::ASTContext& ctx,
                          bool is_c, bool dry_run) override {
        if (op->getOpcode() != clang::UO_Minus)
            return RuleResult::skip();

        auto* sub_expr = op->getSubExpr();
        if (!sub_expr) return RuleResult::skip("no subexpr");

        std::string expr_text = rule_utils::getSourceText(sub_expr->getSourceRange(), ctx);
        if (expr_text.empty()) return RuleResult::skip("empty expr text");

        // Detect type for correct MIN/MAX macros
        const clang::BuiltinType* bt =
            sub_expr->getType().getUnqualifiedType().getCanonicalType()
                               ->getAs<clang::BuiltinType>();

        std::string min_macro = "INT_MIN";
        std::string max_macro = "INT_MAX";
        if (bt) {
            switch (bt->getKind()) {
            case clang::BuiltinType::SChar:
            case clang::BuiltinType::Char_S:
                min_macro = "SCHAR_MIN"; max_macro = "SCHAR_MAX"; break;
            case clang::BuiltinType::Short:
                min_macro = "SHRT_MIN";  max_macro = "SHRT_MAX";  break;
            case clang::BuiltinType::Int:
                min_macro = "INT_MIN";   max_macro = "INT_MAX";   break;
            case clang::BuiltinType::Long:
                min_macro = "LONG_MIN";  max_macro = "LONG_MAX";  break;
            case clang::BuiltinType::LongLong:
                min_macro = "LLONG_MIN"; max_macro = "LLONG_MAX"; break;
            default: break;
            }
        }

        std::string replacement = "((" + expr_text + ") == " + min_macro +
                                  " ? " + max_macro + " : -(" + expr_text + "))";
        std::string header = is_c ? "<limits.h>" : "<climits>";

        if (!dry_run) {
            rewriter.ReplaceText(op->getSourceRange(), replacement);
        }
        // Inject header (in both dry_run and real mode for correct reporting)
        auto& sm = ctx.getSourceManager();
        auto file_id = sm.getFileID(sm.getSpellingLoc(op->getOperatorLoc()));
        rule_utils::injectIncludeIfMissing(file_id, header, rewriter, ctx,
                                            injected_includes_, dry_run);

        int actual_line = sm.getPresumedLoc(op->getOperatorLoc()).getLine();
        double conf = rule_utils::computeLocationConfidence(actual_line, issue.line);

        return RuleResult::success(conf, SafetyTier::MOSTLY_SAFE,
                                    "added " + min_macro + " guard",
                                    "-(" + expr_text + ")", replacement, header);
    }

private:
    std::set<std::string> injected_includes_;
};

void register_signed_negation_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<SignedNegationRule>());
}

} // namespace core
} // namespace optiweave
