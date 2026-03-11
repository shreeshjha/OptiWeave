#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>

namespace optiweave {
namespace core {

class IntegerTruncationRule : public FixRule {
public:
    std::string id() const override { return "integer-truncation"; }
    std::string display_name() const override { return "Integer Truncation Cast"; }
    std::string description() const override {
        return "Adds explicit cast on narrowing integer assignments";
    }
    SafetyTier safety_tier() const override { return SafetyTier::MOSTLY_SAFE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::BinaryOperator, ASTNodeKind::VarDecl};
    }

    bool is_standalone() const override { return true; }

    RuleResult try_apply_standalone(clang::BinaryOperator* op,
                                     clang::Rewriter& rewriter,
                                     clang::ASTContext& ctx,
                                     bool is_c, bool dry_run) override {
        if (op->getOpcode() != clang::BO_Assign) return RuleResult::skip();

        auto lhs_type = op->getLHS()->getType();
        auto rhs_type = op->getRHS()->getType();

        if (!lhs_type->isIntegerType() || !rhs_type->isIntegerType())
            return RuleResult::skip();

        // Check if RHS type is wider than LHS (narrowing)
        unsigned lhs_width = ctx.getTypeSize(lhs_type);
        unsigned rhs_width = ctx.getTypeSize(rhs_type);

        if (rhs_width <= lhs_width) return RuleResult::skip();

        // Skip if RHS is already a cast expression
        if (llvm::isa<clang::CStyleCastExpr>(op->getRHS()->IgnoreParens()) ||
            llvm::isa<clang::CXXStaticCastExpr>(op->getRHS()->IgnoreParens()))
            return RuleResult::skip();

        // Skip integer literals (compiler handles those)
        if (llvm::isa<clang::IntegerLiteral>(op->getRHS()->IgnoreParenImpCasts()))
            return RuleResult::skip();

        std::string rhs_text = rule_utils::getSourceText(op->getRHS()->getSourceRange(), ctx);
        if (rhs_text.empty()) return RuleResult::skip();

        std::string lhs_type_str = lhs_type.getAsString();
        std::string cast_expr;
        if (is_c)
            cast_expr = "((" + lhs_type_str + ")(" + rhs_text + "))";
        else
            cast_expr = "static_cast<" + lhs_type_str + ">(" + rhs_text + ")";

        if (!dry_run)
            rewriter.ReplaceText(op->getRHS()->getSourceRange(), cast_expr);

        return RuleResult::success(0.85, SafetyTier::MOSTLY_SAFE,
                                    "added explicit narrowing cast to " + lhs_type_str,
                                    rhs_text, cast_expr);
    }

    RuleResult try_apply_standalone(clang::VarDecl* decl,
                                     clang::Rewriter& rewriter,
                                     clang::ASTContext& ctx,
                                     bool is_c, bool dry_run) override {
        if (!decl->hasInit()) return RuleResult::skip();

        auto var_type = decl->getType();
        auto* init = decl->getInit()->IgnoreParenImpCasts();
        auto init_type = init->getType();

        if (!var_type->isIntegerType() || !init_type->isIntegerType())
            return RuleResult::skip();

        unsigned var_width = ctx.getTypeSize(var_type);
        unsigned init_width = ctx.getTypeSize(init_type);

        if (init_width <= var_width) return RuleResult::skip();

        if (llvm::isa<clang::CStyleCastExpr>(init) ||
            llvm::isa<clang::CXXStaticCastExpr>(init) ||
            llvm::isa<clang::IntegerLiteral>(init))
            return RuleResult::skip();

        std::string init_text = rule_utils::getSourceText(
            decl->getInit()->getSourceRange(), ctx);
        if (init_text.empty()) return RuleResult::skip();

        std::string var_type_str = var_type.getAsString();
        std::string cast_expr;
        if (is_c)
            cast_expr = "((" + var_type_str + ")(" + init_text + "))";
        else
            cast_expr = "static_cast<" + var_type_str + ">(" + init_text + ")";

        if (!dry_run)
            rewriter.ReplaceText(decl->getInit()->getSourceRange(), cast_expr);

        return RuleResult::success(0.85, SafetyTier::MOSTLY_SAFE,
                                    "added explicit narrowing cast to " + var_type_str,
                                    init_text, cast_expr);
    }
};

void register_integer_truncation_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<IntegerTruncationRule>());
}

} // namespace core
} // namespace optiweave
