#include <optiweave/core/patch_visitor.hpp>
#include <clang/AST/Stmt.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Decl.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Lex/Lexer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <cmath>
#include <algorithm>

namespace optiweave {
namespace core {

PatchVisitor::PatchVisitor(clang::Rewriter& rewriter,
                           clang::ASTContext& context,
                           const std::vector<analysis::OptimizationPattern>& suggestions,
                           bool dry_run,
                           bool is_c_language)
    : rewriter_(rewriter), context_(context), suggestions_(suggestions),
      dry_run_(dry_run), is_c_language_(is_c_language) {}

// --- Location matching ---

bool PatchVisitor::filesMatch(const std::string& a, const std::string& b) const {
    auto basename_a = llvm::sys::path::filename(a);
    auto basename_b = llvm::sys::path::filename(b);
    return basename_a == basename_b;
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
    int best_delta = 4; // threshold: must be <= 3

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

// --- Helpers ---

std::string PatchVisitor::getSourceText(clang::SourceRange range) const {
    auto& sm = context_.getSourceManager();
    auto& lo = context_.getLangOpts();
    auto char_range = clang::CharSourceRange::getTokenRange(range);
    return clang::Lexer::getSourceText(char_range, sm, lo).str();
}

std::string PatchVisitor::getLoopInductionVar(clang::ForStmt* loop) const {
    if (auto* init = loop->getInit()) {
        if (auto* decl_stmt = llvm::dyn_cast<clang::DeclStmt>(init)) {
            if (decl_stmt->isSingleDecl()) {
                if (auto* var = llvm::dyn_cast<clang::VarDecl>(decl_stmt->getSingleDecl())) {
                    return var->getNameAsString();
                }
            }
        }
    }
    return "";
}

std::vector<clang::BinaryOperator*> PatchVisitor::collectAllDivisions(clang::Stmt* stmt) const {
    std::vector<clang::BinaryOperator*> result;
    if (!stmt) return result;

    if (auto* bin_op = llvm::dyn_cast<clang::BinaryOperator>(stmt)) {
        if (bin_op->getOpcode() == clang::BO_Div ||
            bin_op->getOpcode() == clang::BO_DivAssign) {
            result.push_back(bin_op);
        }
    }

    for (auto* child : stmt->children()) {
        auto child_divs = collectAllDivisions(child);
        result.insert(result.end(), child_divs.begin(), child_divs.end());
    }
    return result;
}

bool PatchVisitor::isLoopInvariant(const std::string& expr_text) const {
    for (const auto& var : enclosing_induction_vars_) {
        if (!var.empty() && expr_text.find(var) != std::string::npos) {
            return false;
        }
    }
    return true;
}

std::string PatchVisitor::getIndentation(clang::SourceLocation loc) const {
    if (!loc.isValid()) return "";

    auto& sm = context_.getSourceManager();
    auto presumed = sm.getPresumedLoc(loc);
    if (presumed.isInvalid()) return "";

    // Get the file buffer and find the start of the line
    auto file_id = sm.getFileID(sm.getSpellingLoc(loc));
    bool invalid = false;
    auto buf = sm.getBufferData(file_id, &invalid);
    if (invalid || buf.empty()) return "";

    unsigned offset = sm.getFileOffset(sm.getSpellingLoc(loc));
    // Scan backwards to find line start
    unsigned line_start = offset;
    while (line_start > 0 && buf[line_start - 1] != '\n') {
        line_start--;
    }

    // Collect whitespace from line start to first non-space
    std::string indent;
    for (unsigned i = line_start; i < offset && i < buf.size(); i++) {
        if (buf[i] == ' ' || buf[i] == '\t') {
            indent += buf[i];
        } else {
            break;
        }
    }
    return indent;
}

bool PatchVisitor::checkExistingPragma(clang::SourceLocation loc) const {
    if (!loc.isValid()) return false;

    auto& sm = context_.getSourceManager();
    auto file_id = sm.getFileID(sm.getSpellingLoc(loc));
    bool invalid = false;
    auto buf = sm.getBufferData(file_id, &invalid);
    if (invalid || buf.empty()) return false;

    unsigned offset = sm.getFileOffset(sm.getSpellingLoc(loc));

    // Scan up to 3 lines above
    int lines_scanned = 0;
    unsigned pos = offset;
    while (pos > 0 && lines_scanned < 4) {
        pos--;
        if (buf[pos] == '\n') {
            lines_scanned++;
        }
        // Check this region for the pragma
    }

    // Extract the text from pos to offset and search for pragma
    if (pos < offset) {
        auto region = buf.substr(pos, offset - pos);
        if (region.find("#pragma clang loop vectorize") != llvm::StringRef::npos) {
            return true;
        }
    }
    return false;
}

// --- Visitor methods ---

bool PatchVisitor::VisitForStmt(clang::ForStmt* stmt) {
    // Push induction variable for nested loop invariance checking
    std::string ind_var = getLoopInductionVar(stmt);
    enclosing_induction_vars_.push_back(ind_var);

    handleLoopPatches(stmt->getBody(), stmt->getForLoc(), stmt);

    enclosing_induction_vars_.pop_back();
    return true;
}

bool PatchVisitor::VisitWhileStmt(clang::WhileStmt* stmt) {
    // While loops have no init-declared induction variable, push empty
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

// --- Shared loop logic ---

void PatchVisitor::handleLoopPatches(clang::Stmt* body,
                                      clang::SourceLocation keyword_loc,
                                      clang::Stmt* loop_stmt) {
    // Macro safety: skip if location is inside a macro expansion
    if (keyword_loc.isMacroID()) {
        log_.push_back("SKIP: Loop at macro location — cannot safely patch");
        skipped_++;
        return;
    }

    // Try division-in-loop match first (apply before vectorization for correct ordering)
    auto* div_pat = findMatch(keyword_loc, "Division in Hot Loop");
    if (div_pat) {
        applyDivisionPatch(keyword_loc, body, *div_pat);
    }

    // Try vectorization match
    auto* vec_pat = findMatch(keyword_loc, "SIMD Vectorization Opportunity");
    if (vec_pat) {
        applyVectorizationPatch(keyword_loc, *vec_pat);
    }

    // Try other pattern matches for advisory comments
    auto* any_pat = findMatchByLocation(keyword_loc);
    if (any_pat && any_pat->pattern_name != "Division in Hot Loop" &&
        any_pat->pattern_name != "SIMD Vectorization Opportunity") {
        applyAdvisoryComment(keyword_loc, *any_pat);
    }
}

// --- Concrete patch generators ---

bool PatchVisitor::applyDivisionPatch(clang::SourceLocation insert_loc,
                                       clang::Stmt* body,
                                       const analysis::OptimizationPattern& pat) {
    // Dedup check
    std::string dedup_key = pat.location.get_file() + ":" +
                            std::to_string(pat.location.line) + ":Division in Hot Loop";
    if (applied_locations_.count(dedup_key)) {
        return false;
    }

    // Collect ALL divisions in the loop body
    auto divisions = collectAllDivisions(body);
    if (divisions.empty()) return false;

    std::string indent = getIndentation(insert_loc);
    bool any_applied = false;
    std::set<std::string> already_hoisted_rhs; // track RHS text already hoisted

    for (auto* div_op : divisions) {
        // Macro safety for division location
        if (div_op->getOperatorLoc().isMacroID()) {
            log_.push_back("SKIP: Division inside macro — cannot safely patch");
            skipped_++;
            continue;
        }

        auto* rhs = div_op->getRHS();
        if (!rhs) continue;

        // Integer division safety — skip integer operands
        if (div_op->getLHS()->getType()->isIntegerType() &&
            rhs->getType()->isIntegerType()) {
            log_.push_back("SKIP: Integer division at " + pat.location.get_file() + ":" +
                            std::to_string(pat.location.line) +
                            " — reciprocal would change semantics");
            skipped_++;
            continue;
        }

        std::string rhs_text = getSourceText(rhs->getSourceRange());
        if (rhs_text.empty()) continue;

        // Skip if this exact RHS was already hoisted in this loop
        if (already_hoisted_rhs.count(rhs_text)) {
            // Just replace the operator and RHS to use the existing reciprocal
            // We'd need to track the recip name — for simplicity, skip duplicate RHS
            continue;
        }

        // Check loop invariance against ALL enclosing loop variables
        if (!isLoopInvariant(rhs_text)) {
            log_.push_back("SKIP: Division at " + pat.location.get_file() + ":" +
                            std::to_string(pat.location.line) +
                            " — divisor '" + rhs_text + "' depends on loop variable");
            skipped_++;
            continue;
        }

        recip_counter_++;
        std::string recip_name = "__ow_recip_" + std::to_string(recip_counter_);
        already_hoisted_rhs.insert(rhs_text);

        // C vs C++ type declaration
        std::string type_keyword = is_c_language_ ? "const double" : "const auto";
        std::string recip_decl = indent + type_keyword + " " + recip_name +
                                  " = 1.0 / (" + rhs_text + ");" +
                                  " /* OPTIWEAVE: ensure divisor != 0 */\n";

        if (dry_run_) {
            log_.push_back("PATCH (dry-run): Division in loop at " + pat.location.get_file() + ":" +
                            std::to_string(pat.location.line) +
                            " — would hoist divisor '" + rhs_text + "' as " + recip_name);
            applied_++;
            any_applied = true;
            continue;
        }

        // Insert reciprocal before the loop
        rewriter_.InsertTextBefore(insert_loc, recip_decl);

        // Replace division with multiplication by reciprocal
        if (div_op->getOpcode() == clang::BO_Div) {
            rewriter_.ReplaceText(div_op->getOperatorLoc(), 1, "*");
            rewriter_.ReplaceText(rhs->getSourceRange(), recip_name);
        } else if (div_op->getOpcode() == clang::BO_DivAssign) {
            rewriter_.ReplaceText(div_op->getOperatorLoc(), 2, "*=");
            rewriter_.ReplaceText(rhs->getSourceRange(), recip_name);
        }

        log_.push_back("PATCH: Division in loop at " + pat.location.get_file() + ":" +
                        std::to_string(pat.location.line) +
                        " — hoisted divisor '" + rhs_text + "' as " + recip_name);
        applied_++;
        any_applied = true;
    }

    if (any_applied) {
        applied_locations_.insert(dedup_key);
    }
    return any_applied;
}

bool PatchVisitor::applyVectorizationPatch(clang::SourceLocation keyword_loc,
                                            const analysis::OptimizationPattern& pat) {
    // Dedup check
    std::string dedup_key = pat.location.get_file() + ":" +
                            std::to_string(pat.location.line) + ":SIMD Vectorization Opportunity";
    if (applied_locations_.count(dedup_key)) {
        return false;
    }

    // Idempotency: check if pragma already exists
    if (checkExistingPragma(keyword_loc)) {
        log_.push_back("SKIP: Vectorization pragma already present at " +
                        pat.location.get_file() + ":" + std::to_string(pat.location.line));
        skipped_++;
        return false;
    }

    std::string indent = getIndentation(keyword_loc);
    std::string pragma = indent + "#pragma clang loop vectorize(enable)\n";

    if (dry_run_) {
        log_.push_back("PATCH (dry-run): Vectorization at " + pat.location.get_file() + ":" +
                        std::to_string(pat.location.line) +
                        " — would insert vectorization pragma");
        applied_++;
        applied_locations_.insert(dedup_key);
        return true;
    }

    rewriter_.InsertTextBefore(keyword_loc, pragma);

    log_.push_back("PATCH: Vectorization pragma at " + pat.location.get_file() + ":" +
                    std::to_string(pat.location.line));
    applied_++;
    applied_locations_.insert(dedup_key);
    return true;
}

bool PatchVisitor::applyAdvisoryComment(clang::SourceLocation loc,
                                         const analysis::OptimizationPattern& pat) {
    // Dedup check
    std::string dedup_key = pat.location.get_file() + ":" +
                            std::to_string(pat.location.line) + ":" + pat.pattern_name;
    if (applied_locations_.count(dedup_key)) {
        return false;
    }

    std::string indent = getIndentation(loc);
    std::string comment;

    // Match actual detector output names from pattern_detector.cpp
    if (pat.pattern_name == "Naive Matrix Multiplication") {
        comment = indent + "// OPTIWEAVE: " + pat.description +
                  " -- consider optimized libraries (e.g., BLAS)\n";
    } else if (pat.pattern_name == "O(n²) Algorithm") {
        // Note: the actual pattern name uses the Unicode superscript
        comment = indent + "// OPTIWEAVE: Quadratic complexity detected" +
                  " -- consider more efficient algorithm\n";
    } else if (pat.pattern_name == "Poor Memory Locality") {
        comment = indent + "// OPTIWEAVE: Poor memory locality detected" +
                  " -- consider loop interchange or cache blocking\n";
    } else if (pat.pattern_name == "Repeated Computation in Loop") {
        comment = indent + "// OPTIWEAVE: Loop-invariant computation" +
                  " -- consider hoisting outside loop\n";
    } else if (pat.pattern_name == "Potential Branch Misprediction") {
        comment = indent + "// OPTIWEAVE: High branch ratio" +
                  " -- consider branchless alternatives\n";
    } else {
        comment = indent + "// OPTIWEAVE: " + pat.pattern_name +
                  " -- " + pat.description + "\n";
    }

    if (dry_run_) {
        log_.push_back("ADVISORY (dry-run): " + pat.pattern_name + " at " +
                        pat.location.get_file() + ":" + std::to_string(pat.location.line) +
                        " — would insert comment");
        advisories_++;
        applied_locations_.insert(dedup_key);
        return true;
    }

    rewriter_.InsertTextBefore(loc, comment);
    log_.push_back("ADVISORY: " + pat.pattern_name + " at " +
                    pat.location.get_file() + ":" + std::to_string(pat.location.line));
    advisories_++;
    applied_locations_.insert(dedup_key);
    return true;
}

} // namespace core
} // namespace optiweave
