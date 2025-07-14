#include "../../include/optiweave/matchers/type_matchers.hpp"
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <sstream>

using namespace clang::ast_matchers;

namespace optiweave::matchers {

clang::ast_matchers::TypeMatcher TypeMatchers::arrayTypeMatcher() {
  return anyOf(
    constantArrayType(),
    incompleteArrayType(),
    variableArrayType(),
    dependentSizedArrayType()
  );
}

clang::ast_matchers::TypeMatcher TypeMatchers::pointerTypeMatcher() {
  return pointerType();
}

clang::ast_matchers::TypeMatcher TypeMatchers::arithmeticTypeMatcher() {
  return anyOf(
    builtinType(),
    enumType()
  );
}

clang::ast_matchers::TypeMatcher TypeMatchers::templateDependentTypeMatcher() {
  return anyOf(
    templateTypeParmType(),
    dependentNameType(),
    elaboratedType(hasQualifier(nestedNameSpecifier(isDependent()))),
    declRefExpr(hasType(qualType(isDependentType())))
  );
}

clang::ast_matchers::TypeMatcher TypeMatchers::overloadedOperatorTypeMatcher() {
  return recordType(hasDeclaration(
    cxxRecordDecl(hasMethod(
      cxxMethodDecl(anyOf(
        hasOverloadedOperatorName("[]"),
        hasOverloadedOperatorName("+"),
        hasOverloadedOperatorName("-"),
        hasOverloadedOperatorName("*"),
        hasOverloadedOperatorName("/"),
        hasOverloadedOperatorName("%")
      ))
    ))
  ));
}

clang::ast_matchers::TypeMatcher TypeMatchers::constTypeMatcher() {
  return qualType(isConstQualified());
}

clang::ast_matchers::TypeMatcher TypeMatchers::volatileTypeMatcher() {
  return qualType(isVolatileQualified());
}

clang::ast_matchers::TypeMatcher TypeMatchers::referenceTypeMatcher() {
  return anyOf(
    lValueReferenceType(),
    rValueReferenceType()
  );
}

// TypeAnalyzer implementation
bool TypeAnalyzer::isTransformableArrayType(clang::QualType type) {
  // Remove qualifiers and get canonical type
  auto canonical_type = type.getCanonicalType();
  
  // Check for array types
  if (canonical_type->isArrayType()) {
    return true;
  }
  
  // Check for pointer types (can be subscripted)
  if (canonical_type->isPointerType()) {
    return true;
  }
  
  // Check for class types with potential operator[] overloads
  if (canonical_type->isRecordType()) {
    // This would require deeper analysis to check for operator[] overloads
    return false; // Conservative approach - don't transform class types by default
  }
  
  return false;
}

bool TypeAnalyzer::hasOverloadedSubscriptOperator(clang::QualType type,
                                                  clang::ASTContext &context) {
  auto canonical_type = type.getCanonicalType();
  
  if (const auto *record_type = canonical_type->getAs<clang::RecordType>()) {
    if (const auto *cxx_record = clang::dyn_cast<clang::CXXRecordDecl>(record_type->getDecl())) {
      // Look for operator[] methods
      for (const auto *method : cxx_record->methods()) {
        if (method->isOverloadedOperator() && 
            method->getOverloadedOperator() == clang::OO_Subscript) {
          return true;
        }
      }
    }
  }
  
  return false;
}

bool TypeAnalyzer::hasOverloadedArithmeticOperators(clang::QualType type,
                                                    clang::ASTContext &context) {
  auto canonical_type = type.getCanonicalType();
  
  if (const auto *record_type = canonical_type->getAs<clang::RecordType>()) {
    if (const auto *cxx_record = clang::dyn_cast<clang::CXXRecordDecl>(record_type->getDecl())) {
      // Look for arithmetic operator methods
      for (const auto *method : cxx_record->methods()) {
        if (method->isOverloadedOperator()) {
          auto op = method->getOverloadedOperator();
          if (op == clang::OO_Plus || op == clang::OO_Minus || 
              op == clang::OO_Star || op == clang::OO_Slash || 
              op == clang::OO_Percent) {
            return true;
          }
        }
      }
    }
  }
  
  return false;
}

bool TypeAnalyzer::isTemplateDependentType(clang::QualType type) {
  return type->isDependentType() || 
         type->isInstantiationDependentType() ||
         type->isTemplateTypeParmType() || 
         type->isUndeducedType();
}

std::string TypeAnalyzer::getCanonicalTypeString(clang::QualType type,
                                                clang::ASTContext &context) {
  clang::PrintingPolicy policy(context.getLangOpts());
  policy.SuppressTagKeyword = true;
  policy.SuppressScope = false;
  policy.AnonymousTagLocations = false;
  
  return type.getCanonicalType().getAsString(policy);
}

bool TypeAnalyzer::areTypesCompatibleForBinaryOp(clang::QualType lhs_type,
                                                 clang::QualType rhs_type,
                                                 clang::ASTContext &context) {
  // Remove qualifiers for comparison
  auto lhs_canonical = lhs_type.getCanonicalType().getUnqualifiedType();
  auto rhs_canonical = rhs_type.getCanonicalType().getUnqualifiedType();
  
  // Same types are always compatible
  if (context.hasSameType(lhs_canonical, rhs_canonical)) {
    return true;
  }
  
  // Arithmetic types are generally compatible
  if (lhs_canonical->isArithmeticType() && rhs_canonical->isArithmeticType()) {
    return true;
  }
  
  // Pointer and array types with compatible element types
  if (lhs_canonical->isPointerType() && rhs_canonical->isPointerType()) {
    auto lhs_pointee = lhs_canonical->getPointeeType();
    auto rhs_pointee = rhs_canonical->getPointeeType();
    return context.hasSameType(lhs_pointee.getCanonicalType(), 
                              rhs_pointee.getCanonicalType());
  }
  
  return false;
}

bool TypeAnalyzer::isSafeForInstrumentation(clang::QualType type,
                                           clang::ASTContext &context) {
  auto canonical_type = type.getCanonicalType();
  
  // Avoid instrumenting volatile types (might have side effects)
  if (canonical_type.isVolatileQualified()) {
    return false;
  }
  
  // Avoid instrumenting function types
  if (canonical_type->isFunctionType()) {
    return false;
  }
  
  // Avoid instrumenting incomplete types
  if (canonical_type->isIncompleteType()) {
    return false;
  }
  
  // Safe for basic types
  return true;
}

// TypeTraits implementation
std::string TypeTraits::generateSubscriptOperatorTrait(const std::string &type_name) {
  std::ostringstream oss;
  oss << "template<typename T>\n";
  oss << "struct has_subscript_operator_" << type_name << " {\n";
  oss << "private:\n";
  oss << "    template<typename U>\n";
  oss << "    static auto test(int) -> decltype(std::declval<U>()[0], std::true_type{});\n";
  oss << "    template<typename>\n";
  oss << "    static std::false_type test(...);\n";
  oss << "public:\n";
  oss << "    static constexpr bool value = decltype(test<T>(0))::value;\n";
  oss << "};";
  return oss.str();
}

std::string TypeTraits::generateArithmeticOperatorTrait(const std::string &type_name,
                                                       const std::string &operator_name) {
  std::ostringstream oss;
  oss << "template<typename T, typename U = T>\n";
  oss << "struct has_" << operator_name << "_operator_" << type_name << " {\n";
  oss << "private:\n";
  oss << "    template<typename V, typename W>\n";
  oss << "    static auto test(int) -> decltype(std::declval<V>() " << operator_name 
      << " std::declval<W>(), std::true_type{});\n";
  oss << "    template<typename, typename>\n";
  oss << "    static std::false_type test(...);\n";
  oss << "public:\n";
  oss << "    static constexpr bool value = decltype(test<T, U>(0))::value;\n";
  oss << "};";
  return oss.str();
}

std::string TypeTraits::generateArrayTypeSpecialization(const std::string &element_type,
                                                       const std::string &size) {
  std::ostringstream oss;
  if (size.empty()) {
    oss << "template<>\n";
    oss << "struct __primop_subscript<" << element_type << "[]> {\n";
  } else {
    oss << "template<>\n";
    oss << "struct __primop_subscript<" << element_type << "[" << size << "]> {\n";
  }
  oss << "    using element_type = " << element_type << ";\n";
  oss << "    constexpr element_type& operator()(element_type arr[], std::size_t index) const {\n";
  oss << "        return arr[index];\n";
  oss << "    }\n";
  oss << "};";
  return oss.str();
}

std::string TypeTraits::generatePointerTypeSpecialization(const std::string &pointee_type) {
  std::ostringstream oss;
  oss << "template<>\n";
  oss << "struct __primop_subscript<" << pointee_type << "*> {\n";
  oss << "    using element_type = " << pointee_type << ";\n";
  oss << "    constexpr element_type& operator()(element_type* ptr, std::size_t index) const {\n";
  oss << "        return ptr[index];\n";
  oss << "    }\n";
  oss << "};";
  return oss.str();
}

// TypeTransformationContextBuilder implementation
TypeTransformationContextBuilder::TypeTransformationContextBuilder(clang::QualType type,
                                                                   clang::ASTContext &context)
    : type_(type), context_(context) {
  context_info_.source_type = type;
}

TypeTransformationContext TypeTransformationContextBuilder::build() {
  analyzeType();
  detectOverloadedOperators();
  generateTypeString();
  return context_info_;
}

TypeTransformationContextBuilder& TypeTransformationContextBuilder::withRuntimeTypeCheck() {
  context_info_.requires_runtime_check = true;
  return *this;
}

TypeTransformationContextBuilder& TypeTransformationContextBuilder::withInstrumentationTemplate(
    const std::string &template_code) {
  context_info_.instrumentation_template = template_code;
  return *this;
}

void TypeTransformationContextBuilder::analyzeType() {
  auto canonical_type = type_.getCanonicalType();
  
  context_info_.is_template_dependent = TypeAnalyzer::isTemplateDependentType(type_);
  context_info_.is_const_qualified = canonical_type.isConstQualified();
  context_info_.is_volatile_qualified = canonical_type.isVolatileQualified();
  
  // Set target type (usually same as source for basic transformations)
  context_info_.target_type = type_;
}

void TypeTransformationContextBuilder::detectOverloadedOperators() {
  context_info_.has_overloaded_operators = 
      TypeAnalyzer::hasOverloadedSubscriptOperator(type_, context_) ||
      TypeAnalyzer::hasOverloadedArithmeticOperators(type_, context_);
}

void TypeTransformationContextBuilder::generateTypeString() {
  context_info_.type_string = TypeAnalyzer::getCanonicalTypeString(type_, context_);
}

} // namespace optiweave::matchers