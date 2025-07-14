#include "../../include/optiweave/utils/source_utils.hpp"
#include <clang/Lex/Lexer.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>

namespace optiweave::utils {

std::string getSourceText(clang::SourceRange range, 
                         const clang::SourceManager &source_manager,
                         const clang::LangOptions &lang_opts) {
    if (range.isInvalid()) {
        return "";
    }
    
    auto char_range = clang::CharSourceRange::getTokenRange(range);
    bool invalid = false;
    auto text = clang::Lexer::getSourceText(char_range, source_manager, 
                                           lang_opts, &invalid);
    
    if (invalid) {
        return "";
    }
    
    return text.str();
}

bool isInSystemHeader(clang::SourceLocation location,
                     const clang::SourceManager &source_manager) {
    return source_manager.isInSystemHeader(location);
}

bool isValidLocation(clang::SourceLocation location) {
    return location.isValid() && !location.isInvalid();
}

std::string getFileName(clang::SourceLocation location,
                       const clang::SourceManager &source_manager) {
    if (!isValidLocation(location)) {
        return "";
    }
    
    auto file_entry = source_manager.getFileEntryForLoc(location);
    if (!file_entry) {
        return "";
    }
    
    return file_entry->getName().str();
}

unsigned getLineNumber(clang::SourceLocation location,
                      const clang::SourceManager &source_manager) {
    if (!isValidLocation(location)) {
        return 0;
    }
    
    return source_manager.getExpansionLineNumber(location);
}

unsigned getColumnNumber(clang::SourceLocation location,
                        const clang::SourceManager &source_manager) {
    if (!isValidLocation(location)) {
        return 0;
    }
    
    return source_manager.getExpansionColumnNumber(location);
}

bool rangesOverlap(clang::SourceRange range1, clang::SourceRange range2,
                  const clang::SourceManager &source_manager) {
    if (range1.isInvalid() || range2.isInvalid()) {
        return false;
    }
    
    auto begin1 = source_manager.getFileOffset(range1.getBegin());
    auto end1 = source_manager.getFileOffset(range1.getEnd());
    auto begin2 = source_manager.getFileOffset(range2.getBegin());
    auto end2 = source_manager.getFileOffset(range2.getEnd());
    
    return !(end1 <= begin2 || begin1 >= end2);
}

std::string formatLocation(clang::SourceLocation location,
                          const clang::SourceManager &source_manager) {
    if (!isValidLocation(location)) {
        return "<invalid location>";
    }
    
    auto filename = getFileName(location, source_manager);
    auto line = getLineNumber(location, source_manager);
    auto column = getColumnNumber(location, source_manager);
    
    return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
}

bool isMainFile(clang::SourceLocation location,
               const clang::SourceManager &source_manager) {
    if (!isValidLocation(location)) {
        return false;
    }
    
    auto file_id = source_manager.getFileID(location);
    return file_id == source_manager.getMainFileID();
}

} // namespace optiweave::utils