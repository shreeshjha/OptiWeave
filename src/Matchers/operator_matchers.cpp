#include "../../include/optiweave/matchers/operator_matchers.hpp"
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

namespace clang {
    namespace ast_matchers {
        AST_MATCHER(QualType, isDependentType) {
            return Node->isDependentType();
        }
    }
}

namespace optiweave::matchers {

clang::ast_matchers::StatementMatcher OperatorMatchers::arraySubscriptMatcher() {
    return arraySubscriptExpr(
        unless(isExpansionInSystemHeader())
    ).bind("array_subscript");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::arithmeticOperatorMatcher() {
    return binaryOperator(
        anyOf(
            hasOperatorName("+"),
            hasOperatorName("-"),
            hasOperatorName("*"),
            hasOperatorName("/"),
            hasOperatorName("%")
        ),
        unless(isExpansionInSystemHeader())
    ).bind("arithmetic_op");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::assignmentOperatorMatcher() {
    return binaryOperator(
        anyOf(
            hasOperatorName("="),
            hasOperatorName("+="),
            hasOperatorName("-="),
            hasOperatorName("*="),
            hasOperatorName("/="),
            hasOperatorName("%=")
        ),
        unless(isExpansionInSystemHeader())
    ).bind("assignment_op");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::comparisonOperatorMatcher() {
    return binaryOperator(
        anyOf(
            hasOperatorName("=="),
            hasOperatorName("!="),
            hasOperatorName("<"),
            hasOperatorName(">"),
            hasOperatorName("<="),
            hasOperatorName(">=")
        ),
        unless(isExpansionInSystemHeader())
    ).bind("comparison_op");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::unaryOperatorMatcher() {
    return unaryOperator(
        anyOf(
            hasOperatorName("++"),
            hasOperatorName("--"),
            hasOperatorName("+"),
            hasOperatorName("-"),
            hasOperatorName("!")
        ),
        unless(isExpansionInSystemHeader())
    ).bind("unary_op");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::overloadedOperatorMatcher() {
    return cxxOperatorCallExpr(
        unless(isExpansionInSystemHeader())
    ).bind("overloaded_op");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::addressOfMatcher() {
    return unaryOperator(
        hasOperatorName("&"),
        unless(isExpansionInSystemHeader())
    ).bind("address_of");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::sizeofMatcher() {
    return unaryExprOrTypeTraitExpr(
        ofKind(clang::UETT_SizeOf),
        unless(isExpansionInSystemHeader())
    ).bind("sizeof_expr");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::templateDependentArraySubscriptMatcher() {
    return arraySubscriptExpr(
        hasBase(expr(hasType(qualType(isDependentType())))),
        unless(isExpansionInSystemHeader())
    ).bind("template_array_subscript");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::templateDependentBinaryOperatorMatcher() {
    return binaryOperator(
        anyOf(
            hasLHS(expr(hasType(qualType(isDependentType())))),
            hasRHS(expr(hasType(qualType(isDependentType()))))
        ),
        unless(isExpansionInSystemHeader())
    ).bind("template_binary_op");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::templateDependentUnaryOperatorMatcher() {
    return unaryOperator(
        hasUnaryOperand(expr(hasType(qualType(isDependentType())))),
        unless(isExpansionInSystemHeader())
    ).bind("template_unary_op");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::templateDependentOperatorMatcher() {
    return stmt(
        anyOf(
            templateDependentArraySubscriptMatcher(),
            templateDependentBinaryOperatorMatcher(),
            templateDependentUnaryOperatorMatcher()
        )
    );
}

clang::ast_matchers::StatementMatcher OperatorMatchers::systemHeaderMatcher() {
    return stmt(isExpansionInSystemHeader());
}

clang::ast_matchers::StatementMatcher OperatorMatchers::createCombinedMatcher(
    const std::vector<MatcherType> &matcher_types,
    bool skip_system_headers,
    bool skip_template_dependent) {
    
    std::vector<clang::ast_matchers::StatementMatcher> matchers;
    
    // Add matchers based on requested types
    for (auto type : matcher_types) {
        switch (type) {
            case MatcherType::ArraySubscript:
                matchers.push_back(arraySubscriptMatcher());
                break;
            case MatcherType::ArithmeticOperator:
                matchers.push_back(arithmeticOperatorMatcher());
                break;
            case MatcherType::AssignmentOperator:
                matchers.push_back(assignmentOperatorMatcher());
                break;
            case MatcherType::ComparisonOperator:
                matchers.push_back(comparisonOperatorMatcher());
                break;
            case MatcherType::UnaryOperator:
                matchers.push_back(unaryOperatorMatcher());
                break;
            case MatcherType::OverloadedOperator:
                matchers.push_back(overloadedOperatorMatcher());
                break;
        }
    }
    
    if (matchers.empty()) {
        // Return a matcher that matches nothing
        return stmt(hasParent(stmt(unless(anything()))));
    }
    
    // Combine all matchers with anyOf
    clang::ast_matchers::StatementMatcher combined = matchers[0];
    for (size_t i = 1; i < matchers.size(); ++i) {
        combined = stmt(anyOf(combined, matchers[i]));
    }
    
    // Apply filters
    if (skip_system_headers) {
        combined = stmt(allOf(combined, unless(isExpansionInSystemHeader())));
    }
    
    if (skip_template_dependent) {
        combined = stmt(allOf(combined, unless(templateDependentOperatorMatcher())));
    }
    
    return combined;
}

} // namespace optiweave::matchers