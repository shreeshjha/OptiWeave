#include <optiweave/matchers/type_matchers.hpp>
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>

using namespace clang::ast_matchers;

namespace clang {
    namespace ast_matchers {
        AST_MATCHER(QualType, isDependentType) {
            return Node->isDependentType();
        }
    }
}

namespace optiweave::matchers {



clang::ast_matchers::TypeMatcher TypeMatchers::dependentTypeMatcher() {
    return qualType(isDependentType());
}

clang::ast_matchers::TypeMatcher TypeMatchers::pointerTypeMatcher() {
    return qualType(pointerType());
}

clang::ast_matchers::TypeMatcher TypeMatchers::arrayTypeMatcher() {
    return qualType(arrayType());
}

clang::ast_matchers::TypeMatcher TypeMatchers::integralTypeMatcher() {
    return qualType(isInteger());
}

clang::ast_matchers::TypeMatcher TypeMatchers::floatingTypeMatcher() {
    return qualType(realFloatingPointType());
}

clang::ast_matchers::TypeMatcher TypeMatchers::arithmeticTypeMatcher() {
    return qualType(anyOf(isInteger(), realFloatingPointType()));
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
    // Use isIntegerType() which doesn't require ASTContext
    return type->isIntegerType();
}

bool TypeMatchers::hasOperatorOverload(clang::QualType type, 
                                     const std::string &operator_name) {
    // Get the canonical unqualified type
    clang::QualType canonicalType = type.getCanonicalType().getUnqualifiedType();
    
    // Handle reference types - look at the referenced type
    if (canonicalType->isReferenceType()) {
        canonicalType = canonicalType.getNonReferenceType();
    }
    
    // Handle pointer types - we generally don't have operator overloads on pointers
    if (canonicalType->isPointerType()) {
        return false;
    }
    
    // For builtin/arithmetic types, there are no custom operator overloads
    if (canonicalType->isBuiltinType() || canonicalType->isArithmeticType()) {
        return false;
    }
    
    // Get the CXXRecordDecl for class types
    const clang::CXXRecordDecl *recordDecl = canonicalType->getAsCXXRecordDecl();
    if (!recordDecl) {
        // Not a class type, check if it's a template type parameter
        if (canonicalType->isDependentType()) {
            // For dependent types, we can't know at compile time
            // Return true to be conservative (use SFINAE wrapper)
            return true;
        }
        return false;
    }
    
    // Map operator names to what Clang calls them
    // operator_name could be: "[]", "+", "-", "*", "/", "%", "=", "+=", etc.
    // or Clang's internal names like "operator[]", "operator+", etc.
    std::string searchName = operator_name;
    if (operator_name.find("operator") == std::string::npos) {
        searchName = "operator" + operator_name;
    }
    
    // Search for the operator in the class methods
    for (const auto *method : recordDecl->methods()) {
        if (const auto *opDecl = clang::dyn_cast<clang::CXXMethodDecl>(method)) {
            if (opDecl->isOverloadedOperator()) {
                // Get the overloaded operator kind
                clang::OverloadedOperatorKind opKind = opDecl->getOverloadedOperator();
                std::string opSpelling = clang::getOperatorSpelling(opKind);
                
                // Check if this matches what we're looking for
                if (operator_name == opSpelling || 
                    searchName == ("operator" + std::string(opSpelling))) {
                    return true;
                }
            }
        }
    }
    
    // Also check base classes
    if (recordDecl->hasDefinition()) {
        for (const auto &base : recordDecl->bases()) {
            clang::QualType baseType = base.getType();
            if (hasOperatorOverload(baseType, operator_name)) {
                return true;
            }
        }
    }
    
    return false;
}

} // namespace optiweave::matchers