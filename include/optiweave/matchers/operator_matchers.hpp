#pragma once

#include <clang/ASTMatchers/ASTMatchers.h>
#include <vector>

namespace optiweave::matchers {

/**
 * @brief Types of operators that can be matched
 */
enum class MatcherType {
    ArraySubscript,
    ArithmeticOperator,
    AssignmentOperator,
    ComparisonOperator,
    UnaryOperator,
    OverloadedOperator
};

/**
 * @brief Factory for creating AST matchers for operators
 */
class OperatorMatchers {
public:
    /**
     * @brief Create matcher for array subscript expressions
     */
    static clang::ast_matchers::StatementMatcher arraySubscriptMatcher();
    
    /**
     * @brief Create matcher for arithmetic operators (+, -, *, /, %)
     */
    static clang::ast_matchers::StatementMatcher arithmeticOperatorMatcher();
    
    /**
     * @brief Create matcher for assignment operators (=, +=, -=, etc.)
     */
    static clang::ast_matchers::StatementMatcher assignmentOperatorMatcher();
    
    /**
     * @brief Create matcher for comparison operators (<, >, ==, !=, etc.)
     */
    static clang::ast_matchers::StatementMatcher comparisonOperatorMatcher();
    
    /**
     * @brief Create matcher for unary operators (++, --, +, -, !)
     */
    static clang::ast_matchers::StatementMatcher unaryOperatorMatcher();
    
    /**
     * @brief Create matcher for overloaded operators
     */
    static clang::ast_matchers::StatementMatcher overloadedOperatorMatcher();
    
    /**
     * @brief Create matcher for address-of expressions
     */
    static clang::ast_matchers::StatementMatcher addressOfMatcher();
    
    /**
     * @brief Create matcher for sizeof expressions
     */
    static clang::ast_matchers::StatementMatcher sizeofMatcher();
    
    /**
     * @brief Create matcher for template-dependent array subscripts
     */
    static clang::ast_matchers::StatementMatcher templateDependentArraySubscriptMatcher();
    
    /**
     * @brief Create matcher for template-dependent binary operators
     */
    static clang::ast_matchers::StatementMatcher templateDependentBinaryOperatorMatcher();
    
    /**
     * @brief Create matcher for template-dependent unary operators
     */
    static clang::ast_matchers::StatementMatcher templateDependentUnaryOperatorMatcher();
    
    /**
     * @brief Create matcher for any template-dependent operator
     */
    static clang::ast_matchers::StatementMatcher templateDependentOperatorMatcher();
    
    /**
     * @brief Create matcher for expressions in system headers
     */
    static clang::ast_matchers::StatementMatcher systemHeaderMatcher();
    
    /**
     * @brief Create combined matcher from multiple types
     */
    static clang::ast_matchers::StatementMatcher createCombinedMatcher(
        const std::vector<MatcherType> &matcher_types,
        bool skip_system_headers = true,
        bool skip_template_dependent = false);
};

} // namespace optiweave::matchers