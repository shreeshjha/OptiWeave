#pragma once

#include <clang/Basic/SourceLocation.h>
#include <clang/AST/Type.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Expr.h>
#include <llvm/Support/raw_ostream.h>
#include <vector>
#include <string>

namespace optiweave::utils {

/**
 * @brief Diagnostic message levels
 */
enum class DiagnosticLevel {
    Error,
    Warning,
    Info,
    Note
};

/**
 * @brief Represents a diagnostic message
 */
struct DiagnosticMessage {
    DiagnosticLevel level;
    std::string message;
    clang::SourceLocation location;
};

/**
 * @brief Collects and manages diagnostic messages during transformation
 */
class DiagnosticCollector {
public:
    DiagnosticCollector();

    /**
     * @brief Add an error message
     * @param message Error message
     * @param location Source location (optional)
     */
    void addError(const std::string &message, 
                  clang::SourceLocation location = clang::SourceLocation());

    /**
     * @brief Add a warning message
     * @param message Warning message
     * @param location Source location (optional)
     */
    void addWarning(const std::string &message, 
                    clang::SourceLocation location = clang::SourceLocation());

    /**
     * @brief Add an info message
     * @param message Info message
     * @param location Source location (optional)
     */
    void addInfo(const std::string &message, 
                 clang::SourceLocation location = clang::SourceLocation());

    /**
     * @brief Add a note message
     * @param message Note message
     * @param location Source location (optional)
     */
    void addNote(const std::string &message, 
                 clang::SourceLocation location = clang::SourceLocation());

    /**
     * @brief Get number of error messages
     */
    size_t getErrorCount() const;

    /**
     * @brief Get number of warning messages
     */
    size_t getWarningCount() const;

    /**
     * @brief Get number of info messages
     */
    size_t getInfoCount() const;

    /**
     * @brief Get number of note messages
     */
    size_t getNoteCount() const;

    /**
     * @brief Get total number of messages
     */
    size_t getTotalCount() const;

    /**
     * @brief Get all diagnostic messages
     */
    const std::vector<DiagnosticMessage>& getMessages() const;

    /**
     * @brief Clear all diagnostic messages
     */
    void clear();

    /**
     * @brief Print all diagnostics to output stream
     * @param os Output stream
     * @param source_manager Source manager for location printing (optional)
     */
    void printDiagnostics(llvm::raw_ostream &os, 
                         const clang::SourceManager *source_manager = nullptr) const;

    /**
     * @brief Check if there are any errors
     */
    bool hasErrors() const;

    /**
     * @brief Check if there are any warnings
     */
    bool hasWarnings() const;

private:
    std::vector<DiagnosticMessage> messages_;
    size_t error_count_ = 0;
    size_t warning_count_ = 0;
    size_t info_count_ = 0;
    size_t note_count_ = 0;
};

// Utility functions for diagnostic information

/**
 * @brief Extract type information as string
 * @param type QualType to extract info from
 * @param context AST context
 * @return Type information string
 */
std::string extractTypeInfo(clang::QualType type, clang::ASTContext &context);

/**
 * @brief Format source location as string
 * @param location Source location
 * @param source_manager Source manager
 * @return Formatted location string
 */
std::string formatSourceLocation(clang::SourceLocation location, 
                                const clang::SourceManager &source_manager);

/**
 * @brief Get context information for an expression
 * @param expr Expression to analyze
 * @param context AST context
 * @return Context information string
 */
std::string getContextInfo(const clang::Expr *expr, clang::ASTContext &context);

} // namespace optiweave::utils