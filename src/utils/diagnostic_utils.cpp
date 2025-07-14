#include "../../include/optiweave/utils/diagnostic_utils.hpp"
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>

namespace optiweave::utils {

DiagnosticCollector::DiagnosticCollector() = default;

void DiagnosticCollector::addError(const std::string &message, 
                                  clang::SourceLocation location) {
    DiagnosticMessage diag;
    diag.level = DiagnosticLevel::Error;
    diag.message = message;
    diag.location = location;
    messages_.push_back(diag);
    ++error_count_;
}

void DiagnosticCollector::addWarning(const std::string &message, 
                                    clang::SourceLocation location) {
    DiagnosticMessage diag;
    diag.level = DiagnosticLevel::Warning;
    diag.message = message;
    diag.location = location;
    messages_.push_back(diag);
    ++warning_count_;
}

void DiagnosticCollector::addInfo(const std::string &message, 
                                 clang::SourceLocation location) {
    DiagnosticMessage diag;
    diag.level = DiagnosticLevel::Info;
    diag.message = message;
    diag.location = location;
    messages_.push_back(diag);
    ++info_count_;
}

void DiagnosticCollector::addNote(const std::string &message, 
                                 clang::SourceLocation location) {
    DiagnosticMessage diag;
    diag.level = DiagnosticLevel::Note;
    diag.message = message;
    diag.location = location;
    messages_.push_back(diag);
    ++note_count_;
}

size_t DiagnosticCollector::getErrorCount() const {
    return error_count_;
}

size_t DiagnosticCollector::getWarningCount() const {
    return warning_count_;
}

size_t DiagnosticCollector::getInfoCount() const {
    return info_count_;
}

size_t DiagnosticCollector::getNoteCount() const {
    return note_count_;
}

size_t DiagnosticCollector::getTotalCount() const {
    return messages_.size();
}

const std::vector<DiagnosticMessage>& DiagnosticCollector::getMessages() const {
    return messages_;
}

void DiagnosticCollector::clear() {
    messages_.clear();
    error_count_ = 0;
    warning_count_ = 0;
    info_count_ = 0;
    note_count_ = 0;
}

void DiagnosticCollector::printDiagnostics(llvm::raw_ostream &os, 
                                          const clang::SourceManager *source_manager) const {
    for (const auto &msg : messages_) {
        // Print level
        switch (msg.level) {
            case DiagnosticLevel::Error:
                os << "error: ";
                break;
            case DiagnosticLevel::Warning:
                os << "warning: ";
                break;
            case DiagnosticLevel::Info:
                os << "info: ";
                break;
            case DiagnosticLevel::Note:
                os << "note: ";
                break;
        }
        
        // Print message
        os << msg.message;
        
        // Print location if available
        if (source_manager && msg.location.isValid()) {
            os << " at ";
            msg.location.print(os, *source_manager);
        }
        
        os << "\n";
    }
    
    // Print summary
    if (!messages_.empty()) {
        os << "\nDiagnostic Summary:\n";
        if (error_count_ > 0) {
            os << "  " << error_count_ << " error(s)\n";
        }
        if (warning_count_ > 0) {
            os << "  " << warning_count_ << " warning(s)\n";
        }
        if (info_count_ > 0) {
            os << "  " << info_count_ << " info message(s)\n";
        }
        if (note_count_ > 0) {
            os << "  " << note_count_ << " note(s)\n";
        }
    }
}

bool DiagnosticCollector::hasErrors() const {
    return error_count_ > 0;
}

bool DiagnosticCollector::hasWarnings() const {
    return warning_count_ > 0;
}

std::string extractTypeInfo(clang::QualType type, clang::ASTContext &context) {
    if (type.isNull()) {
        return "<null type>";
    }
    
    return type.getAsString(context.getPrintingPolicy());
}

std::string formatSourceLocation(clang::SourceLocation location, 
                                const clang::SourceManager &source_manager) {
    if (location.isInvalid()) {
        return "<invalid location>";
    }
    
    std::string result;
    llvm::raw_string_ostream os(result);
    location.print(os, source_manager);
    return os.str();
}

std::string getContextInfo(const clang::Expr *expr, clang::ASTContext &context) {
    if (!expr) {
        return "<null expression>";
    }
    
    std::string result = "Expression: ";
    result += expr->getStmtClassName();
    result += ", Type: ";
    result += extractTypeInfo(expr->getType(), context);
    
    return result;
}

} // namespace optiweave::utils