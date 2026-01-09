// include/optiweave/utils/source_utils.hpp
#pragma once

#include <clang/Basic/SourceManager.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/LangOptions.h>
#include <string>

namespace optiweave::utils {

/**
 * @brief Extract source text from a range
 */
std::string getSourceText(clang::SourceRange range, 
                         const clang::SourceManager &source_manager,
                         const clang::LangOptions &lang_opts);

/**
 * @brief Check if location is in system header
 */
bool isInSystemHeader(clang::SourceLocation location,
                     const clang::SourceManager &source_manager);

/**
 * @brief Check if location is valid
 */
bool isValidLocation(clang::SourceLocation location);

/**
 * @brief Get filename from location
 */
std::string getFileName(clang::SourceLocation location,
                       const clang::SourceManager &source_manager);

/**
 * @brief Get line number from location
 */
unsigned getLineNumber(clang::SourceLocation location,
                      const clang::SourceManager &source_manager);

/**
 * @brief Get column number from location
 */
unsigned getColumnNumber(clang::SourceLocation location,
                        const clang::SourceManager &source_manager);

/**
 * @brief Check if two ranges overlap
 */
bool rangesOverlap(clang::SourceRange range1, clang::SourceRange range2,
                  const clang::SourceManager &source_manager);

/**
 * @brief Format location as string
 */
std::string formatLocation(clang::SourceLocation location,
                          const clang::SourceManager &source_manager);

/**
 * @brief Check if location is in main file
 */
bool isMainFile(clang::SourceLocation location,
               const clang::SourceManager &source_manager);

} // namespace optiweave::utils

// include/optiweave/utils/diagnostic_utils.hpp
#pragma once

#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Type.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <vector>

namespace optiweave::utils {

enum class DiagnosticLevel {
    Error,
    Warning,
    Info,
    Note
};

struct DiagnosticMessage {
    DiagnosticLevel level;
    std::string message;
    clang::SourceLocation location;
};

/**
 * @brief Utility for collecting and formatting diagnostic messages
 */
class DiagnosticCollector {
public:
    DiagnosticCollector();
    
    void addError(const std::string &message, 
                  clang::SourceLocation location = clang::SourceLocation{});
    void addWarning(const std::string &message, 
                    clang::SourceLocation location = clang::SourceLocation{});
    void addInfo(const std::string &message, 
                 clang::SourceLocation location = clang::SourceLocation{});
    void addNote(const std::string &message, 
                 clang::SourceLocation location = clang::SourceLocation{});
    
    size_t getErrorCount() const;
    size_t getWarningCount() const;
    size_t getInfoCount() const;
    size_t getNoteCount() const;
    size_t getTotalCount() const;
    
    const std::vector<DiagnosticMessage>& getMessages() const;
    void clear();
    
    void printDiagnostics(llvm::raw_ostream &os, 
                         const clang::SourceManager *source_manager = nullptr) const;
    
    bool hasErrors() const;
    bool hasWarnings() const;

private:
    std::vector<DiagnosticMessage> messages_;
    size_t error_count_ = 0;
    size_t warning_count_ = 0;
    size_t info_count_ = 0;
    size_t note_count_ = 0;
};

/**
 * @brief Extract type information as string
 */
std::string extractTypeInfo(clang::QualType type, clang::ASTContext &context);

/**
 * @brief Format source location as string
 */
std::string formatSourceLocation(clang::SourceLocation location, 
                                const clang::SourceManager &source_manager);

/**
 * @brief Get context information for an expression
 */
std::string getContextInfo(const clang::Expr *expr, clang::ASTContext &context);

} // namespace optiweave::utils