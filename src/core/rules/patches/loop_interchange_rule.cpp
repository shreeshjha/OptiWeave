#include <optiweave/core/patch_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <optiweave/analysis/pattern_names.hpp>

namespace optiweave {
namespace core {

class LoopInterchangeRule : public PatchRule {
public:
    std::string id() const override { return "loop-interchange"; }
    std::string display_name() const override { return "Loop Interchange"; }
    std::string description() const override {
        return "Swaps nested loop headers to improve cache locality";
    }
    SafetyTier safety_tier() const override { return SafetyTier::AGGRESSIVE; }

    std::vector<std::string_view> handled_patterns() const override {
        return {analysis::patterns::kLoopInterchange};
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

        // Find the inner for-loop
        clang::ForStmt* inner_for = nullptr;
        clang::CompoundStmt* compound = llvm::dyn_cast<clang::CompoundStmt>(body);
        if (compound) {
            for (auto* child : compound->body()) {
                if (auto* f = llvm::dyn_cast<clang::ForStmt>(child)) {
                    inner_for = f;
                    break;
                }
            }
        } else {
            inner_for = llvm::dyn_cast<clang::ForStmt>(body);
        }

        if (!inner_for)
            return RuleResult::skip("no inner for-loop found");

        // Get the full for(...) header text for outer and inner loops
        // Outer: from keyword_loc to the body start
        auto outer_body_begin = body->getBeginLoc();
        std::string outer_header = rule_utils::getSourceText(
            clang::SourceRange(keyword_loc, outer_body_begin), ctx);

        auto inner_body = inner_for->getBody();
        if (!inner_body)
            return RuleResult::skip("inner loop has no body");

        auto inner_body_begin = inner_body->getBeginLoc();
        std::string inner_header = rule_utils::getSourceText(
            clang::SourceRange(inner_for->getForLoc(), inner_body_begin), ctx);

        // Validate we got reasonable header text
        if (outer_header.empty() || inner_header.empty())
            return RuleResult::skip("could not extract loop headers");

        // Swap the loop headers
        if (!dry_run) {
            auto outer_range = clang::SourceRange(keyword_loc, outer_body_begin);
            auto inner_range = clang::SourceRange(inner_for->getForLoc(), inner_body_begin);

            rewriter.ReplaceText(clang::CharSourceRange::getTokenRange(outer_range),
                                  inner_header);
            rewriter.ReplaceText(clang::CharSourceRange::getTokenRange(inner_range),
                                  outer_header);
        }

        return RuleResult::success(0.7, SafetyTier::AGGRESSIVE,
                                    "swapped outer/inner loop headers for cache locality");
    }
};

void register_loop_interchange_rule(PatchRuleRegistry& reg) {
    reg.register_rule(std::make_unique<LoopInterchangeRule>());
}

} // namespace core
} // namespace optiweave
