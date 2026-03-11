#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ParentMapContext.h>
#include <clang/Lex/Lexer.h>

namespace optiweave {
namespace core {

class UninitializedVarRule : public FixRule {
public:
    std::string id() const override { return "uninitialized-var"; }
    std::string display_name() const override { return "Uninitialized Variable Initializer"; }
    std::string description() const override {
        return "Adds zero-initializer to uninitialized local variables";
    }
    SafetyTier safety_tier() const override { return SafetyTier::SAFE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::VarDecl};
    }

    bool has_legacy_kind() const override { return true; }
    analysis::BugFixKind mapped_bug_fix_kind() const override {
        return analysis::BugFixKind::UninitializedVariable;
    }

    RuleResult try_apply(clang::VarDecl* decl,
                          const analysis::BugFixIssue& issue,
                          clang::Rewriter& rewriter,
                          clang::ASTContext& ctx,
                          bool is_c, bool dry_run) override {
        if (decl->hasInit()) return RuleResult::skip("already initialized");
        if (decl->isLocalExternDecl()) return RuleResult::skip("extern decl");

        // Skip multi-variable declarations
        auto parents = ctx.getParents(*decl);
        if (!parents.empty()) {
            if (auto* decl_stmt = parents[0].get<clang::DeclStmt>()) {
                if (!decl_stmt->isSingleDecl())
                    return RuleResult::skip("multi-variable declaration");
            }
        }

        std::string initializer;
        if (decl->getType()->isPointerType())
            initializer = is_c ? " = NULL" : " = nullptr";
        else if (decl->getType()->isFloatingType())
            initializer = " = 0.0";
        else if (decl->getType()->isBooleanType())
            initializer = is_c ? " = 0" : " = false";
        else if (decl->getType()->isIntegerType() || decl->getType()->isEnumeralType())
            initializer = " = 0";
        else
            initializer = is_c ? " = {0}" : " = {}";

        if (!dry_run) {
            auto end_loc = decl->getSourceRange().getEnd();
            auto after_name = clang::Lexer::getLocForEndOfToken(
                end_loc, 0, ctx.getSourceManager(), ctx.getLangOpts());
            rewriter.InsertTextAfter(after_name, initializer);
        }

        auto& sm = ctx.getSourceManager();
        int actual_line = sm.getPresumedLoc(decl->getLocation()).getLine();
        double conf = rule_utils::computeLocationConfidence(actual_line, issue.line);

        return RuleResult::success(conf, SafetyTier::SAFE,
                                    "added '" + initializer + "'",
                                    issue.var_name, issue.var_name + initializer);
    }
};

void register_uninitialized_var_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<UninitializedVarRule>());
}

} // namespace core
} // namespace optiweave
