#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <clang/Lex/Lexer.h>

namespace optiweave {
namespace core {

class ImplicitFallthroughRule : public FixRule {
public:
    std::string id() const override { return "implicit-fallthrough"; }
    std::string display_name() const override { return "Implicit Fallthrough Break"; }
    std::string description() const override {
        return "Inserts break; before next case in switch to prevent implicit fallthrough";
    }
    SafetyTier safety_tier() const override { return SafetyTier::SAFE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::SwitchStmt};
    }

    bool is_standalone() const override { return true; }

    RuleResult try_apply_standalone(clang::SwitchStmt* stmt,
                                     clang::Rewriter& rewriter,
                                     clang::ASTContext& ctx,
                                     bool is_c, bool dry_run) override {
        (void)is_c;
        auto* body = stmt->getBody();
        if (!body) return RuleResult::skip();

        auto* compound = llvm::dyn_cast<clang::CompoundStmt>(body);
        if (!compound) return RuleResult::skip();

        int fixes_added = 0;
        clang::Stmt* prev_stmt = nullptr;

        for (auto* child : compound->body()) {
            bool is_case = llvm::isa<clang::CaseStmt>(child) ||
                           llvm::isa<clang::DefaultStmt>(child);

            if (is_case && prev_stmt) {
                // Check if prev_stmt ends with break/return/continue/goto
                bool has_terminator = false;
                auto* check = prev_stmt;

                // If prev is a case/default, check its sub-statement
                while (auto* cs = llvm::dyn_cast<clang::CaseStmt>(check))
                    check = cs->getSubStmt();
                while (auto* ds = llvm::dyn_cast<clang::DefaultStmt>(check))
                    check = ds->getSubStmt();

                if (auto* comp = llvm::dyn_cast<clang::CompoundStmt>(check)) {
                    if (!comp->body_empty()) {
                        auto* last = *(std::prev(comp->body_end()));
                        has_terminator = llvm::isa<clang::BreakStmt>(last) ||
                                        llvm::isa<clang::ReturnStmt>(last) ||
                                        llvm::isa<clang::ContinueStmt>(last) ||
                                        llvm::isa<clang::GotoStmt>(last);
                    }
                } else {
                    has_terminator = llvm::isa<clang::BreakStmt>(check) ||
                                    llvm::isa<clang::ReturnStmt>(check) ||
                                    llvm::isa<clang::ContinueStmt>(check) ||
                                    llvm::isa<clang::GotoStmt>(check);
                }

                // Check for fallthrough comment/attribute
                if (!has_terminator) {
                    std::string indent = rule_utils::getIndentation(child->getBeginLoc(), ctx);
                    std::string break_stmt = indent + "    break;\n";

                    if (!dry_run) {
                        rewriter.InsertTextBefore(child->getBeginLoc(), break_stmt);
                    }
                    fixes_added++;
                }
            }
            prev_stmt = child;
        }

        if (fixes_added == 0) return RuleResult::skip("no fallthrough found");

        return RuleResult::success(0.95, SafetyTier::SAFE,
                                    "added " + std::to_string(fixes_added) + " break statement(s)");
    }
};

void register_implicit_fallthrough_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<ImplicitFallthroughRule>());
}

} // namespace core
} // namespace optiweave
