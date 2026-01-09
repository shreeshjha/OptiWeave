#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ASTContext.h>
#include <llvm/Support/raw_ostream.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace optiweave::analysis {

struct TemplateStats {
    size_t function_template_count = 0;
    size_t class_template_count = 0;
    size_t variable_template_count = 0;
    size_t template_specialization_count = 0;
    size_t dependent_name_count = 0;
    size_t dependent_member_count = 0;
    size_t template_instantiation_count = 0;
    
    void reset() { *this = TemplateStats{}; }
};

enum class TemplateComplexity {
    None,
    Low,
    Medium,
    High
};

enum class TemplateTransformationStrategy {
    CompileTime,
    RuntimeCheck,
    SfinaeDetection,
    Hybrid
};

class TemplateAnalyzer : public clang::RecursiveASTVisitor<TemplateAnalyzer> {
public:
    explicit TemplateAnalyzer(clang::ASTContext &context);
    
    void analyzeTranslationUnit(clang::TranslationUnitDecl *decl);
    
    bool VisitFunctionTemplateDecl(clang::FunctionTemplateDecl *decl);
    bool VisitClassTemplateDecl(clang::ClassTemplateDecl *decl);
    bool VisitVarTemplateDecl(clang::VarTemplateDecl *decl);
    bool VisitTemplateSpecializationType(clang::TemplateSpecializationType *type);
    bool VisitDependentScopeDeclRefExpr(clang::DependentScopeDeclRefExpr *expr);
    bool VisitCXXDependentScopeMemberExpr(clang::CXXDependentScopeMemberExpr *expr);
    
    const TemplateStats& getStats() const;
    void printStats(llvm::raw_ostream &os) const;
    
    bool isTemplateDependentType(clang::QualType type) const;
    bool isInTemplateContext(const clang::Decl *decl) const;
    TemplateComplexity assessComplexity(const clang::Decl *decl) const;

private:
    clang::ASTContext &context_;
    TemplateStats stats_;
};

// Additional structures for test compatibility
struct TemplateStatistics {
    size_t total_template_functions = 0;
    size_t total_template_classes = 0;
    size_t total_template_instantiations = 0;
    size_t dependent_operator_usages = 0;
    size_t sfinae_candidates = 0;
    std::unordered_map<std::string, size_t> template_name_counts;
    std::unordered_map<std::string, size_t> argument_type_counts;
    
    void reset() { *this = TemplateStatistics{}; }
};

struct TemplateUsage {
    bool is_dependent = false;
    bool is_instantiation = false;
    bool has_operator_usage = false;
    std::string template_name;
    std::vector<std::string> template_arguments;
    std::string context_info;
};

struct TemplateTransformationRecommendation {
    TemplateTransformationStrategy strategy = TemplateTransformationStrategy::CompileTime;
    double confidence_score = 0.0;
    std::string template_name;
    std::vector<std::string> required_traits;
    std::string generated_code;
    std::string rationale;
};

struct TemplateAnalysisResult {
    bool success = false;
    std::string error_message;
    TemplateStatistics statistics;
    std::vector<TemplateUsage> all_usages;
    std::vector<TemplateUsage> sfinae_candidates;
    std::vector<TemplateUsage> dependent_operators;
    std::vector<TemplateTransformationRecommendation> recommendations;
};

} // namespace optiweave::analysis