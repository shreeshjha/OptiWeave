#include "../../include/optiweave/matchers/type_matchers.hpp"
#include <clang/ASTMatchers/ASTMatchers.h>

namespace optiweave::matchers {

using namespace clang::ast_matchers;

clang::ast_matchers::TypeMatcher TypeMatchers::dependentTypeMatcher() {
    return qualType(isDependentType());
}

clang::ast_matchers::TypeMatcher TypeMatchers::pointerTypeMatcher() {
    return qualType(isPointerType());
}

clang::ast_matchers::TypeMatcher TypeMatchers::arrayTypeMatcher() {
    return qualType(isArrayType());
}

clang::ast_matchers::TypeMatcher TypeMatchers::integralTypeMatcher() {
    return qualType(isIntegralType());
}

clang::ast_matchers::TypeMatcher TypeMatchers::floatingTypeMatcher() {
    return qualType(isFloatingType());
}

clang::ast_matchers::TypeMatcher TypeMatchers::arithmeticTypeMatcher() {
    return qualType(isArithmeticType());
}

clang::ast_matchers::TypeMatcher TypeMatchers::templateSpecializationMatcher() {
    return qualType(hasDeclaration(
        classTemplateSpecializationDecl()
    ));
}

clang::ast_matchers::TypeMatcher TypeMatchers::builtinTypeMatcher() {
    return qualType(builtinType());
}

clang::ast_matchers::TypeMatcher TypeMatchers::constTypeMatcher() {
    return qualType(isConstQualified());
}

clang::ast_matchers::TypeMatcher TypeMatchers::volatileTypeMatcher() {
    return qualType(isVolatileQualified());
}

bool TypeMatchers::isPointerLikeType(clang::QualType type) {
    return type->isPointerType() || type->isArrayType() || 
           type->isReferenceType();
}

bool TypeMatchers::isTemplateDependentType(clang::QualType type) {
    return type->isDependentType() || 
           type->isInstantiationDependentType() || 
           type->isTemplateTypeParmType();
}

bool TypeMatchers::isArithmeticType(clang::QualType type) {
    return type->isArithmeticType();
}

bool TypeMatchers::isIntegralType(clang::QualType type) {
    return type->isIntegralType(clang::ASTContext::getASTContext());
}

bool TypeMatchers::hasOperatorOverload(clang::QualType type, 
                                     const std::string &operator_name) {
    // This would require more complex analysis of the type's methods
    // For now, return false as a placeholder
    return false;
}

} // namespace optiweave::matchers