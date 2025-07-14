#include "../../include/optiweave/analysis/template_analyzer.hpp"
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/Support/raw_ostream.h>

namespace optiweave::analysis {

TemplateAnalyzer::TemplateAnalyzer(clang::ASTContext &context)
    : context_(context) {}

void TemplateAnalyzer::analyzeTranslationUnit(clang::TranslationUnitDecl *decl) {
    // Reset statistics
    stats_ = TemplateStats{};
    
    // Traverse the translation unit
    TraverseDecl(decl);
}

bool TemplateAnalyzer::VisitFunctionTemplateDecl(clang::FunctionTemplateDecl *decl) {
    ++stats_.function_template_count;
    return true;
}

bool TemplateAnalyzer::VisitClassTemplateDecl(clang::ClassTemplateDecl *decl) {
    ++stats_.class_template_count;
    return true;
}

bool TemplateAnalyzer::VisitVarTemplateDecl(clang::VarTemplateDecl *decl) {
    ++stats_.variable_template_count;
    return true;
}

bool TemplateAnalyzer::VisitTemplateSpecializationType(clang::TemplateSpecializationType *type) {
    ++stats_.template_specialization_count;
    return true;
}

bool TemplateAnalyzer::VisitDependentScopeDeclRefExpr(clang::DependentScopeDeclRefExpr *expr) {
    ++stats_.dependent_name_count;
    return true;
}

bool TemplateAnalyzer::VisitCXXDependentScopeMemberExpr(clang::CXXDependentScopeMemberExpr *expr) {
    ++stats_.dependent_member_count;
    return true;
}

const TemplateStats& TemplateAnalyzer::getStats() const {
    return stats_;
}

void TemplateAnalyzer::printStats(llvm::raw_ostream &os) const {
    os << "Template Analysis Statistics:\n";
    os << "  Function templates: " << stats_.function_template_count << "\n";
    os << "  Class templates: " << stats_.class_template_count << "\n";
    os << "  Variable templates: " << stats_.variable_template_count << "\n";
    os << "  Template specializations: " << stats_.template_specialization_count << "\n";
    os << "  Dependent names: " << stats_.dependent_name_count << "\n";
    os << "  Dependent members: " << stats_.dependent_member_count << "\n";
    os << "  Template instantiations: " << stats_.template_instantiation_count << "\n";
}

bool TemplateAnalyzer::isTemplateDependentType(clang::QualType type) const {
    return type->isDependentType() || 
           type->isInstantiationDependentType() || 
           type->isTemplateTypeParmType() ||
           type->isUndeducedType();
}

bool TemplateAnalyzer::isInTemplateContext(const clang::Decl *decl) const {
    if (!decl) return false;
    
    const clang::DeclContext *context = decl->getDeclContext();
    while (context) {
        if (clang::isa<clang::FunctionTemplateDecl>(context) ||
            clang::isa<clang::ClassTemplateDecl>(context) ||
            clang::isa<clang::ClassTemplateSpecializationDecl>(context)) {
            return true;
        }
        context = context->getParent();
    }
    return false;
}

TemplateComplexity TemplateAnalyzer::assessComplexity(const clang::Decl *decl) const {
    if (!isInTemplateContext(decl)) {
        return TemplateComplexity::None;
    }
    
    // Simple heuristic based on template depth and dependent types
    // This could be enhanced with more sophisticated analysis
    
    if (const auto *func_template = clang::dyn_cast<clang::FunctionTemplateDecl>(decl)) {
        auto param_count = func_template->getTemplateParameters()->size();
        if (param_count > 3) return TemplateComplexity::High;
        if (param_count > 1) return TemplateComplexity::Medium;
        return TemplateComplexity::Low;
    }
    
    if (const auto *class_template = clang::dyn_cast<clang::ClassTemplateDecl>(decl)) {
        auto param_count = class_template->getTemplateParameters()->size();
        if (param_count > 3) return TemplateComplexity::High;
        if (param_count > 1) return TemplateComplexity::Medium;
        return TemplateComplexity::Low;
    }
    
    return TemplateComplexity::Low;
}

} // namespace optiweave::analysis