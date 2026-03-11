#include <optiweave/core/patch_rule.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <optiweave/analysis/pattern_names.hpp>

namespace optiweave {
namespace core {

class AdvisoryCommentRule : public PatchRule {
public:
    std::string id() const override { return "advisory-comment"; }
    std::string display_name() const override { return "Advisory Comment"; }
    std::string description() const override {
        return "Inserts human-readable advisory comments for non-patchable patterns";
    }
    SafetyTier safety_tier() const override { return SafetyTier::ADVISORY; }

    std::vector<std::string_view> handled_patterns() const override {
        return {
            analysis::patterns::kNaiveMatrixMultiply,
            analysis::patterns::kQuadraticAlgorithm,
            analysis::patterns::kPoorMemoryLocality,
            analysis::patterns::kRepeatedComputation,
            analysis::patterns::kBranchMisprediction
        };
    }

    RuleResult try_apply_loop(clang::SourceLocation keyword_loc,
                               clang::Stmt* body,
                               const analysis::OptimizationPattern& pat,
                               clang::Rewriter& rewriter,
                               clang::ASTContext& ctx,
                               const std::vector<std::string>& induction_vars,
                               bool is_c, bool dry_run,
                               int& recip_counter) override {
        (void)body; (void)induction_vars; (void)is_c; (void)recip_counter;

        std::string indent = rule_utils::getIndentation(keyword_loc, ctx);
        std::string comment;

        namespace pn = analysis::patterns;
        if (pat.pattern_name == pn::kNaiveMatrixMultiply) {
            comment = indent + "// OPTIWEAVE: " + pat.description +
                      " -- consider optimized libraries (e.g., BLAS)\n";
        } else if (pat.pattern_name == pn::kQuadraticAlgorithm) {
            comment = indent + "// OPTIWEAVE: Quadratic complexity detected" +
                      " -- consider more efficient algorithm\n";
        } else if (pat.pattern_name == pn::kPoorMemoryLocality) {
            comment = indent + "// OPTIWEAVE: Poor memory locality detected" +
                      " -- consider loop interchange or cache blocking\n";
        } else if (pat.pattern_name == pn::kRepeatedComputation) {
            comment = indent + "// OPTIWEAVE: Loop-invariant computation" +
                      " -- consider hoisting outside loop\n";
        } else if (pat.pattern_name == pn::kBranchMisprediction) {
            comment = indent + "// OPTIWEAVE: High branch ratio" +
                      " -- consider branchless alternatives\n";
        } else {
            comment = indent + "// OPTIWEAVE: " + pat.pattern_name +
                      " -- " + pat.description + "\n";
        }

        if (!dry_run) {
            rewriter.InsertTextBefore(keyword_loc, comment);
        }

        return RuleResult::success(1.0, SafetyTier::ADVISORY,
                                    std::string(pat.pattern_name));
    }
};

void register_advisory_comment_rule(PatchRuleRegistry& reg) {
    reg.register_rule(std::make_unique<AdvisoryCommentRule>());
}

} // namespace core
} // namespace optiweave
