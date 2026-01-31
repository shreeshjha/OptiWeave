#pragma once

#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Basic/SourceManager.h>
#include <string>
#include <vector>

namespace optiweave::core {

/**
 * @brief Safe rewriter that handles conflicts and provides atomic operations
 */
class SafeRewriter {
public:
    explicit SafeRewriter(clang::SourceManager &source_manager,
                         const clang::LangOptions &lang_opts);
    
    /**
     * @brief Replace text in source range
     */
    bool replaceText(clang::SourceRange range, const std::string &replacement);
    
    /**
     * @brief Insert text at location
     */
    bool insertText(clang::SourceLocation location, const std::string &text);
    
    /**
     * @brief Check for conflicts between ranges
     */
    bool hasConflicts() const;
    
    /**
     * @brief Get the underlying rewriter
     */
    clang::Rewriter& getRewriter() { return rewriter_; }
    
    /**
     * @brief Reset all changes
     */
    void reset();

private:
    clang::Rewriter rewriter_;
    std::vector<clang::SourceRange> modified_ranges_;
    
    bool rangesOverlap(clang::SourceRange range1, clang::SourceRange range2) const;
};

} // namespace optiweave::core