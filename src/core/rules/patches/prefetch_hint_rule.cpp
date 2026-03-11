#include <optiweave/core/patch_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <optiweave/analysis/pattern_names.hpp>
#include <clang/AST/Expr.h>

namespace optiweave {
namespace core {

class PrefetchHintRule : public PatchRule {
public:
    std::string id() const override { return "prefetch-hint"; }
    std::string display_name() const override { return "Prefetch Hint"; }
    std::string description() const override {
        return "Inserts __builtin_prefetch for strided memory accesses";
    }
    SafetyTier safety_tier() const override { return SafetyTier::SAFE; }

    std::vector<std::string_view> handled_patterns() const override {
        return {analysis::patterns::kPrefetchHint};
    }

    RuleResult try_apply_loop(clang::SourceLocation keyword_loc,
                               clang::Stmt* body,
                               const analysis::OptimizationPattern& pat,
                               clang::Rewriter& rewriter,
                               clang::ASTContext& ctx,
                               const std::vector<std::string>& induction_vars,
                               bool is_c, bool dry_run,
                               int& recip_counter) override {
        (void)induction_vars; (void)is_c; (void)recip_counter;

        if (!body) return RuleResult::skip("no body");

        // Find array subscript expressions in the loop body
        auto subscripts = rule_utils::collectArraySubscripts(body);
        if (subscripts.empty()) {
            // Fallback to advisory comment
            std::string indent = rule_utils::getIndentation(keyword_loc, ctx);
            std::string comment = indent + "// OPTIWEAVE: Consider __builtin_prefetch for strided access\n";
            if (!dry_run) {
                rewriter.InsertTextBefore(keyword_loc, comment);
            }
            return RuleResult::success(0.6, SafetyTier::SAFE,
                                        "added prefetch hint advisory (no array subscripts found)");
        }

        // Use the first array subscript
        auto* first_sub = subscripts[0];
        std::string base_text = rule_utils::getSourceText(
            first_sub->getBase()->getSourceRange(), ctx);
        std::string idx_text = rule_utils::getSourceText(
            first_sub->getIdx()->getSourceRange(), ctx);

        std::string indent = rule_utils::getIndentation(keyword_loc, ctx);
        std::string prefetch_stmt;

        // Insert prefetch before the loop body's first statement
        if (auto* compound = llvm::dyn_cast<clang::CompoundStmt>(body)) {
            if (compound->body_empty()) return RuleResult::skip("empty body");
            auto first_stmt_loc = (*compound->body_begin())->getBeginLoc();
            std::string body_indent = rule_utils::getIndentation(first_stmt_loc, ctx);
            prefetch_stmt = body_indent + "__builtin_prefetch(&" + base_text +
                            "[" + idx_text + " + 8], 0, 3);\n";

            if (!dry_run) {
                rewriter.InsertTextBefore(first_stmt_loc, prefetch_stmt);
            }
        } else {
            // Single-statement body — insert before the loop
            prefetch_stmt = indent + "// OPTIWEAVE: __builtin_prefetch(&" + base_text +
                            "[" + idx_text + " + 8], 0, 3);\n";
            if (!dry_run) {
                rewriter.InsertTextBefore(keyword_loc, prefetch_stmt);
            }
            return RuleResult::success(0.65, SafetyTier::SAFE,
                                        "added prefetch advisory (single-statement body)");
        }

        return RuleResult::success(0.8, SafetyTier::SAFE,
                                    "inserted __builtin_prefetch for " + base_text +
                                    "[" + idx_text + " + 8]");
    }
};

void register_prefetch_hint_rule(PatchRuleRegistry& reg) {
    reg.register_rule(std::make_unique<PrefetchHintRule>());
}

} // namespace core
} // namespace optiweave
