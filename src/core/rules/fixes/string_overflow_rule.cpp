#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>

namespace optiweave {
namespace core {

class StringOverflowRule : public FixRule {
public:
    std::string id() const override { return "string-overflow"; }
    std::string display_name() const override { return "String Overflow Prevention"; }
    std::string description() const override {
        return "Replaces unsafe string functions: strcpy->strncpy, sprintf->snprintf, strcat->strncat";
    }
    SafetyTier safety_tier() const override { return SafetyTier::MOSTLY_SAFE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::CallExpr};
    }

    bool is_standalone() const override { return true; }

    RuleResult try_apply_standalone(clang::CallExpr* call,
                                     clang::Rewriter& rewriter,
                                     clang::ASTContext& ctx,
                                     bool is_c, bool dry_run) override {
        (void)is_c;
        auto* callee = call->getDirectCallee();
        if (!callee) return RuleResult::skip();

        std::string func_name = callee->getNameAsString();

        if (func_name == "strcpy" && call->getNumArgs() >= 2) {
            std::string dst = rule_utils::getSourceText(call->getArg(0)->getSourceRange(), ctx);
            std::string src = rule_utils::getSourceText(call->getArg(1)->getSourceRange(), ctx);
            if (dst.empty() || src.empty()) return RuleResult::skip();

            std::string replacement = "strncpy(" + dst + ", " + src +
                                      ", sizeof(" + dst + ") - 1)";
            if (!dry_run)
                rewriter.ReplaceText(call->getSourceRange(), replacement);
            return RuleResult::success(0.85, SafetyTier::MOSTLY_SAFE,
                                        "strcpy -> strncpy with sizeof guard",
                                        "strcpy(" + dst + ", " + src + ")", replacement,
                                        "<string.h>");
        }

        if (func_name == "sprintf" && call->getNumArgs() >= 2) {
            std::string dst = rule_utils::getSourceText(call->getArg(0)->getSourceRange(), ctx);
            if (dst.empty()) return RuleResult::skip();

            // Collect remaining args
            std::string args;
            for (unsigned i = 1; i < call->getNumArgs(); i++) {
                if (i > 1) args += ", ";
                args += rule_utils::getSourceText(call->getArg(i)->getSourceRange(), ctx);
            }

            std::string replacement = "snprintf(" + dst + ", sizeof(" + dst + "), " + args + ")";
            if (!dry_run)
                rewriter.ReplaceText(call->getSourceRange(), replacement);
            return RuleResult::success(0.85, SafetyTier::MOSTLY_SAFE,
                                        "sprintf -> snprintf with sizeof guard",
                                        "sprintf(...)", replacement);
        }

        if (func_name == "strcat" && call->getNumArgs() >= 2) {
            std::string dst = rule_utils::getSourceText(call->getArg(0)->getSourceRange(), ctx);
            std::string src = rule_utils::getSourceText(call->getArg(1)->getSourceRange(), ctx);
            if (dst.empty() || src.empty()) return RuleResult::skip();

            std::string replacement = "strncat(" + dst + ", " + src +
                                      ", sizeof(" + dst + ") - strlen(" + dst + ") - 1)";
            if (!dry_run)
                rewriter.ReplaceText(call->getSourceRange(), replacement);
            return RuleResult::success(0.85, SafetyTier::MOSTLY_SAFE,
                                        "strcat -> strncat with sizeof guard",
                                        "strcat(" + dst + ", " + src + ")", replacement,
                                        "<string.h>");
        }

        return RuleResult::skip();
    }
};

void register_string_overflow_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<StringOverflowRule>());
}

} // namespace core
} // namespace optiweave
