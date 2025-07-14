#include "../../include/optiweave/matchers/operator_matchers.hpp"

using namespace clang::ast_matchers;

namespace optiweave::matchers {

clang::ast_matchers::StatementMatcher OperatorMatchers::arraySubscriptMatcher() {
  return arraySubscriptExpr(
    unless(isExpansionInSystemHeader()),
    unless(hasParent(unaryOperator(hasOperatorName("&")))),
    unless(hasParent(unaryExprOrTypeTraitExpr()))
  ).bind("arraySubscript");
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
    unless(isExpansionInSystemHeader()),
    unless(hasParent(unaryExprOrTypeTraitExpr()))
  ).bind("arithmeticOp");
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
  ).bind("assignmentOp");
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
  ).bind("comparisonOp");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::overloadedOperatorMatcher() {
  return cxxOperatorCallExpr(
    anyOf(
      hasOverloadedOperatorName("[]"),
      hasOverloadedOperatorName("+"),
      hasOverloadedOperatorName("-"),
      hasOverloadedOperatorName("*"),
      hasOverloadedOperatorName("/"),
      hasOverloadedOperatorName("%"),
      hasOverloadedOperatorName("="),
      hasOverloadedOperatorName("+="),
      hasOverloadedOperatorName("-="),
      hasOverloadedOperatorName("*="),
      hasOverloadedOperatorName("/="),
      hasOverloadedOperatorName("%=")
    ),
    unless(isExpansionInSystemHeader())
  ).bind("overloadedOp");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::unaryOperatorMatcher() {
  return unaryOperator(
    anyOf(
      hasOperatorName("++"),
      hasOperatorName("--"),
      hasOperatorName("-"),
      hasOperatorName("+")
    ),
    unless(isExpansionInSystemHeader()),
    unless(hasParent(unaryExprOrTypeTraitExpr()))
  ).bind("unaryOp");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::systemHeaderOperatorMatcher() {
  return anyOf(
    arraySubscriptExpr(isExpansionInSystemHeader()),
    binaryOperator(isExpansionInSystemHeader()),
    unaryOperator(isExpansionInSystemHeader()),
    cxxOperatorCallExpr(isExpansionInSystemHeader())
  ).bind("systemHeaderOp");
}

clang::ast_matchers::StatementMatcher OperatorMatchers::templateDependentOperatorMatcher() {
  return anyOf(
    arraySubscriptExpr(
      anyOf(
        hasLHS(expr(hasType(qualType(isDependentType())))),
        hasRHS(expr(hasType(qualType(isDependentType()))))
      )
    ),
    binaryOperator(
      anyOf(
        hasLHS(expr(hasType(qualType(isDependentType())))),
        hasRHS(expr(hasType(qualType(isDependentType()))))
      )
    ),
    unaryOperator(
      hasUnaryOperand(expr(hasType(qualType(isDependentType()))))
    )
  ).bind("templateDependentOp");
}

// ArraySubscriptMatchCallback implementation
ArraySubscriptMatchCallback::ArraySubscriptMatchCallback(
    std::function<void(const clang::ArraySubscriptExpr*)> callback)
    : callback_(std::move(callback)) {}

void ArraySubscriptMatchCallback::run(const clang::ast_matchers::MatchFinder::MatchResult &result) {
  if (const auto *expr = result.Nodes.getNodeAs<clang::ArraySubscriptExpr>("arraySubscript")) {
    callback_(expr);
  }
}

// BinaryOperatorMatchCallback implementation
BinaryOperatorMatchCallback::BinaryOperatorMatchCallback(
    std::function<void(const clang::BinaryOperator*)> callback)
    : callback_(std::move(callback)) {}

void BinaryOperatorMatchCallback::run(const clang::ast_matchers::MatchFinder::MatchResult &result) {
  if (const auto *expr = result.Nodes.getNodeAs<clang::BinaryOperator>("arithmeticOp")) {
    callback_(expr);
  } else if (const auto *expr = result.Nodes.getNodeAs<clang::BinaryOperator>("assignmentOp")) {
    callback_(expr);
  } else if (const auto *expr = result.Nodes.getNodeAs<clang::BinaryOperator>("comparisonOp")) {
    callback_(expr);
  }
}

// OverloadedOperatorMatchCallback implementation
OverloadedOperatorMatchCallback::OverloadedOperatorMatchCallback(
    std::function<void(const clang::CXXOperatorCallExpr*)> callback)
    : callback_(std::move(callback)) {}

void OverloadedOperatorMatchCallback::run(const clang::ast_matchers::MatchFinder::MatchResult &result) {
  if (const auto *expr = result.Nodes.getNodeAs<clang::CXXOperatorCallExpr>("overloadedOp")) {
    callback_(expr);
  }
}

// OperatorMatcherBuilder implementation
OperatorMatcherBuilder::OperatorMatcherBuilder() {}

OperatorMatcherBuilder& OperatorMatcherBuilder::withArraySubscripts() {
  matchers_.push_back(OperatorMatchers::arraySubscriptMatcher());
  return *this;
}

OperatorMatcherBuilder& OperatorMatcherBuilder::withArithmeticOperators() {
  matchers_.push_back(OperatorMatchers::arithmeticOperatorMatcher());
  return *this;
}

OperatorMatcherBuilder& OperatorMatcherBuilder::withAssignmentOperators() {
  matchers_.push_back(OperatorMatchers::assignmentOperatorMatcher());
  return *this;
}

OperatorMatcherBuilder& OperatorMatcherBuilder::withComparisonOperators() {
  matchers_.push_back(OperatorMatchers::comparisonOperatorMatcher());
  return *this;
}

OperatorMatcherBuilder& OperatorMatcherBuilder::excludeSystemHeaders() {
  exclude_system_headers_ = true;
  return *this;
}

OperatorMatcherBuilder& OperatorMatcherBuilder::excludeTemplateDependentExpressions() {
  exclude_template_dependent_ = true;
  return *this;
}

clang::ast_matchers::StatementMatcher OperatorMatcherBuilder::build() {
  if (matchers_.empty()) {
    return stmt(); // Match nothing specific if no matchers added
  }

  auto combined = anyOf(matchers_);

  // Apply exclusions
  if (exclude_system_headers_) {
    combined = allOf(combined, unless(isExpansionInSystemHeader()));
  }

  if (exclude_template_dependent_) {
    combined = allOf(combined, unless(OperatorMatchers::templateDependentOperatorMatcher()));
  }

  return combined;
}

} // namespace optiweave::matchers