#include <optiweave/core/fix_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <clang/Lex/Lexer.h>

namespace optiweave {
namespace core {

class UnusedVarRule : public FixRule {
public:
    std::string id() const override { return "unused-var"; }
    std::string display_name() const override { return "Unused Variable Void Cast"; }
    std::string description() const override {
        return "Inserts (void)var; cast after unused variable declarations";
    }
    SafetyTier safety_tier() const override { return SafetyTier::SAFE; }

    std::vector<ASTNodeKind> subscribed_nodes() const override {
        return {ASTNodeKind::VarDecl};
    }

    bool has_legacy_kind() const override { return true; }
    analysis::BugFixKind mapped_bug_fix_kind() const override {
        return analysis::BugFixKind::UnusedVariable;
    }

    RuleResult try_apply(clang::VarDecl* decl,
                          const analysis::BugFixIssue& issue,
                          clang::Rewriter& rewriter,
                          clang::ASTContext& ctx,
                          bool is_c, bool dry_run) override {
        (void)is_c;
        std::string var_name = decl->getNameAsString();
        std::string indent = rule_utils::getIndentation(decl->getBeginLoc(), ctx);
        std::string void_cast = "\n" + indent + "(void)" + var_name + ";";

        if (!dry_run) {
            auto end_loc = decl->getSourceRange().getEnd();
            auto after_end = clang::Lexer::getLocForEndOfToken(
                end_loc, 0, ctx.getSourceManager(), ctx.getLangOpts());

            auto& sm = ctx.getSourceManager();
            auto file_id = sm.getFileID(sm.getSpellingLoc(after_end));
            bool invalid = false;
            auto buf = sm.getBufferData(file_id, &invalid);
            if (!invalid && !buf.empty()) {
                unsigned offset = sm.getFileOffset(sm.getSpellingLoc(after_end));
                while (offset < buf.size() && buf[offset] != ';')
                    offset++;
                if (offset < buf.size() && buf[offset] == ';') {
                    auto insert_loc = sm.getSpellingLoc(after_end).getLocWithOffset(
                        offset - sm.getFileOffset(sm.getSpellingLoc(after_end)) + 1);
                    rewriter.InsertTextAfter(insert_loc, void_cast);
                }
            }
        }

        auto& sm = ctx.getSourceManager();
        int actual_line = sm.getPresumedLoc(decl->getLocation()).getLine();
        double conf = rule_utils::computeLocationConfidence(actual_line, issue.line);

        return RuleResult::success(conf, SafetyTier::SAFE,
                                    "inserted (void) cast",
                                    var_name, "(void)" + var_name + ";");
    }
};

void register_unused_var_rule(FixRuleRegistry& reg) {
    reg.register_rule(std::make_unique<UnusedVarRule>());
}

} // namespace core
} // namespace optiweave
