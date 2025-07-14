#include "../../include/optiweave/utils/source_utils.hpp"
#include <clang/AST/ASTContext.h>
#include <clang/AST/ParentMapContext.h>
#include <sstream>
#include <algorithm>

namespace optiweave::utils {

std::string SourceUtils::getSourceText(const clang::SourceRange &range,
                                       const clang::SourceManager &source_manager,
                                       const clang::LangOptions &lang_options) {
  if (range.isInvalid()) {
    return "";
  }

  auto char_range = clang::CharSourceRange::getTokenRange(range);
  bool invalid = false;
  auto text = clang::Lexer::getSourceText(char_range, source_manager, lang_options, &invalid);
  
  if (invalid) {
    return "";
  }
  
  return text.str();
}

std::string SourceUtils::getExpressionText(const clang::Expr *expr,
                                          clang::ASTContext &context) {
  if (!expr) {
    return "";
  }
  
  return getSourceText(expr->getSourceRange(), 
                      context.getSourceManager(),
                      context.getLangOpts());
}

bool SourceUtils::isValidLocation(clang::SourceLocation location,
                                 const clang::SourceManager &source_manager) {
  return location.isValid() && location.isFileID();
}

std::string SourceUtils::getFileName(clang::SourceLocation location,
                                    const clang::SourceManager &source_manager) {
  if (!isValidLocation(location, source_manager)) {
    return "";
  }
  
  auto file_entry = source_manager.getFileEntryForLoc(location);
  if (file_entry) {
    return file_entry->getName().str();
  }
  
  return "";
}

unsigned SourceUtils::getLineNumber(clang::SourceLocation location,
                                   const clang::SourceManager &source_manager) {
  if (!isValidLocation(location, source_manager)) {
    return 0;
  }
  
  return source_manager.getSpellingLineNumber(location);
}

unsigned SourceUtils::getColumnNumber(clang::SourceLocation location,
                                     const clang::SourceManager &source_manager) {
  if (!isValidLocation(location, source_manager)) {
    return 0;
  }
  
  return source_manager.getSpellingColumnNumber(location);
}

std::string SourceUtils::formatLocation(clang::SourceLocation location,
                                       const clang::SourceManager &source_manager) {
  if (!isValidLocation(location, source_manager)) {
    return "unknown:0:0";
  }
  
  std::string file_name = getFileName(location, source_manager);
  unsigned line = getLineNumber(location, source_manager);
  unsigned column = getColumnNumber(location, source_manager);
  
  return file_name + ":" + std::to_string(line) + ":" + std::to_string(column);
}

bool SourceUtils::isInSystemHeader(clang::SourceLocation location,
                                  const clang::SourceManager &source_manager) {
  return source_manager.isInSystemHeader(location);
}

bool SourceUtils::isInMainFile(clang::SourceLocation location,
                              const clang::SourceManager &source_manager) {
  return source_manager.isInMainFile(location);
}

const clang::FunctionDecl* SourceUtils::getContainingFunction(clang::SourceLocation location,
                                                             clang::ASTContext &context) {
  // This is a simplified implementation - a full version would need more sophisticated traversal
  auto &source_manager = context.getSourceManager();
  
  if (!isValidLocation(location, source_manager)) {
    return nullptr;
  }
  
  // For a complete implementation, we'd need to traverse the AST and find the enclosing function
  // This would require additional infrastructure
  return nullptr;
}

clang::SourceLocation SourceUtils::expandMacroLocation(clang::SourceLocation location,
                                                      const clang::SourceManager &source_manager) {
  if (location.isMacroID()) {
    return source_manager.getExpansionLoc(location);
  }
  return location;
}

bool SourceUtils::rangesOverlap(const clang::SourceRange &range1,
                               const clang::SourceRange &range2,
                               const clang::SourceManager &source_manager) {
  if (range1.isInvalid() || range2.isInvalid()) {
    return false;
  }
  
  auto begin1 = range1.getBegin();
  auto end1 = range1.getEnd();
  auto begin2 = range2.getBegin();
  auto end2 = range2.getEnd();
  
  // Check if ranges are in the same file
  if (source_manager.getFileID(begin1) != source_manager.getFileID(begin2)) {
    return false;
  }
  
  // Check for overlap: ranges overlap if neither is completely before the other
  return !(source_manager.isBeforeInTranslationUnit(end1, begin2) ||
           source_manager.isBeforeInTranslationUnit(end2, begin1));
}

clang::SourceRange SourceUtils::combineRanges(const clang::SourceRange &range1,
                                             const clang::SourceRange &range2,
                                             const clang::SourceManager &source_manager) {
  if (range1.isInvalid()) return range2;
  if (range2.isInvalid()) return range1;
  
  auto begin1 = range1.getBegin();
  auto end1 = range1.getEnd();
  auto begin2 = range2.getBegin();
  auto end2 = range2.getEnd();
  
  // Find the earliest begin and latest end
  auto combined_begin = source_manager.isBeforeInTranslationUnit(begin1, begin2) ? begin1 : begin2;
  auto combined_end = source_manager.isBeforeInTranslationUnit(end1, end2) ? end2 : end1;
  
  return clang::SourceRange(combined_begin, combined_end);
}

unsigned SourceUtils::getByteOffset(clang::SourceLocation location,
                                   const clang::SourceManager &source_manager) {
  if (!isValidLocation(location, source_manager)) {
    return 0;
  }
  
  return source_manager.getFileOffset(location);
}

clang::SourceLocation SourceUtils::getLocationFromOffset(clang::FileID file_id,
                                                        unsigned offset,
                                                        const clang::SourceManager &source_manager) {
  return source_manager.getLocForStartOfFile(file_id).getLocWithOffset(offset);
}

// ReplacementRangeHelper implementation
clang::SourceRange ReplacementRangeHelper::getSafeReplacementRange(const clang::Expr *expr,
                                                                  clang::ASTContext &context) {
  if (!expr) {
    return clang::SourceRange();
  }
  
  auto range = expr->getSourceRange();
  return adjustRangeForSafety(range, context);
}

clang::SourceRange ReplacementRangeHelper::adjustRangeForSafety(const clang::SourceRange &range,
                                                               clang::ASTContext &context) {
  if (range.isInvalid()) {
    return range;
  }
  
  auto &source_manager = context.getSourceManager();
  auto &lang_options = context.getLangOpts();
  
  // Get token-aligned range to ensure we don't cut off partial tokens
  return getTokenAlignedRange(range, source_manager, lang_options);
}

bool ReplacementRangeHelper::isSafeForReplacement(const clang::SourceRange &range,
                                                 clang::ASTContext &context) {
  if (range.isInvalid()) {
    return false;
  }
  
  auto &source_manager = context.getSourceManager();
  
  // Check if range is in main file (not system header)
  if (!SourceUtils::isInMainFile(range.getBegin(), source_manager)) {
    return false;
  }
  
  // Check if range is not in macro expansion
  if (range.getBegin().isMacroID() || range.getEnd().isMacroID()) {
    return false;
  }
  
  return true;
}

clang::SourceRange ReplacementRangeHelper::getTokenAlignedRange(const clang::SourceRange &range,
                                                               const clang::SourceManager &source_manager,
                                                               const clang::LangOptions &lang_options) {
  if (range.isInvalid()) {
    return range;
  }
  
  // Use Clang's Lexer to get precise token boundaries
  auto begin = range.getBegin();
  auto end = clang::Lexer::getLocForEndOfToken(range.getEnd(), 0, source_manager, lang_options);
  
  return clang::SourceRange(begin, end);
}

// FormattingUtils implementation
std::string FormattingUtils::preserveIndentation(const std::string &original_text,
                                                 const std::string &replacement_text) {
  if (original_text.empty() || replacement_text.empty()) {
    return replacement_text;
  }
  
  // Find the indentation of the first line
  size_t first_non_space = original_text.find_first_not_of(" \t");
  if (first_non_space == std::string::npos) {
    return replacement_text; // Original was all whitespace
  }
  
  std::string indentation = original_text.substr(0, first_non_space);
  
  return formatMultilineText(replacement_text, indentation);
}

std::string FormattingUtils::getIndentationAtLocation(clang::SourceLocation location,
                                                     const clang::SourceManager &source_manager) {
  if (!SourceUtils::isValidLocation(location, source_manager)) {
    return "";
  }
  
  // Get the start of the line
  auto file_id = source_manager.getFileID(location);
  auto line_number = source_manager.getSpellingLineNumber(location);
  auto line_start = source_manager.translateLineCol(file_id, line_number, 1);
  
  if (line_start.isInvalid()) {
    return "";
  }
  
  // Read from start of line until first non-whitespace character
  bool invalid = false;
  auto buffer = source_manager.getBufferData(file_id, &invalid);
  if (invalid) {
    return "";
  }
  
  auto offset = source_manager.getFileOffset(line_start);
  std::string indentation;
  
  for (size_t i = offset; i < buffer.size(); ++i) {
    char c = buffer[i];
    if (c == ' ' || c == '\t') {
      indentation += c;
    } else {
      break;
    }
  }
  
  return indentation;
}

std::string FormattingUtils::formatMultilineText(const std::string &text,
                                                const std::string &base_indentation) {
  std::istringstream iss(text);
  std::ostringstream oss;
  std::string line;
  bool first_line = true;
  
  while (std::getline(iss, line)) {
    if (!first_line) {
      oss << "\n" << base_indentation;
    }
    oss << line;
    first_line = false;
  }
  
  return oss.str();
}

std::string FormattingUtils::cleanWhitespace(const std::string &text) {
  std::string result = text;
  
  // Remove trailing whitespace from each line
  std::istringstream iss(result);
  std::ostringstream oss;
  std::string line;
  bool first_line = true;
  
  while (std::getline(iss, line)) {
    if (!first_line) {
      oss << "\n";
    }
    
    // Remove trailing whitespace
    auto end = line.find_last_not_of(" \t");
    if (end != std::string::npos) {
      oss << line.substr(0, end + 1);
    }
    
    first_line = false;
  }
  
  return oss.str();
}

// SourceTransformationContextBuilder implementation
SourceTransformationContextBuilder::SourceTransformationContextBuilder(const clang::Expr *expr,
                                                                       clang::ASTContext &context)
    : expr_(expr), context_(context) {}

SourceTransformationContext SourceTransformationContextBuilder::build() {
  if (expr_) {
    context_info_.original_range = expr_->getSourceRange();
    extractLocationInfo();
    extractSourceText();
    detectContext();
  }
  
  return context_info_;
}

SourceTransformationContextBuilder& 
SourceTransformationContextBuilder::withReplacementText(const std::string &text) {
  context_info_.replacement_text = text;
  return *this;
}

SourceTransformationContextBuilder& 
SourceTransformationContextBuilder::preserveFormatting() {
  preserve_formatting_ = true;
  return *this;
}

void SourceTransformationContextBuilder::extractLocationInfo() {
  if (!expr_ || context_info_.original_range.isInvalid()) {
    return;
  }
  
  auto &source_manager = context_.getSourceManager();
  auto location = context_info_.original_range.getBegin();
  
  context_info_.file_name = SourceUtils::getFileName(location, source_manager);
  context_info_.line_number = SourceUtils::getLineNumber(location, source_manager);
  context_info_.column_number = SourceUtils::getColumnNumber(location, source_manager);
  context_info_.in_system_header = SourceUtils::isInSystemHeader(location, source_manager);
  context_info_.in_macro_expansion = location.isMacroID();
}

void SourceTransformationContextBuilder::extractSourceText() {
  if (!expr_ || context_info_.original_range.isInvalid()) {
    return;
  }
  
  context_info_.original_text = SourceUtils::getExpressionText(expr_, context_);
  
  if (preserve_formatting_) {
    auto &source_manager = context_.getSourceManager();
    context_info_.indentation = FormattingUtils::getIndentationAtLocation(
        context_info_.original_range.getBegin(), source_manager);
  }
}

void SourceTransformationContextBuilder::detectContext() {
  if (!expr_) {
    return;
  }
  
  // Try to find containing function (simplified implementation)
  auto &source_manager = context_.getSourceManager();
  auto location = context_info_.original_range.getBegin();
  
  // In a full implementation, we would traverse the AST to find the containing function
  // For now, we'll use a placeholder
  context_info_.containing_function = "unknown_function";
}

} // namespace optiweave::utils