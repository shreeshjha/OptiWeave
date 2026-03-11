#include <optiweave/core/patch_visitor.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <optiweave/analysis/pattern_names.hpp>
#include <clang/AST/Stmt.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace optiweave {
namespace core {

PatchVisitor::PatchVisitor(clang::Rewriter& rewriter,
                           clang::ASTContext& context,
                           const std::vector<analysis::OptimizationPattern>& suggestions,
                           bool dry_run,
                           bool is_c_language,
                           const RuleConfig& config,
                           bool verbose_rules)
    : rewriter_(rewriter), context_(context), suggestions_(suggestions),
      dry_run_(dry_run), is_c_language_(is_c_language), config_(config),
      verbose_rules_(verbose_rules) {}

// --- Location matching ---

bool PatchVisitor::filesMatch(const std::string& a, const std::string& b) const {
    return rule_utils::filesMatch(a, b);
}

const analysis::OptimizationPattern* PatchVisitor::findMatch(
    clang::SourceLocation loc, const std::string& pattern_name) const {
    if (!loc.isValid()) return nullptr;

    auto& sm = context_.getSourceManager();
    auto presumed = sm.getPresumedLoc(loc);
    if (presumed.isInvalid()) return nullptr;

    std::string file = presumed.getFilename();
    int line = static_cast<int>(presumed.getLine());

    const analysis::OptimizationPattern* best = nullptr;
    int best_delta = 4;

    for (const auto& s : suggestions_) {
        if (s.pattern_name != pattern_name) continue;
        if (!filesMatch(file, s.location.file)) continue;

        int delta = std::abs(line - s.location.line);
        if (delta < best_delta) {
            best_delta = delta;
            best = &s;
        }
    }
    return best;
}

const analysis::OptimizationPattern* PatchVisitor::findMatchByLocation(
    clang::SourceLocation loc) const {
    if (!loc.isValid()) return nullptr;

    auto& sm = context_.getSourceManager();
    auto presumed = sm.getPresumedLoc(loc);
    if (presumed.isInvalid()) return nullptr;

    std::string file = presumed.getFilename();
    int line = static_cast<int>(presumed.getLine());

    const analysis::OptimizationPattern* best = nullptr;
    int best_delta = 4;

    for (const auto& s : suggestions_) {
        if (!filesMatch(file, s.location.file)) continue;

        int delta = std::abs(line - s.location.line);
        if (delta < best_delta) {
            best_delta = delta;
            best = &s;
        }
    }
    return best;
}

// --- Visitor methods ---

bool PatchVisitor::VisitForStmt(clang::ForStmt* stmt) {
    std::string ind_var = rule_utils::getLoopInductionVar(stmt);
    enclosing_induction_vars_.push_back(ind_var);
    handleLoopPatches(stmt->getBody(), stmt->getForLoc(), stmt);
    enclosing_induction_vars_.pop_back();
    return true;
}

bool PatchVisitor::VisitWhileStmt(clang::WhileStmt* stmt) {
    enclosing_induction_vars_.push_back("");
    handleLoopPatches(stmt->getBody(), stmt->getWhileLoc(), stmt);
    enclosing_induction_vars_.pop_back();
    return true;
}

bool PatchVisitor::VisitDoStmt(clang::DoStmt* stmt) {
    enclosing_induction_vars_.push_back("");
    handleLoopPatches(stmt->getBody(), stmt->getDoLoc(), stmt);
    enclosing_induction_vars_.pop_back();
    return true;
}

bool PatchVisitor::VisitBinaryOperator(clang::BinaryOperator* op) {
    // Division patches are handled at the loop level
    return true;
}

bool PatchVisitor::VisitFunctionDecl(clang::FunctionDecl* func) {
    if (!func->hasBody()) return true;
    auto loc = func->getBeginLoc();
    if (rule_utils::isInMacro(loc)) return true;

    auto& reg = PatchRuleRegistry::instance();
    for (auto& rule : reg.all_rules()) {
        if (!rule->is_function_level()) continue;
        if (rule->safety_tier() > config_.max_safety_tier) continue;

        // Try each handled pattern
        for (auto pattern_sv : rule->handled_patterns()) {
            std::string pattern_name(pattern_sv);
            auto* pat = findMatch(func->getBeginLoc(), pattern_name);
            if (!pat) continue;

            std::string dedup_key = pat->location.get_file() + ":" +
                                    std::to_string(pat->location.line) + ":" + rule->id();
            if (applied_locations_.count(dedup_key)) continue;

            auto result = rule->try_apply_function(func, *pat, rewriter_, context_,
                                                     is_c_language_, dry_run_);
            if (result.applied && result.confidence >= config_.min_confidence) {
                applied_++;
                applied_locations_.insert(dedup_key);
                {
                    std::ostringstream oss;
                    std::string prefix = dry_run_ ? "PATCH (dry-run)" : "PATCH";
                    oss << prefix << " [" << safetyTierToString(result.tier)
                        << ", " << std::fixed << std::setprecision(2) << result.confidence
                        << "]: " << rule->display_name() << " at "
                        << pat->location.get_file() << ":" << pat->location.line
                        << " — " << result.reason;
                    log_.push_back(oss.str());
                }
                if (verbose_rules_) {
                    std::ostringstream voss;
                    voss << "VERBOSE: rule=" << rule->id()
                         << " confidence=" << std::fixed << std::setprecision(2) << result.confidence
                         << " tier=" << safetyTierToString(result.tier)
                         << " applied=yes reason=" << result.reason;
                    log_.push_back(voss.str());
                }
            }
        }
    }
    return true;
}

// --- Shared loop logic (registry dispatch) ---

void PatchVisitor::handleLoopPatches(clang::Stmt* body,
                                      clang::SourceLocation keyword_loc,
                                      clang::Stmt* loop_stmt) {
    if (keyword_loc.isMacroID()) {
        log_.push_back("SKIP: Loop at macro location — cannot safely patch");
        skipped_++;
        return;
    }

    auto& reg = PatchRuleRegistry::instance();

    // Collect all matched patterns for this location
    for (auto& rule : reg.all_rules()) {
        if (rule->is_function_level()) continue;
        if (rule->safety_tier() > config_.max_safety_tier) continue;

        for (auto pattern_sv : rule->handled_patterns()) {
            std::string pattern_name(pattern_sv);
            auto* pat = findMatch(keyword_loc, pattern_name);
            if (!pat) continue;

            std::string dedup_key = pat->location.get_file() + ":" +
                                    std::to_string(pat->location.line) + ":" +
                                    pattern_name;
            if (applied_locations_.count(dedup_key)) continue;

            auto result = rule->try_apply_loop(keyword_loc, body, *pat,
                                                rewriter_, context_,
                                                enclosing_induction_vars_,
                                                is_c_language_, dry_run_,
                                                recip_counter_);
            if (result.applied && result.confidence >= config_.min_confidence) {
                applied_locations_.insert(dedup_key);
                if (rule->safety_tier() == SafetyTier::ADVISORY) {
                    advisories_++;
                    std::ostringstream oss;
                    std::string prefix = dry_run_ ? "ADVISORY (dry-run)" : "ADVISORY";
                    oss << prefix << " [" << safetyTierToString(result.tier)
                        << ", " << std::fixed << std::setprecision(2) << result.confidence
                        << "]: " << result.reason << " at "
                        << pat->location.get_file() << ":" << pat->location.line;
                    log_.push_back(oss.str());
                } else {
                    applied_++;
                    std::ostringstream oss;
                    std::string prefix = dry_run_ ? "PATCH (dry-run)" : "PATCH";
                    oss << prefix << " [" << safetyTierToString(result.tier)
                        << ", " << std::fixed << std::setprecision(2) << result.confidence
                        << "]: " << rule->display_name() << " at "
                        << pat->location.get_file() << ":" << pat->location.line
                        << " — " << result.reason;
                    log_.push_back(oss.str());
                }
                if (verbose_rules_) {
                    std::ostringstream voss;
                    voss << "VERBOSE: rule=" << rule->id()
                         << " pattern=" << pattern_name
                         << " confidence=" << std::fixed << std::setprecision(2) << result.confidence
                         << " tier=" << safetyTierToString(result.tier)
                         << " applied=yes reason=" << result.reason;
                    log_.push_back(voss.str());
                }
            } else if (!result.applied && !result.reason.empty()) {
                skipped_++;
                log_.push_back("SKIP: " + rule->display_name() + " at " +
                                pat->location.get_file() + ":" +
                                std::to_string(pat->location.line) +
                                " — " + result.reason);
                if (verbose_rules_) {
                    std::ostringstream voss;
                    voss << "VERBOSE: rule=" << rule->id()
                         << " pattern=" << pattern_name
                         << " applied=no reason=" << result.reason;
                    log_.push_back(voss.str());
                }
            }
        }
    }

    // Fall through to advisory for any remaining pattern match by location
    auto* any_pat = findMatchByLocation(keyword_loc);
    if (any_pat) {
        std::string dedup_key = any_pat->location.get_file() + ":" +
                                std::to_string(any_pat->location.line) + ":" +
                                any_pat->pattern_name;
        if (!applied_locations_.count(dedup_key)) {
            // Try advisory rules for unhandled patterns
            auto advisory_rules = reg.rules_for_pattern(any_pat->pattern_name);
            if (advisory_rules.empty()) {
                // No specific rule, use the generic advisory
                for (auto& rule : reg.all_rules()) {
                    if (rule->safety_tier() != SafetyTier::ADVISORY) continue;
                    // Check if this advisory handles the pattern
                    for (auto sv : rule->handled_patterns()) {
                        if (sv == any_pat->pattern_name) {
                            auto result = rule->try_apply_loop(keyword_loc, body, *any_pat,
                                                                rewriter_, context_,
                                                                enclosing_induction_vars_,
                                                                is_c_language_, dry_run_,
                                                                recip_counter_);
                            if (result.applied) {
                                applied_locations_.insert(dedup_key);
                                advisories_++;
                                std::ostringstream oss;
                                std::string prefix = dry_run_ ? "ADVISORY (dry-run)" : "ADVISORY";
                                oss << prefix << " [" << safetyTierToString(result.tier)
                                    << ", " << std::fixed << std::setprecision(2) << result.confidence
                                    << "]: " << result.reason << " at "
                                    << any_pat->location.get_file() << ":"
                                    << any_pat->location.line;
                                log_.push_back(oss.str());
                            }
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
}

} // namespace core
} // namespace optiweave
