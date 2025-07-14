#include "../../include/optiweave/utils/diagnostic_utils.hpp"
#include "../../include/optiweave/utils/source_utils.hpp"
#include <sstream>
#include <iomanip>

namespace optiweave::utils {

std::string DiagnosticMessage::format() const {
  std::ostringstream oss;
  
  // Add color codes if this were a real terminal output
  const char* level_prefix;
  switch (level) {
    case DiagnosticLevel::Note:
      level_prefix = "note";
      break;
    case DiagnosticLevel::Warning:
      level_prefix = "warning";
      break;
    case DiagnosticLevel::Error:
      level_prefix = "error";
      break;
    case DiagnosticLevel::Fatal:
      level_prefix = "fatal error";
      break;
  }
  
  // Format: file:line:column: level: message
  if (!file_name.empty() && line_number > 0) {
    oss << file_name << ":" << line_number << ":" << column_number 
        << ": " << level_prefix << ": " << message;
  } else {
    oss << level_prefix << ": " << message;
  }
  
  // Add source snippet if available
  if (!source_snippet.empty()) {
    oss << "\n" << source_snippet;
  }
  
  // Add fix hints if available
  for (const auto &hint : fix_hints) {
    oss << "\n  " << level_prefix << ": " << hint;
  }
  
  return oss.str();
}

// DiagnosticCollection implementation
void DiagnosticCollection::addDiagnostic(const DiagnosticMessage &diagnostic) {
  diagnostics_.push_back(diagnostic);
}

void DiagnosticCollection::addNote(const std::string &message, 
                                  clang::SourceLocation location) {
  DiagnosticMessage diag;
  diag.level = DiagnosticLevel::Note;
  diag.message = message;
  diag.location = location;
  addDiagnostic(diag);
}

void DiagnosticCollection::addWarning(const std::string &message,
                                     clang::SourceLocation location) {
  DiagnosticMessage diag;
  diag.level = DiagnosticLevel::Warning;
  diag.message = message;
  diag.location = location;
  addDiagnostic(diag);
}

void DiagnosticCollection::addError(const std::string &message,
                                   clang::SourceLocation location) {
  DiagnosticMessage diag;
  diag.level = DiagnosticLevel::Error;
  diag.message = message;
  diag.location = location;
  addDiagnostic(diag);
}

void DiagnosticCollection::addFatal(const std::string &message,
                                   clang::SourceLocation location) {
  DiagnosticMessage diag;
  diag.level = DiagnosticLevel::Fatal;
  diag.message = message;
  diag.location = location;
  addDiagnostic(diag);
}

size_t DiagnosticCollection::getErrorCount() const {
  size_t count = 0;
  for (const auto &diag : diagnostics_) {
    if (diag.isError()) {
      ++count;
    }
  }
  return count;
}

size_t DiagnosticCollection::getWarningCount() const {
  size_t count = 0;
  for (const auto &diag : diagnostics_) {
    if (diag.isWarning()) {
      ++count;
    }
  }
  return count;
}

void DiagnosticCollection::print(llvm::raw_ostream &os, bool colors) const {
  for (const auto &diag : diagnostics_) {
    os << diag.format() << "\n";
  }
}

std::string DiagnosticCollection::getSummary() const {
  size_t errors = getErrorCount();
  size_t warnings = getWarningCount();
  
  std::ostringstream oss;
  if (errors > 0 || warnings > 0) {
    oss << errors << " error(s), " << warnings << " warning(s) generated";
  } else {
    oss << "No issues found";
  }
  
  return oss.str();
}

// DiagnosticReporter implementation
DiagnosticReporter::DiagnosticReporter(clang::SourceManager *source_manager)
    : source_manager_(source_manager) {
  diagnostics_.source_manager_ = source_manager;
}

void DiagnosticReporter::reportNote(const std::string &message, 
                                   clang::SourceLocation location) {
  auto diag = createDiagnostic(DiagnosticLevel::Note, message, location);
  diagnostics_.addDiagnostic(diag);
}

void DiagnosticReporter::reportWarning(const std::string &message,
                                      clang::SourceLocation location) {
  auto diag = createDiagnostic(DiagnosticLevel::Warning, message, location);
  diagnostics_.addDiagnostic(diag);
}

void DiagnosticReporter::reportError(const std::string &message,
                                    clang::SourceLocation location) {
  auto diag = createDiagnostic(DiagnosticLevel::Error, message, location);
  diagnostics_.addDiagnostic(diag);
}

void DiagnosticReporter::reportFatal(const std::string &message,
                                    clang::SourceLocation location) {
  auto diag = createDiagnostic(DiagnosticLevel::Fatal, message, location);
  diagnostics_.addDiagnostic(diag);
}

void DiagnosticReporter::printDiagnostics(llvm::raw_ostream &os, bool colors) const {
  diagnostics_.print(os, colors);
}

DiagnosticMessage DiagnosticReporter::createDiagnostic(DiagnosticLevel level,
                                                      const std::string &message,
                                                      clang::SourceLocation location) {
  DiagnosticMessage diag;
  diag.level = level;
  diag.message = message;
  diag.location = location;
  
  if (source_manager_ && location.isValid()) {
    diag.file_name = SourceUtils::getFileName(location, *source_manager_);
    diag.line_number = SourceUtils::getLineNumber(location, *source_manager_);
    diag.column_number = SourceUtils::getColumnNumber(location, *source_manager_);
    diag.source_snippet = extractSourceSnippet(location);
  }
  
  return diag;
}

std::string DiagnosticReporter::extractSourceSnippet(clang::SourceLocation location,
                                                    unsigned context_lines) {
  if (!source_manager_ || !location.isValid()) {
    return "";
  }
  
  // Simplified implementation - extract the line containing the location
  auto file_id = source_manager_->getFileID(location);
  auto line_number = source_manager_->getSpellingLineNumber(location);
  auto column_number = source_manager_->getSpellingColumnNumber(location);
  
  bool invalid = false;
  auto buffer = source_manager_->getBufferData(file_id, &invalid);
  if (invalid) {
    return "";
  }
  
  // Find the start of the line
  auto line_start = source_manager_->translateLineCol(file_id, line_number, 1);
  if (line_start.isInvalid()) {
    return "";
  }
  
  auto offset = source_manager_->getFileOffset(line_start);
  
  // Extract the line
  std::string line;
  for (size_t i = offset; i < buffer.size() && buffer[i] != '\n'; ++i) {
    line += buffer[i];
  }
  
  // Add pointer to the problematic location
  std::string pointer_line(column_number - 1, ' ');
  pointer_line += "^";
  
  return line + "\n" + pointer_line;
}

// Specialized reporters implementation
namespace reporters {

void ASTVisitorReporter::reportTransformationError(const std::string &operation,
                                                   clang::SourceLocation location,
                                                   const std::string &details) {
  std::string message = diagnostic_utils::formatTransformationError(operation, "", 
                                                                   details.empty() ? "unknown reason" : details);
  reportError(message, location);
}

void ASTVisitorReporter::reportSkippedExpression(const std::string &reason,
                                                 clang::SourceLocation location) {
  std::string message = diagnostic_utils::formatSkippedOperation("expression transformation", reason);
  reportWarning(message, location);
}

void ASTVisitorReporter::reportTemplateInstantiationSkipped(clang::SourceLocation location) {
  reportNote("template instantiation skipped for instrumentation", location);
}

void ASTVisitorReporter::reportInvalidSourceRange(clang::SourceLocation location) {
  reportError("invalid source range encountered during transformation", location);
}

void RewriterReporter::reportReplacementConflict(clang::SourceLocation location,
                                                 const std::string &details) {
  std::string message = "replacement conflicts with existing modification";
  if (!details.empty()) {
    message += ": " + details;
  }
  reportError(message, location);
}

void RewriterReporter::reportInvalidReplacement(clang::SourceLocation location,
                                               const std::string &reason) {
  std::string message = "invalid replacement operation";
  if (!reason.empty()) {
    message += ": " + reason;
  }
  reportError(message, location);
}

void RewriterReporter::reportFileWriteError(const std::string &file_path,
                                           const std::string &error_message) {
  std::string message = "failed to write transformed file '" + file_path + "'";
  if (!error_message.empty()) {
    message += ": " + error_message;
  }
  reportError(message);
}

void TemplateAnalysisReporter::reportComplexTemplate(clang::SourceLocation location,
                                                     const std::string &template_name) {
  std::string message = "complex template '" + template_name + "' may require manual review";
  reportWarning(message, location);
}

void TemplateAnalysisReporter::reportSfinaeFailure(clang::SourceLocation location,
                                                   const std::string &details) {
  std::string message = "SFINAE detection failed";
  if (!details.empty()) {
    message += ": " + details;
  }
  reportWarning(message, location);
}

void TemplateAnalysisReporter::reportInstantiationFailure(clang::SourceLocation location,
                                                          const std::string &template_name) {
  std::string message = "template instantiation failed for '" + template_name + "'";
  reportError(message, location);
}

} // namespace reporters

// DiagnosticScope implementation
DiagnosticScope::DiagnosticScope(DiagnosticReporter &reporter, const std::string &scope_name)
    : reporter_(reporter), scope_name_(scope_name) {
  initial_error_count_ = reporter_.getDiagnostics().getErrorCount();
  initial_warning_count_ = reporter_.getDiagnostics().getWarningCount();
}

DiagnosticScope::~DiagnosticScope() {
  auto final_errors = reporter_.getDiagnostics().getErrorCount();
  auto final_warnings = reporter_.getDiagnostics().getWarningCount();
  
  auto new_errors = final_errors - initial_error_count_;
  auto new_warnings = final_warnings - initial_warning_count_;
  
  if (new_errors > 0 || new_warnings > 0) {
    std::ostringstream oss;
    oss << "Scope '" << scope_name_ << "' completed with " 
        << new_errors << " error(s) and " << new_warnings << " warning(s)";
    reporter_.reportNote(oss.str());
  }
}

void DiagnosticScope::addContext(const std::string &context) {
  reporter_.reportNote("Context: " + context);
}

// Utility functions implementation
namespace diagnostic_utils {

std::string formatTransformationError(const std::string &operation,
                                     const std::string &type_info,
                                     const std::string &reason) {
  std::ostringstream oss;
  oss << "transformation failed for " << operation;
  if (!type_info.empty()) {
    oss << " (type: " << type_info << ")";
  }
  if (!reason.empty()) {
    oss << ": " << reason;
  }
  return oss.str();
}

std::string formatSkippedOperation(const std::string &operation,
                                  const std::string &reason) {
  return "skipped " + operation + ": " + reason;
}

std::string formatSuggestion(const std::string &suggestion,
                            const std::vector<std::string> &steps) {
  std::ostringstream oss;
  oss << "suggestion: " << suggestion;
  
  if (!steps.empty()) {
    oss << "\n  Steps:";
    for (size_t i = 0; i < steps.size(); ++i) {
      oss << "\n    " << (i + 1) << ". " << steps[i];
    }
  }
  
  return oss.str();
}

std::string extractTypeInfo(clang::QualType type, clang::ASTContext &context) {
  if (type.isNull()) {
    return "unknown";
  }
  
  clang::PrintingPolicy policy(context.getLangOpts());
  policy.SuppressTagKeyword = true;
  return type.getAsString(policy);
}

std::string formatSourceLocation(clang::SourceLocation location,
                                clang::SourceManager &source_manager) {
  return SourceUtils::formatLocation(location, source_manager);
}

} // namespace diagnostic_utils

} // namespace optiweave::utils