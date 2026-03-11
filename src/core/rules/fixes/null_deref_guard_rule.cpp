#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <clang/AST/DeclCXX.h>
#include <clang/Lex/Lexer.h>

namespace optiweave {
namespace core {

class NullDerefGuardRule : public FixRule {
public:
    std::string id() const override { return "null-deref-guard"; }
    std::string display_name() const override { return "Null Dereference Guard"; }
    std::string description() const override {
        return "Inserts null check for pointer parameters at function entry";
    }
    SafetyTier safety_tier() const override { return SafetyTier::MOSTLY_SAFE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::VarDecl}; // We subscribe to VarDecl but use standalone on params
    }

    bool is_standalone() const override { return true; }

    RuleResult try_apply_standalone(clang::VarDecl* decl,
                                     clang::Rewriter& rewriter,
                                     clang::ASTContext& ctx,
                                     bool is_c, bool dry_run) override {
        // Only operate on ParmVarDecl (function parameters)
        auto* param = llvm::dyn_cast<clang::ParmVarDecl>(decl);
        if (!param) return RuleResult::skip();
        if (!param->getType()->isPointerType()) return RuleResult::skip();

        // Get the parent function
        auto* dc = param->getDeclContext();
        auto* func = dc ? llvm::dyn_cast<clang::FunctionDecl>(dc) : nullptr;
        if (!func || !func->hasBody()) return RuleResult::skip();

        // Skip if function is a callback / short function (< 3 stmts)
        auto* body = llvm::dyn_cast<clang::CompoundStmt>(func->getBody());
        if (!body) return RuleResult::skip();
        if (std::distance(body->body_begin(), body->body_end()) < 2)
            return RuleResult::skip();

        // Check if function already has a null check for this param at the start
        // (simple heuristic: first statement is an if checking this param)
        if (body->body_begin() != body->body_end()) {
            if (auto* first_if = llvm::dyn_cast<clang::IfStmt>(*body->body_begin())) {
                std::string cond_text = rule_utils::getSourceText(
                    first_if->getCond()->getSourceRange(), ctx);
                if (cond_text.find(param->getNameAsString()) != std::string::npos &&
                    cond_text.find("!") != std::string::npos)
                    return RuleResult::skip("null check already exists");
            }
        }

        // Only guard the first pointer param to avoid cluttering
        bool is_first_ptr_param = true;
        for (unsigned i = 0; i < func->getNumParams(); i++) {
            if (func->getParamDecl(i) == param) break;
            if (func->getParamDecl(i)->getType()->isPointerType()) {
                is_first_ptr_param = false;
                break;
            }
        }
        if (!is_first_ptr_param) return RuleResult::skip();

        std::string param_name = param->getNameAsString();
        if (param_name.empty()) return RuleResult::skip();

        // Determine return statement based on return type
        std::string ret_stmt;
        auto ret_type = func->getReturnType();
        if (ret_type->isVoidType()) {
            ret_stmt = "return;";
        } else if (ret_type->isPointerType()) {
            ret_stmt = is_c ? "return NULL;" : "return nullptr;";
        } else if (ret_type->isIntegerType()) {
            ret_stmt = "return 0;";
        } else if (ret_type->isBooleanType()) {
            ret_stmt = is_c ? "return 0;" : "return false;";
        } else {
            ret_stmt = "return {};"; // C++ aggregate
        }

        auto body_loc = body->getLBracLoc();
        std::string indent = rule_utils::getIndentation(body_loc, ctx);
        std::string guard = "\n" + indent + "    if (!" + param_name + ") " + ret_stmt;

        if (!dry_run) {
            auto after_brace = clang::Lexer::getLocForEndOfToken(
                body_loc, 0, ctx.getSourceManager(), ctx.getLangOpts());
            rewriter.InsertTextAfter(after_brace, guard);
        }

        return RuleResult::success(0.8, SafetyTier::MOSTLY_SAFE,
                                    "added null guard for '" + param_name + "'");
    }
};

void register_null_deref_guard_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<NullDerefGuardRule>());
}

} // namespace core
} // namespace optiweave
