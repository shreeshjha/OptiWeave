#include "../../include/optiweave/analysis/operator_detector.hpp"
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/Support/raw_ostream.h>

namespace optiweave::analysis {

OperatorDetector::OperatorDetector(clang::ASTContext &context)
    : context_(context) {}

void OperatorDetector::analyzeTranslationUnit(clang::TranslationUnitDecl *decl) {
    // Reset statistics
    stats_ = DetectionStats{};
    
    // Traverse the translation unit
    TraverseDecl(decl);
}

bool OperatorDetector::VisitArraySubscriptExpr(clang::ArraySubscriptExpr *expr) {
    if (shouldAnalyzeExpression(expr)) {
        ++stats_.array_subscript_count;
        
        // Analyze the array type
        auto lhs_type = expr->getLHS()->getType();
        if (lhs_type->isArrayType()) {
            ++stats_.native_array_count;
        } else if (lhs_type->isPointerType()) {
            ++stats_.pointer_access_count;
        }
        
        // Check if template dependent
        if (isTemplateDependentExpression(expr)) {
            ++stats_.template_dependent_count;
        }
    }
    return true;
}

bool OperatorDetector::VisitBinaryOperator(clang::BinaryOperator *expr) {
    if (shouldAnalyzeExpression(expr)) {
        if (expr->isArithmeticOp()) {
            ++stats_.arithmetic_operator_count;
        } else if (expr->isAssignmentOp()) {
            ++stats_.assignment_operator_count;
        } else if (expr->isComparisonOp()) {
            ++stats_.comparison_operator_count;
        }
        
        // Check if template dependent
        if (isTemplateDependentExpression(expr)) {
            ++stats_.template_dependent_count;
        }
    }
    return true;
}

bool OperatorDetector::VisitUnaryOperator(clang::UnaryOperator *expr) {
    if (shouldAnalyzeExpression(expr)) {
        ++stats_.unary_operator_count;
        
        // Check if template dependent
        if (isTemplateDependentExpression(expr)) {
            ++stats_.template_dependent_count;
        }
    }
    return true;
}

bool OperatorDetector::VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *expr) {
    if (shouldAnalyzeExpression(expr)) {
        ++stats_.overloaded_operator_count;
        
        // Check if template dependent
        if (isTemplateDependentExpression(expr)) {
            ++stats_.template_dependent_count;
        }
    }
    return true;
}

const DetectionStats& OperatorDetector::getStats() const {
    return stats_;
}

void OperatorDetector::printStats(llvm::raw_ostream &os) const {
    os << "Operator Detection Statistics:\n";
    os << "  Array subscripts: " << stats_.array_subscript_count << "\n";
    os << "    Native arrays: " << stats_.native_array_count << "\n";
    os << "    Pointer access: " << stats_.pointer_access_count << "\n";
    os << "  Arithmetic operators: " << stats_.arithmetic_operator_count << "\n";
    os << "  Assignment operators: " << stats_.assignment_operator_count << "\n";
    os << "  Comparison operators: " << stats_.comparison_operator_count << "\n";
    os << "  Unary operators: " << stats_.unary_operator_count << "\n";
    os << "  Overloaded operators: " << stats_.overloaded_operator_count << "\n";
    os << "  Template dependent: " << stats_.template_dependent_count << "\n";
    os << "  System header expressions: " << stats_.system_header_count << "\n";
}

bool OperatorDetector::shouldAnalyzeExpression(const clang::Expr *expr) const {
    auto &source_manager = context_.getSourceManager();
    auto location = expr->getBeginLoc();
    
    // Skip invalid locations
    if (location.isInvalid()) {
        return false;
    }
    
    // Count system header expressions separately
    if (source_manager.isInSystemHeader(location)) {
        const_cast<OperatorDetector*>(this)->stats_.system_header_count++;
        return false; // Don't analyze system headers by default
    }
    
    return true;
}

bool OperatorDetector::isTemplateDependentExpression(const clang::Expr *expr) const {
    if (!expr) return false;
    
    // Check if the expression type is template dependent
    if (expr->getType()->isDependentType()) {
        return true;
    }
    
    // Check if any operands are template dependent
    if (const auto *binary_op = clang::dyn_cast<clang::BinaryOperator>(expr)) {
        return isTemplateDependentExpression(binary_op->getLHS()) ||
               isTemplateDependentExpression(binary_op->getRHS());
    }
    
    if (const auto *unary_op = clang::dyn_cast<clang::UnaryOperator>(expr)) {
        return isTemplateDependentExpression(unary_op->getSubExpr());
    }
    
    if (const auto *array_sub = clang::dyn_cast<clang::ArraySubscriptExpr>(expr)) {
        return isTemplateDependentExpression(array_sub->getBase()) ||
               isTemplateDependentExpression(array_sub->getIdx());
    }
    
    return false;
}

} // namespace optiweave::analysis