#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <clang/Lex/Lexer.h>

namespace optiweave {
namespace core {

class DanglingElseRule : public FixRule {
public:
    std::string id() const override { return "dangling-else"; }
    std::string display_name() const override { return "Dangling Else Braces"; }
    std::string description() const override {
        return "Wraps single-statement if/else bodies in braces to prevent dangling else";
    }
    SafetyTier safety_tier() const override { return SafetyTier::SAFE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::IfStmt};
    }

    bool is_standalone() const override { return true; }

    RuleResult try_apply_standalone(clang::IfStmt* stmt,
                                     clang::Rewriter& rewriter,
                                     clang::ASTContext& ctx,
                                     bool is_c, bool dry_run) override {
        (void)is_c;
        int braces_added = 0;

        // Check "then" branch
        auto* then_body = stmt->getThen();
        if (then_body && !llvm::isa<clang::CompoundStmt>(then_body) &&
            !llvm::isa<clang::NullStmt>(then_body)) {
            if (!dry_run) {
                wrapInBraces(then_body, rewriter, ctx);
            }
            braces_added++;
        }

        // Check "else" branch
        auto* else_body = stmt->getElse();
        if (else_body && !llvm::isa<clang::CompoundStmt>(else_body) &&
            !llvm::isa<clang::IfStmt>(else_body) &&  // else if is OK
            !llvm::isa<clang::NullStmt>(else_body)) {
            if (!dry_run) {
                wrapInBraces(else_body, rewriter, ctx);
            }
            braces_added++;
        }

        if (braces_added == 0) return RuleResult::skip("already has braces");

        return RuleResult::success(1.0, SafetyTier::SAFE,
                                    "wrapped " + std::to_string(braces_added) + " body(ies) in braces");
    }

private:
    void wrapInBraces(clang::Stmt* body, clang::Rewriter& rewriter,
                       clang::ASTContext& ctx) {
        auto& sm = ctx.getSourceManager();
        auto begin = body->getBeginLoc();
        auto end = clang::Lexer::getLocForEndOfToken(
            body->getEndLoc(), 0, sm, ctx.getLangOpts());

        // Find the semicolon after the statement
        bool invalid = false;
        auto file_id = sm.getFileID(sm.getSpellingLoc(end));
        auto buf = sm.getBufferData(file_id, &invalid);
        if (!invalid && !buf.empty()) {
            unsigned offset = sm.getFileOffset(sm.getSpellingLoc(end));
            while (offset < buf.size() && (buf[offset] == ' ' || buf[offset] == '\t'))
                offset++;
            if (offset < buf.size() && buf[offset] == ';') {
                end = sm.getSpellingLoc(end).getLocWithOffset(
                    offset - sm.getFileOffset(sm.getSpellingLoc(end)) + 1);
            }
        }

        rewriter.InsertTextBefore(begin, "{ ");
        rewriter.InsertTextAfter(end, " }");
    }
};

void register_dangling_else_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<DanglingElseRule>());
}

} // namespace core
} // namespace optiweave
