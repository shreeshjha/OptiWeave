#include <optiweave/core/bug_fix_visitor.hpp>
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

namespace optiweave {
namespace core {

BugFixVisitor::BugFixVisitor(clang::Rewriter& rewriter,
                             clang::ASTContext& context,
                             const std::vector<analysis::BugFixIssue>& issues,
                             bool dry_run,
                             bool is_c_language,
                             std::set<analysis::BugFixKind> enabled_kinds)
    : rewriter_(rewriter), context_(context), issues_(issues),
      dry_run_(dry_run), is_c_language_(is_c_language),
      enabled_kinds_(std::move(enabled_kinds)) {}

// --- Location matching ---

bool BugFixVisitor::filesMatch(const std::string& a, const std::string& b) const {
    // Fix 6: prefer exact full-path match; fall back to basename only when
    // at least one path has no directory component (bare filename).
    if (a == b) return true;
    bool a_bare = (a.find('/') == std::string::npos);
    bool b_bare = (b.find('/') == std::string::npos);
    if (a_bare || b_bare)
        return llvm::sys::path::filename(a) == llvm::sys::path::filename(b);
    return false;
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

std::string BugFixVisitor::getSourceText(clang::SourceRange range) const {
    auto& sm = context_.getSourceManager();
    auto& lo = context_.getLangOpts();
    auto char_range = clang::CharSourceRange::getTokenRange(range);
    return clang::Lexer::getSourceText(char_range, sm, lo).str();
}

std::string BugFixVisitor::getIndentation(clang::SourceLocation loc) const {
    if (!loc.isValid()) return "";

    auto& sm = context_.getSourceManager();
    auto file_id = sm.getFileID(sm.getSpellingLoc(loc));
    bool invalid = false;
    auto buf = sm.getBufferData(file_id, &invalid);
    if (invalid || buf.empty()) return "";

    unsigned offset = sm.getFileOffset(sm.getSpellingLoc(loc));
    unsigned line_start = offset;
    while (line_start > 0 && buf[line_start - 1] != '\n') {
        line_start--;
    }

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

bool BugFixVisitor::isInMacro(clang::SourceLocation loc) const {
    return loc.isValid() && loc.isMacroID();
}

std::string BugFixVisitor::kindToString(analysis::BugFixKind kind) const {
    switch (kind) {
    case analysis::BugFixKind::UnsignedWraparound:     return "UnsignedWraparound";
    case analysis::BugFixKind::SignedNegationOverflow:  return "SignedNegationOverflow";
    case analysis::BugFixKind::SignedLeftShift:         return "SignedLeftShift";
    case analysis::BugFixKind::UninitializedVariable:   return "UninitializedVariable";
    case analysis::BugFixKind::UnusedVariable:          return "UnusedVariable";
    case analysis::BugFixKind::FPEqualityComparison:    return "FPEqualityComparison";
    }
    return "Unknown";
}

bool BugFixVisitor::isKindEnabled(analysis::BugFixKind kind) const {
    if (enabled_kinds_.empty()) return true; // empty = all enabled
    return enabled_kinds_.count(kind) > 0;
}

// Fix 3: Inject #include <header> if not already present in the file.
bool BugFixVisitor::injectIncludeIfMissing(clang::FileID file_id,
                                            const std::string& header) {
    // Idempotency: already injected in this run
    if (injected_includes_.count(header)) return false;

    auto& sm = context_.getSourceManager();
    bool invalid = false;
    auto buf = sm.getBufferData(file_id, &invalid);
    if (invalid || buf.empty()) return false;

    // Already present in the file
    if (buf.find(header) != llvm::StringRef::npos) {
        injected_includes_.insert(header);
        return false;
    }

    if (dry_run_) {
        log_.push_back("WOULD INJECT: #include " + header);
        injected_includes_.insert(header);
        return true;
    }

    // Find the offset just after the last #include line
    std::string buf_str = buf.str();
    size_t last_include_end = std::string::npos;
    size_t pos = 0;
    while ((pos = buf_str.find("#include", pos)) != std::string::npos) {
        size_t end_of_line = buf_str.find('\n', pos);
        if (end_of_line != std::string::npos) {
            last_include_end = end_of_line + 1;
        } else {
            last_include_end = buf_str.size();
        }
        pos++;
    }

    std::string inject = "#include " + header + "\n";
    clang::SourceLocation insert_loc;
    if (last_include_end != std::string::npos) {
        insert_loc = sm.getLocForStartOfFile(file_id)
                       .getLocWithOffset(static_cast<int>(last_include_end));
    } else {
        insert_loc = sm.getLocForStartOfFile(file_id);
    }

    rewriter_.InsertTextBefore(insert_loc, inject);
    injected_includes_.insert(header);
    return true;
}

// --- Visitor methods ---

bool BugFixVisitor::VisitBinaryOperator(clang::BinaryOperator* op) {
    if (isInMacro(op->getOperatorLoc())) return true;

    auto loc = op->getOperatorLoc();

    // Unsigned wraparound: subtraction
    if (op->getOpcode() == clang::BO_Sub && isKindEnabled(analysis::BugFixKind::UnsignedWraparound)) {
        if (auto* issue = findMatch(loc, analysis::BugFixKind::UnsignedWraparound)) {
            applyUnsignedWraparoundFix(op, *issue);
        }
    }

    // Signed left shift
    if (op->getOpcode() == clang::BO_Shl && isKindEnabled(analysis::BugFixKind::SignedLeftShift)) {
        if (auto* issue = findMatch(loc, analysis::BugFixKind::SignedLeftShift)) {
            applySignedLeftShiftFix(op, *issue);
        }
    }

    // FP equality comparison
    if ((op->getOpcode() == clang::BO_EQ || op->getOpcode() == clang::BO_NE) &&
        isKindEnabled(analysis::BugFixKind::FPEqualityComparison)) {
        if (auto* issue = findMatch(loc, analysis::BugFixKind::FPEqualityComparison)) {
            applyFPEqualityFix(op, *issue);
        }
    }

    return true;
}

bool BugFixVisitor::VisitUnaryOperator(clang::UnaryOperator* op) {
    if (isInMacro(op->getOperatorLoc())) return true;

    if (op->getOpcode() == clang::UO_Minus &&
        isKindEnabled(analysis::BugFixKind::SignedNegationOverflow)) {
        if (auto* issue = findMatch(op->getOperatorLoc(),
                                     analysis::BugFixKind::SignedNegationOverflow)) {
            applySignedNegationFix(op, *issue);
        }
    }

    return true;
}

bool BugFixVisitor::VisitVarDecl(clang::VarDecl* decl) {
    if (!decl->hasLocalStorage()) return true; // skip globals, statics
    if (decl->isImplicit()) return true;
    if (isInMacro(decl->getLocation())) return true;

    auto loc = decl->getLocation();

    // Uninitialized variable fix
    if (isKindEnabled(analysis::BugFixKind::UninitializedVariable)) {
        if (auto* issue = findMatch(loc, analysis::BugFixKind::UninitializedVariable)) {
            if (decl->getNameAsString() == issue->var_name) {
                applyUninitializedVarFix(decl, *issue);
            }
        }
    }

    // Unused variable fix
    if (isKindEnabled(analysis::BugFixKind::UnusedVariable)) {
        if (auto* issue = findMatch(loc, analysis::BugFixKind::UnusedVariable)) {
            if (decl->getNameAsString() == issue->var_name) {
                applyUnusedVarFix(decl, *issue);
            }
        }
    }

    return true;
}

// --- Fix applicators ---

bool BugFixVisitor::applyUnsignedWraparoundFix(clang::BinaryOperator* op,
                                                 const analysis::BugFixIssue& issue) {
    std::string dedup_key = issue.file + ":" + std::to_string(issue.line) +
                            ":UnsignedWraparound";
    if (applied_locations_.count(dedup_key)) return false;

    std::string lhs_text = getSourceText(op->getLHS()->getSourceRange());
    std::string rhs_text = getSourceText(op->getRHS()->getSourceRange());
    if (lhs_text.empty() || rhs_text.empty()) {
        skipped_++;
        return false;
    }

    // a - b  →  ((a) >= (b) ? (a) - (b) : 0)
    std::string replacement = "((" + lhs_text + ") >= (" + rhs_text +
                              ") ? (" + lhs_text + ") - (" + rhs_text + ") : 0)";

    if (dry_run_) {
        log_.push_back("FIX (dry-run): Unsigned wraparound at " + issue.file + ":" +
                        std::to_string(issue.line) + " — would guard subtraction");
        applied_++;
        applied_locations_.insert(dedup_key);
        return true;
    }

    rewriter_.ReplaceText(op->getSourceRange(), replacement);
    log_.push_back("FIX: Unsigned wraparound at " + issue.file + ":" +
                    std::to_string(issue.line) + " — guarded subtraction");
    applied_++;
    applied_locations_.insert(dedup_key);
    return true;
}

bool BugFixVisitor::applySignedNegationFix(clang::UnaryOperator* op,
                                             const analysis::BugFixIssue& issue) {
    std::string dedup_key = issue.file + ":" + std::to_string(issue.line) +
                            ":SignedNegationOverflow";
    if (applied_locations_.count(dedup_key)) return false;

    auto* sub_expr = op->getSubExpr();
    if (!sub_expr) { skipped_++; return false; }

    std::string expr_text = getSourceText(sub_expr->getSourceRange());
    if (expr_text.empty()) { skipped_++; return false; }

    // Fix 2: Use BuiltinType::getKind() on canonical type to correctly detect
    // typedef'd integer types (int8_t, int32_t, etc.) and avoid false matches
    // on "unsigned long" (string "long" is a substring of it).
    // Use QualType::getCanonicalType() (returns QualType, not CanQualType).
    const clang::BuiltinType* bt =
        sub_expr->getType().getUnqualifiedType().getCanonicalType()
                           ->getAs<clang::BuiltinType>();

    std::string min_macro = "INT_MIN";
    std::string max_macro = "INT_MAX";
    if (bt) {
        switch (bt->getKind()) {
        case clang::BuiltinType::SChar:
        case clang::BuiltinType::Char_S:
            min_macro = "SCHAR_MIN"; max_macro = "SCHAR_MAX"; break;
        case clang::BuiltinType::Short:
            min_macro = "SHRT_MIN";  max_macro = "SHRT_MAX";  break;
        case clang::BuiltinType::Int:
            min_macro = "INT_MIN";   max_macro = "INT_MAX";   break;
        case clang::BuiltinType::Long:
            min_macro = "LONG_MIN";  max_macro = "LONG_MAX";  break;
        case clang::BuiltinType::LongLong:
            min_macro = "LLONG_MIN"; max_macro = "LLONG_MAX"; break;
        default:
            break; // Keep INT_MIN/INT_MAX as conservative fallback
        }
    }

    // -x  →  ((x) == <MIN> ? <MAX> : -(x))
    std::string replacement = "((" + expr_text + ") == " + min_macro +
                              " ? " + max_macro + " : -(" + expr_text + "))";

    if (dry_run_) {
        log_.push_back("FIX (dry-run): Signed negation overflow at " + issue.file + ":" +
                        std::to_string(issue.line) + " — would add " + min_macro + " guard");
        applied_++;
        applied_locations_.insert(dedup_key);
        // Fix 3: report what include would be injected
        auto& sm = context_.getSourceManager();
        auto file_id = sm.getFileID(sm.getSpellingLoc(op->getOperatorLoc()));
        std::string header = is_c_language_ ? "<limits.h>" : "<climits>";
        injectIncludeIfMissing(file_id, header);
        return true;
    }

    rewriter_.ReplaceText(op->getSourceRange(), replacement);

    // Fix 3: inject the limits header
    {
        auto& sm = context_.getSourceManager();
        auto file_id = sm.getFileID(sm.getSpellingLoc(op->getOperatorLoc()));
        std::string header = is_c_language_ ? "<limits.h>" : "<climits>";
        injectIncludeIfMissing(file_id, header);
    }

    log_.push_back("FIX: Signed negation overflow at " + issue.file + ":" +
                    std::to_string(issue.line) + " — added " + min_macro + " guard");
    applied_++;
    applied_locations_.insert(dedup_key);
    return true;
}

bool BugFixVisitor::applySignedLeftShiftFix(clang::BinaryOperator* op,
                                              const analysis::BugFixIssue& issue) {
    std::string dedup_key = issue.file + ":" + std::to_string(issue.line) +
                            ":SignedLeftShift";
    if (applied_locations_.count(dedup_key)) return false;

    std::string lhs_text = getSourceText(op->getLHS()->getSourceRange());
    if (lhs_text.empty()) { skipped_++; return false; }

    // x << n  →  ((unsigned)(x)) << n   (C)
    // x << n  →  (static_cast<unsigned>(x)) << n   (C++)
    std::string cast_expr;
    if (is_c_language_) {
        cast_expr = "((unsigned)(" + lhs_text + "))";
    } else {
        cast_expr = "(static_cast<unsigned>(" + lhs_text + "))";
    }

    if (dry_run_) {
        log_.push_back("FIX (dry-run): Signed left shift at " + issue.file + ":" +
                        std::to_string(issue.line) + " — would cast LHS to unsigned");
        applied_++;
        applied_locations_.insert(dedup_key);
        return true;
    }

    rewriter_.ReplaceText(op->getLHS()->getSourceRange(), cast_expr);
    log_.push_back("FIX: Signed left shift at " + issue.file + ":" +
                    std::to_string(issue.line) + " — cast LHS to unsigned");
    applied_++;
    applied_locations_.insert(dedup_key);
    return true;
}

bool BugFixVisitor::applyUninitializedVarFix(clang::VarDecl* decl,
                                               const analysis::BugFixIssue& issue) {
    std::string dedup_key = issue.file + ":" + std::to_string(issue.line) +
                            ":UninitializedVariable";
    if (applied_locations_.count(dedup_key)) return false;

    // Skip if already has an initializer
    if (decl->hasInit()) { skipped_++; return false; }

    // Skip parameters and globals (handled by hasLocalStorage check in visitor)
    if (decl->isLocalExternDecl()) { skipped_++; return false; }

    // Skip multi-variable declarations: check via parent map
    {
        auto parents = context_.getParents(*decl);
        if (!parents.empty()) {
            if (auto* decl_stmt = parents[0].get<clang::DeclStmt>()) {
                if (!decl_stmt->isSingleDecl()) {
                    log_.push_back("SKIP: Multi-variable declaration at " + issue.file + ":" +
                                    std::to_string(issue.line) + " — too risky to auto-fix");
                    skipped_++;
                    return false;
                }
            }
        }
    }

    // Determine initializer based on type
    std::string initializer;

    if (decl->getType()->isPointerType()) {
        initializer = is_c_language_ ? " = NULL" : " = nullptr";
    } else if (decl->getType()->isFloatingType()) {
        initializer = " = 0.0";
    } else if (decl->getType()->isBooleanType()) {
        initializer = is_c_language_ ? " = 0" : " = false";
    } else if (decl->getType()->isIntegerType() || decl->getType()->isEnumeralType()) {
        initializer = " = 0";
    } else {
        // struct/class/union — use {} in C++, {0} in C
        if (is_c_language_) {
            initializer = " = {0}";
        } else {
            initializer = " = {}";
        }
    }

    if (dry_run_) {
        log_.push_back("FIX (dry-run): Uninitialized variable '" + issue.var_name +
                        "' at " + issue.file + ":" + std::to_string(issue.line) +
                        " — would add '" + initializer + "'");
        applied_++;
        applied_locations_.insert(dedup_key);
        return true;
    }

    // Insert initializer before the semicolon — we find the end of the declarator
    auto end_loc = decl->getSourceRange().getEnd();
    auto after_name = clang::Lexer::getLocForEndOfToken(
        end_loc, 0, context_.getSourceManager(), context_.getLangOpts());
    rewriter_.InsertTextAfter(after_name, initializer);

    log_.push_back("FIX: Uninitialized variable '" + issue.var_name + "' at " +
                    issue.file + ":" + std::to_string(issue.line) +
                    " — added '" + initializer + "'");
    applied_++;
    applied_locations_.insert(dedup_key);
    return true;
}

bool BugFixVisitor::applyUnusedVarFix(clang::VarDecl* decl,
                                        const analysis::BugFixIssue& issue) {
    std::string dedup_key = issue.file + ":" + std::to_string(issue.line) +
                            ":UnusedVariable";
    if (applied_locations_.count(dedup_key)) return false;

    std::string var_name = decl->getNameAsString();
    std::string indent = getIndentation(decl->getBeginLoc());

    // Insert (void)varName; on the line after the declaration
    std::string void_cast = "\n" + indent + "(void)" + var_name + ";";

    if (dry_run_) {
        log_.push_back("FIX (dry-run): Unused variable '" + var_name + "' at " +
                        issue.file + ":" + std::to_string(issue.line) +
                        " — would insert (void) cast");
        applied_++;
        applied_locations_.insert(dedup_key);
        return true;
    }

    // Find the end of the declaration statement (after the semicolon)
    auto end_loc = decl->getSourceRange().getEnd();
    auto after_end = clang::Lexer::getLocForEndOfToken(
        end_loc, 0, context_.getSourceManager(), context_.getLangOpts());

    // Search for the semicolon from the end of the decl
    auto& sm = context_.getSourceManager();
    auto file_id = sm.getFileID(sm.getSpellingLoc(after_end));
    bool invalid = false;
    auto buf = sm.getBufferData(file_id, &invalid);
    if (!invalid && !buf.empty()) {
        unsigned offset = sm.getFileOffset(sm.getSpellingLoc(after_end));
        // Scan forward for the semicolon
        while (offset < buf.size() && buf[offset] != ';') {
            offset++;
        }
        if (offset < buf.size() && buf[offset] == ';') {
            // Insert after the semicolon
            auto insert_loc = sm.getSpellingLoc(after_end).getLocWithOffset(
                offset - sm.getFileOffset(sm.getSpellingLoc(after_end)) + 1);
            rewriter_.InsertTextAfter(insert_loc, void_cast);
        }
    }

    log_.push_back("FIX: Unused variable '" + var_name + "' at " +
                    issue.file + ":" + std::to_string(issue.line) +
                    " — inserted (void) cast");
    applied_++;
    applied_locations_.insert(dedup_key);
    return true;
}

bool BugFixVisitor::applyFPEqualityFix(clang::BinaryOperator* op,
                                         const analysis::BugFixIssue& issue) {
    std::string dedup_key = issue.file + ":" + std::to_string(issue.line) +
                            ":FPEqualityComparison";
    if (applied_locations_.count(dedup_key)) return false;

    std::string lhs_text = getSourceText(op->getLHS()->getSourceRange());
    std::string rhs_text = getSourceText(op->getRHS()->getSourceRange());
    if (lhs_text.empty() || rhs_text.empty()) {
        skipped_++;
        return false;
    }

    // Fix 1: select epsilon and fabs variant based on actual operand types
    clang::QualType lhs_qt = op->getLHS()->getType().getUnqualifiedType();
    clang::QualType rhs_qt = op->getRHS()->getType().getUnqualifiedType();

    auto isFloat = [](clang::QualType qt) {
        auto* bt = qt->getAs<clang::BuiltinType>();
        return bt && bt->getKind() == clang::BuiltinType::Float;
    };
    auto isLongDouble = [](clang::QualType qt) {
        auto* bt = qt->getAs<clang::BuiltinType>();
        return bt && bt->getKind() == clang::BuiltinType::LongDouble;
    };

    std::string epsilon_val;
    std::string fabs_fn;
    std::string header;

    if (isFloat(lhs_qt) && isFloat(rhs_qt)) {
        epsilon_val = "1e-6f";
        fabs_fn = is_c_language_ ? "fabsf" : "std::fabsf";
        header = is_c_language_ ? "<math.h>" : "<cmath>";
    } else if (isLongDouble(lhs_qt) || isLongDouble(rhs_qt)) {
        epsilon_val = "1e-18L";
        fabs_fn = is_c_language_ ? "fabsl" : "std::fabsl";
        header = is_c_language_ ? "<math.h>" : "<cmath>";
    } else {
        // double (default)
        epsilon_val = "1e-9";
        fabs_fn = is_c_language_ ? "fabs" : "std::fabs";
        header = is_c_language_ ? "<math.h>" : "<cmath>";
    }

    bool is_not_equal = (op->getOpcode() == clang::BO_NE);

    // a == b  →  fabs((a) - (b)) < epsilon
    // a != b  →  fabs((a) - (b)) >= epsilon
    std::string replacement;
    if (is_not_equal) {
        replacement = fabs_fn + "((" + lhs_text + ") - (" + rhs_text + ")) >= " + epsilon_val;
    } else {
        replacement = fabs_fn + "((" + lhs_text + ") - (" + rhs_text + ")) < " + epsilon_val;
    }

    if (dry_run_) {
        log_.push_back("FIX (dry-run): FP equality at " + issue.file + ":" +
                        std::to_string(issue.line) +
                        " — would replace with " + fabs_fn + " epsilon=" + epsilon_val);
        // Fix 3: report what include would be injected
        auto& sm = context_.getSourceManager();
        auto file_id = sm.getFileID(sm.getSpellingLoc(op->getOperatorLoc()));
        injectIncludeIfMissing(file_id, header);
        applied_++;
        applied_locations_.insert(dedup_key);
        return true;
    }

    rewriter_.ReplaceText(op->getSourceRange(), replacement);

    // Fix 3: inject the math header
    {
        auto& sm = context_.getSourceManager();
        auto file_id = sm.getFileID(sm.getSpellingLoc(op->getOperatorLoc()));
        injectIncludeIfMissing(file_id, header);
    }

    log_.push_back("FIX: FP equality at " + issue.file + ":" +
                    std::to_string(issue.line) +
                    " — replaced with " + fabs_fn + " epsilon=" + epsilon_val);
    applied_++;
    applied_locations_.insert(dedup_key);
    return true;
}

} // namespace core
} // namespace optiweave
