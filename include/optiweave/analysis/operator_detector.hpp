// include/optiweave/analysis/operator_detector.hpp
#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTContext.h>
#include <llvm/Support/raw_ostream.h>

namespace optiweave::analysis {

struct DetectionStats {
    size_t array_subscript_count = 0;
    size_t native_array_count = 0;
    size_t pointer_access_count = 0;
    size_t arithmetic_operator_count = 0;
    size_t assignment_operator_count = 0;
    size_t comparison_operator_count = 0;
    size_t unary_operator_count = 0;
    size_t overloaded_operator_count = 0;
    size_t template_dependent_count = 0;
    size_t system_header_count = 0;
    
    void reset() { *this = DetectionStats{}; }
};

class OperatorDetector : public clang::RecursiveASTVisitor<OperatorDetector> {
public:
    explicit OperatorDetector(clang::ASTContext &context);
    
    void analyzeTranslationUnit(clang::TranslationUnitDecl *decl);
    
    bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr *expr);
    bool VisitBinaryOperator(clang::BinaryOperator *expr);
    bool VisitUnaryOperator(clang::UnaryOperator *expr);
    bool VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *expr);
    
    const DetectionStats& getStats() const;
    void printStats(llvm::raw_ostream &os) const;

private:
    clang::ASTContext &context_;
    DetectionStats stats_;
    
    bool shouldAnalyzeExpression(const clang::Expr *expr) const;
    bool isTemplateDependentExpression(const clang::Expr *expr) const;
};

// Placeholder structures for test compatibility
struct OperatorStatistics {
    size_t total_array_subscripts = 0;
    size_t total_arithmetic_ops = 0;
    size_t total_assignment_ops = 0;
    size_t total_comparison_ops = 0;
    size_t overloaded_operators = 0;
    size_t template_dependent_ops = 0;
    size_t system_header_ops = 0;
    
    void reset() { *this = OperatorStatistics{}; }
};

struct OperatorUsage {
    bool is_overloaded = false;
    bool is_template_dependent = false;
    bool in_system_header = false;
    std::string operator_name;
    std::string lhs_type;
    std::string rhs_type;
};

struct OperatorAnalysisResult {
    bool success = false;
    std::string error_message;
    std::vector<OperatorUsage> all_usages;
    std::vector<OperatorUsage> transformation_candidates;
    std::vector<std::string> recommendations;
};

} // namespace optiweave::analysis