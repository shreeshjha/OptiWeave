#pragma once

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/AST/Type.h>
#include <string>

namespace optiweave::matchers {

/**
 * @brief Factory for creating type-related AST matchers
 */
class TypeMatchers {
public:
    /**
     * @brief Create matcher for template-dependent types
     */
    static clang::ast_matchers::TypeMatcher dependentTypeMatcher();
    
    /**
     * @brief Create matcher for pointer types
     */
    static clang::ast_matchers::TypeMatcher pointerTypeMatcher();
    
    /**
     * @brief Create matcher for array types
     */
    static clang::ast_matchers::TypeMatcher arrayTypeMatcher();
    
    /**
     * @brief Create matcher for integral types
     */
    static clang::ast_matchers::TypeMatcher integralTypeMatcher();
    
    /**
     * @brief Create matcher for floating point types
     */
    static clang::ast_matchers::TypeMatcher floatingTypeMatcher();
    
    /**
     * @brief Create matcher for arithmetic types
     */
    static clang::ast_matchers::TypeMatcher arithmeticTypeMatcher();
    
    /**
     * @brief Create matcher for template specializations
     */
    static clang::ast_matchers::TypeMatcher templateSpecializationMatcher();
    
    /**
     * @brief Create matcher for builtin types
     */
    static clang::ast_matchers::TypeMatcher builtinTypeMatcher();
    
    /**
     * @brief Create matcher for const-qualified types
     */
    static clang::ast_matchers::TypeMatcher constTypeMatcher();
    
    /**
     * @brief Create matcher for volatile-qualified types
     */
    static clang::ast_matchers::TypeMatcher volatileTypeMatcher();
    
    /**
     * @brief Check if type is pointer-like (pointer, array, or reference)
     */
    static bool isPointerLikeType(clang::QualType type);
    
    /**
     * @brief Check if type is template-dependent
     */
    static bool isTemplateDependentType(clang::QualType type);
    
    /**
     * @brief Check if type is arithmetic
     */
    static bool isArithmeticType(clang::QualType type);
    
    /**
     * @brief Check if type is integral
     */
    static bool isIntegralType(clang::QualType type);
    
    /**
     * @brief Check if type has operator overload
     */
    static bool hasOperatorOverload(clang::QualType type, 
                                   const std::string &operator_name);
};

} // namespace optiweave::matchers