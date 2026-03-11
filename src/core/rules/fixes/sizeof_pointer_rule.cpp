#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>

namespace optiweave {
namespace core {

class SizeofPointerRule : public FixRule {
public:
    std::string id() const override { return "sizeof-pointer"; }
    std::string display_name() const override { return "Sizeof Pointer Fix"; }
    std::string description() const override {
        return "Fixes malloc(sizeof(ptr)) to malloc(sizeof(*ptr))";
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
        if (func_name != "malloc" && func_name != "calloc" && func_name != "realloc")
            return RuleResult::skip();

        // Look for sizeof(pointer_expr) in arguments
        for (unsigned i = 0; i < call->getNumArgs(); i++) {
            auto* arg = call->getArg(i)->IgnoreParenImpCasts();
            auto* sizeof_expr = llvm::dyn_cast<clang::UnaryExprOrTypeTraitExpr>(arg);
            if (!sizeof_expr) continue;
            if (sizeof_expr->getKind() != clang::UETT_SizeOf) continue;

            if (sizeof_expr->isArgumentType()) {
                // sizeof(type) — check if the type is a pointer type
                auto arg_type = sizeof_expr->getArgumentType();
                if (arg_type->isPointerType()) {
                    // sizeof(T*) should be sizeof(T) or sizeof(*ptr)
                    auto pointee = arg_type->getPointeeType();
                    std::string original = rule_utils::getSourceText(
                        sizeof_expr->getSourceRange(), ctx);
                    std::string replacement = "sizeof(" + pointee.getAsString() + ")";

                    if (!dry_run)
                        rewriter.ReplaceText(sizeof_expr->getSourceRange(), replacement);

                    return RuleResult::success(0.9, SafetyTier::MOSTLY_SAFE,
                                                "sizeof(ptr_type) -> sizeof(pointee_type)",
                                                original, replacement);
                }
            } else {
                // sizeof(expr) — check if expr is a pointer
                auto* arg_expr = sizeof_expr->getArgumentExpr();
                if (arg_expr && arg_expr->getType()->isPointerType()) {
                    std::string expr_text = rule_utils::getSourceText(
                        arg_expr->getSourceRange(), ctx);
                    std::string original = rule_utils::getSourceText(
                        sizeof_expr->getSourceRange(), ctx);
                    std::string replacement = "sizeof(*(" + expr_text + "))";

                    if (!dry_run)
                        rewriter.ReplaceText(sizeof_expr->getSourceRange(), replacement);

                    return RuleResult::success(0.9, SafetyTier::MOSTLY_SAFE,
                                                "sizeof(ptr) -> sizeof(*ptr)",
                                                original, replacement);
                }
            }
        }

        return RuleResult::skip();
    }
};

void register_sizeof_pointer_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<SizeofPointerRule>());
}

} // namespace core
} // namespace optiweave
