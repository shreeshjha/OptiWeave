#include "../../include/optiweave/core/rewriter.hpp"
#include "../../include/optiweave/utils/source_utils.hpp"

namespace optiweave::core {

SafeRewriter::SafeRewriter(clang::SourceManager &source_manager,
                          const clang::LangOptions &lang_opts)
    : rewriter_(source_manager, lang_opts) {}

bool SafeRewriter::replaceText(clang::SourceRange range, const std::string &replacement) {
    // Check for conflicts with existing modifications
    for (const auto &existing_range : modified_ranges_) {
        if (rangesOverlap(range, existing_range)) {
            return false; // Conflict detected
        }
    }
    
    // Apply the replacement
    if (rewriter_.ReplaceText(range, replacement)) {
        return false; // Rewriter failed
    }
    
    // Track the modified range
    modified_ranges_.push_back(range);
    return true;
}

bool SafeRewriter::insertText(clang::SourceLocation location, const std::string &text) {
    // Create a range for the insertion point
    clang::SourceRange insertion_range(location, location);
    
    // Check for conflicts
    for (const auto &existing_range : modified_ranges_) {
        if (rangesOverlap(insertion_range, existing_range)) {
            return false; // Conflict detected
        }
    }
    
    // Apply the insertion
    if (rewriter_.InsertText(location, text)) {
        return false; // Rewriter failed
    }
    
    // Track the modified range
    modified_ranges_.push_back(insertion_range);
    return true;
}

bool SafeRewriter::hasConflicts() const {
    // Check all pairs of ranges for overlaps
    for (size_t i = 0; i < modified_ranges_.size(); ++i) {
        for (size_t j = i + 1; j < modified_ranges_.size(); ++j) {
            if (rangesOverlap(modified_ranges_[i], modified_ranges_[j])) {
                return true;
            }
        }
    }
    return false;
}

void SafeRewriter::reset() {
    // Reset the rewriter (this may not be possible with current Clang API)
    // For now, just clear our tracking
    modified_ranges_.clear();
}

bool SafeRewriter::rangesOverlap(clang::SourceRange range1, clang::SourceRange range2) const {
    return optiweave::utils::rangesOverlap(range1, range2, rewriter_.getSourceMgr());
}

} // namespace optiweave::core