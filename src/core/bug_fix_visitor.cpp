#include <optiweave/core/bug_fix_visitor.hpp>
#include <optiweave/core/rule_registry.hpp>
#include <optiweave/core/rule_utils.hpp>
#include <clang/AST/Stmt.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/ParentMapContext.h>
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

BugFixVisitor::BugFixVisitor(clang::Rewriter& rewriter,
                             clang::ASTContext& context,
                             const std::vector<analysis::BugFixIssue>& issues,
                             bool dry_run,
                             bool is_c_language,
                             std::set<analysis::BugFixKind> enabled_kinds,
                             const RuleConfig& config,
                             bool verbose_rules)
    : rewriter_(rewriter), context_(context), issues_(issues),
      dry_run_(dry_run), is_c_language_(is_c_language),
      config_(config), verbose_rules_(verbose_rules),
      enabled_kinds_(std::move(enabled_kinds)) {}

// --- Location matching ---

bool BugFixVisitor::filesMatch(const std::string& a, const std::string& b) const {
    return rule_utils::filesMatch(a, b);
}

const analysis::BugFixIssue* BugFixVisitor::findMatch(
    clang::SourceLocation loc, analysis::BugFixKind kind) const {
    if (!loc.isValid()) return nullptr;

    auto& sm = context_.getSourceManager();
    auto presumed = sm.getPresumedLoc(loc);
    if (presumed.isInvalid()) return nullptr;

    std::string file = presumed.getFilename();
    int line = static_cast<int>(presumed.getLine());

    const analysis::BugFixIssue* best = nullptr;
    int best_delta = 4; // threshold: must be <= 3

    for (const auto& issue : issues_) {
        if (issue.kind != kind) continue;
        if (!filesMatch(file, issue.file)) continue;

        int delta = std::abs(line - static_cast<int>(issue.line));
        if (delta < best_delta) {
            best_delta = delta;
            best = &issue;
        }
    }
    return best;
}

// --- Helpers ---

bool BugFixVisitor::isInMacro(clang::SourceLocation loc) const {
    return rule_utils::isInMacro(loc);
}

std::string BugFixVisitor::kindToString(analysis::BugFixKind kind) const {
    switch (kind) {
    case analysis::BugFixKind::UnsignedWraparound:     return "UnsignedWraparound";
    case analysis::BugFixKind::SignedNegationOverflow:  return "SignedNegationOverflow";
    case analysis::BugFixKind::SignedLeftShift:         return "SignedLeftShift";
    case analysis::BugFixKind::UninitializedVariable:   return "UninitializedVariable";
    case analysis::BugFixKind::UnusedVariable:          return "UnusedVariable";
    case analysis::BugFixKind::FPEqualityComparison:    return "FPEqualityComparison";
    case analysis::BugFixKind::DivisionByZero:          return "DivisionByZero";
    case analysis::BugFixKind::NullDerefGuard:          return "NullDerefGuard";
    case analysis::BugFixKind::StringOverflow:          return "StringOverflow";
    case analysis::BugFixKind::ImplicitFallthrough:     return "ImplicitFallthrough";
    case analysis::BugFixKind::SizeofPointer:           return "SizeofPointer";
    case analysis::BugFixKind::IntegerTruncation:       return "IntegerTruncation";
    case analysis::BugFixKind::DanglingElse:            return "DanglingElse";
    }
    return "Unknown";
}

bool BugFixVisitor::isKindEnabled(analysis::BugFixKind kind) const {
    if (enabled_kinds_.empty()) return true;
    return enabled_kinds_.count(kind) > 0;
}

bool BugFixVisitor::isRuleEnabled(const std::string& rule_id) const {
    // If no kind filter is set, check safety tier only
    // If kind filter is set, we already checked in the dispatch loop
    (void)rule_id;
    return true;
}

// --- Visitor methods (registry dispatch) ---

bool BugFixVisitor::VisitBinaryOperator(clang::BinaryOperator* op) {
    if (isInMacro(op->getOperatorLoc())) return true;
    auto loc = op->getOperatorLoc();

    auto& reg = FixRuleRegistry::instance();

    // Issue-driven rules
    for (auto* rule : reg.rules_for(ASTNodeKind::BinaryOperator)) {
        if (!rule->has_legacy_kind()) continue;
        auto kind = rule->mapped_bug_fix_kind();
        if (!isKindEnabled(kind)) continue;
        if (rule->safety_tier() > config_.max_safety_tier) continue;

        if (auto* issue = findMatch(loc, kind)) {
            std::string dedup_key = issue->file + ":" + std::to_string(issue->line) +
                                    ":" + rule->id();
            if (applied_locations_.count(dedup_key)) continue;

            auto result = rule->try_apply(op, *issue, rewriter_, context_,
                                           is_c_language_, dry_run_);
            if (result.applied && result.confidence >= config_.min_confidence) {
                applied_++;
                applied_locations_.insert(dedup_key);
                if (!result.header.empty()) {
                    auto& sm = context_.getSourceManager();
                    auto file_id = sm.getFileID(sm.getSpellingLoc(loc));
                    rule_utils::injectIncludeIfMissing(file_id, result.header,
                                                        rewriter_, context_,
                                                        injected_includes_, dry_run_, &log_);
                }
                {
                    std::ostringstream oss;
                    std::string prefix = dry_run_ ? "FIX (dry-run)" : "FIX";
                    oss << prefix << " [" << safetyTierToString(result.tier)
                        << ", " << std::fixed << std::setprecision(2) << result.confidence
                        << "]: " << rule->display_name() << " at "
                        << issue->file << ":" << issue->line
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
            } else if (!result.applied && !result.reason.empty()) {
                skipped_++;
            }
        }
    }

    // Standalone rules
    for (auto* rule : reg.rules_for(ASTNodeKind::BinaryOperator)) {
        if (!rule->is_standalone()) continue;
        if (rule->safety_tier() > config_.max_safety_tier) continue;

        auto& sm = context_.getSourceManager();
        auto presumed = sm.getPresumedLoc(loc);
        if (presumed.isInvalid()) continue;
        std::string file = presumed.getFilename();
        unsigned line = presumed.getLine();

        std::string dedup_key = file + ":" + std::to_string(line) + ":" + rule->id();
        if (applied_locations_.count(dedup_key)) continue;

        auto result = rule->try_apply_standalone(op, rewriter_, context_,
                                                   is_c_language_, dry_run_);
        if (result.applied && result.confidence >= config_.min_confidence) {
            applied_++;
            applied_locations_.insert(dedup_key);
            if (!result.header.empty()) {
                auto file_id = sm.getFileID(sm.getSpellingLoc(loc));
                rule_utils::injectIncludeIfMissing(file_id, result.header,
                                                    rewriter_, context_,
                                                    injected_includes_, dry_run_, &log_);
            }
            {
                std::ostringstream oss;
                std::string prefix = dry_run_ ? "FIX (dry-run)" : "FIX";
                oss << prefix << " [" << safetyTierToString(result.tier)
                    << ", " << std::fixed << std::setprecision(2) << result.confidence
                    << "]: " << rule->display_name() << " at "
                    << file << ":" << line
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

    return true;
}

bool BugFixVisitor::VisitUnaryOperator(clang::UnaryOperator* op) {
    if (isInMacro(op->getOperatorLoc())) return true;
    auto loc = op->getOperatorLoc();

    auto& reg = FixRuleRegistry::instance();

    for (auto* rule : reg.rules_for(ASTNodeKind::UnaryOperator)) {
        if (!rule->has_legacy_kind()) continue;
        auto kind = rule->mapped_bug_fix_kind();
        if (!isKindEnabled(kind)) continue;
        if (rule->safety_tier() > config_.max_safety_tier) continue;

        if (auto* issue = findMatch(loc, kind)) {
            std::string dedup_key = issue->file + ":" + std::to_string(issue->line) +
                                    ":" + rule->id();
            if (applied_locations_.count(dedup_key)) continue;

            auto result = rule->try_apply(op, *issue, rewriter_, context_,
                                           is_c_language_, dry_run_);
            if (result.applied && result.confidence >= config_.min_confidence) {
                applied_++;
                applied_locations_.insert(dedup_key);
                {
                    std::ostringstream oss;
                    std::string prefix = dry_run_ ? "FIX (dry-run)" : "FIX";
                    oss << prefix << " [" << safetyTierToString(result.tier)
                        << ", " << std::fixed << std::setprecision(2) << result.confidence
                        << "]: " << rule->display_name() << " at "
                        << issue->file << ":" << issue->line
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
            } else if (!result.applied && !result.reason.empty()) {
                skipped_++;
            }
        }
    }

    return true;
}

bool BugFixVisitor::VisitVarDecl(clang::VarDecl* decl) {
    if (!decl->hasLocalStorage()) return true;
    if (decl->isImplicit()) return true;
    if (isInMacro(decl->getLocation())) return true;

    auto loc = decl->getLocation();
    auto& reg = FixRuleRegistry::instance();

    for (auto* rule : reg.rules_for(ASTNodeKind::VarDecl)) {
        if (!rule->has_legacy_kind()) continue;
        auto kind = rule->mapped_bug_fix_kind();
        if (!isKindEnabled(kind)) continue;
        if (rule->safety_tier() > config_.max_safety_tier) continue;

        if (auto* issue = findMatch(loc, kind)) {
            // For var-related fixes, also check variable name matches
            if (!issue->var_name.empty() &&
                decl->getNameAsString() != issue->var_name) continue;

            std::string dedup_key = issue->file + ":" + std::to_string(issue->line) +
                                    ":" + rule->id();
            if (applied_locations_.count(dedup_key)) continue;

            auto result = rule->try_apply(decl, *issue, rewriter_, context_,
                                           is_c_language_, dry_run_);
            if (result.applied && result.confidence >= config_.min_confidence) {
                applied_++;
                applied_locations_.insert(dedup_key);
                {
                    std::ostringstream oss;
                    std::string prefix = dry_run_ ? "FIX (dry-run)" : "FIX";
                    oss << prefix << " [" << safetyTierToString(result.tier)
                        << ", " << std::fixed << std::setprecision(2) << result.confidence
                        << "]: " << rule->display_name() << " '"
                        << issue->var_name << "' at "
                        << issue->file << ":" << issue->line
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
            } else if (!result.applied && !result.reason.empty()) {
                skipped_++;
                if (result.reason != "already initialized" &&
                    result.reason != "extern decl")
                    log_.push_back("SKIP: " + result.reason + " at " +
                                    issue->file + ":" + std::to_string(issue->line));
            }
        }
    }

    return true;
}

bool BugFixVisitor::VisitCallExpr(clang::CallExpr* call) {
    auto loc = call->getBeginLoc();
    if (isInMacro(loc)) return true;

    auto& reg = FixRuleRegistry::instance();
    for (auto* rule : reg.rules_for(ASTNodeKind::CallExpr)) {
        if (!rule->is_standalone()) continue;
        if (rule->safety_tier() > config_.max_safety_tier) continue;

        auto& sm = context_.getSourceManager();
        auto presumed = sm.getPresumedLoc(loc);
        if (presumed.isInvalid()) continue;
        std::string file = presumed.getFilename();
        unsigned line = presumed.getLine();

        std::string dedup_key = file + ":" + std::to_string(line) + ":" + rule->id();
        if (applied_locations_.count(dedup_key)) continue;

        auto result = rule->try_apply_standalone(call, rewriter_, context_,
                                                   is_c_language_, dry_run_);
        if (result.applied && result.confidence >= config_.min_confidence) {
            applied_++;
            applied_locations_.insert(dedup_key);
            if (!result.header.empty()) {
                auto file_id = sm.getFileID(sm.getSpellingLoc(loc));
                rule_utils::injectIncludeIfMissing(file_id, result.header,
                                                    rewriter_, context_,
                                                    injected_includes_, dry_run_, &log_);
            }
            {
                std::ostringstream oss;
                std::string prefix = dry_run_ ? "FIX (dry-run)" : "FIX";
                oss << prefix << " [" << safetyTierToString(result.tier)
                    << ", " << std::fixed << std::setprecision(2) << result.confidence
                    << "]: " << rule->display_name() << " at "
                    << file << ":" << line
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
    return true;
}

bool BugFixVisitor::VisitSwitchStmt(clang::SwitchStmt* stmt) {
    auto loc = stmt->getSwitchLoc();
    if (isInMacro(loc)) return true;

    auto& reg = FixRuleRegistry::instance();
    for (auto* rule : reg.rules_for(ASTNodeKind::SwitchStmt)) {
        if (!rule->is_standalone()) continue;
        if (rule->safety_tier() > config_.max_safety_tier) continue;

        auto& sm = context_.getSourceManager();
        auto presumed = sm.getPresumedLoc(loc);
        if (presumed.isInvalid()) continue;
        std::string file = presumed.getFilename();
        unsigned line = presumed.getLine();

        std::string dedup_key = file + ":" + std::to_string(line) + ":" + rule->id();
        if (applied_locations_.count(dedup_key)) continue;

        auto result = rule->try_apply_standalone(stmt, rewriter_, context_,
                                                   is_c_language_, dry_run_);
        if (result.applied && result.confidence >= config_.min_confidence) {
            applied_++;
            applied_locations_.insert(dedup_key);
            {
                std::ostringstream oss;
                std::string prefix = dry_run_ ? "FIX (dry-run)" : "FIX";
                oss << prefix << " [" << safetyTierToString(result.tier)
                    << ", " << std::fixed << std::setprecision(2) << result.confidence
                    << "]: " << rule->display_name() << " at "
                    << file << ":" << line
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
    return true;
}

bool BugFixVisitor::VisitIfStmt(clang::IfStmt* stmt) {
    auto loc = stmt->getIfLoc();
    if (isInMacro(loc)) return true;

    auto& reg = FixRuleRegistry::instance();
    for (auto* rule : reg.rules_for(ASTNodeKind::IfStmt)) {
        if (!rule->is_standalone()) continue;
        if (rule->safety_tier() > config_.max_safety_tier) continue;

        auto& sm = context_.getSourceManager();
        auto presumed = sm.getPresumedLoc(loc);
        if (presumed.isInvalid()) continue;
        std::string file = presumed.getFilename();
        unsigned line = presumed.getLine();

        std::string dedup_key = file + ":" + std::to_string(line) + ":" + rule->id();
        if (applied_locations_.count(dedup_key)) continue;

        auto result = rule->try_apply_standalone(stmt, rewriter_, context_,
                                                   is_c_language_, dry_run_);
        if (result.applied && result.confidence >= config_.min_confidence) {
            applied_++;
            applied_locations_.insert(dedup_key);
            {
                std::ostringstream oss;
                std::string prefix = dry_run_ ? "FIX (dry-run)" : "FIX";
                oss << prefix << " [" << safetyTierToString(result.tier)
                    << ", " << std::fixed << std::setprecision(2) << result.confidence
                    << "]: " << rule->display_name() << " at "
                    << file << ":" << line
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
    return true;
}

} // namespace core
} // namespace optiweave
