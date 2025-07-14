#include "../../include/optiweave/analysis/operator_detector.hpp"
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>

namespace optiweave::analysis {

void OperatorStatistics::print(llvm::raw_ostream &os) const {
  os << "Operator Usage Statistics:\n";
  os << "  Total array subscripts: " << total_array_subscripts << "\n";
  os << "  Total arithmetic ops: " << total_arithmetic_ops << "\n";
  os << "  Total assignment ops: " << total_assignment_ops << "\n";
  os << "  Total comparison ops: " << total_comparison_ops << "\n";
  os << "  Overloaded operators: " << overloaded_operators << "\n";
  os << "  Template dependent ops: " << template_dependent_ops << "\n";
  os << "  System header ops: " << system_header_ops << "\n";
  
  os << "\nOperator breakdown:\n";
  for (const auto &pair : operator_counts) {
    os << "  " << pair.first << ": " << pair.second << "\n";
  }
  
  os << "\nType usage breakdown:\n";
  for (const auto &pair : type_usage_counts) {
    os << "  " << pair.first << ": " << pair.second << "\n";
  }
}

void OperatorStatistics::reset() {
  *this = OperatorStatistics{};
}

OperatorDetector::OperatorDetector(clang::ASTContext &context)
    : context_(context) {}

bool OperatorDetector::VisitArraySubscriptExpr(clang::ArraySubscriptExpr *expr) {
  OperatorUsage usage;
  usage.location = expr->getBeginLoc();
  usage.operator_name = "[]";
  usage.lhs_type = getTypeString(expr->getLHS()->getType());
  usage.rhs_type = getTypeString(expr->getRHS()->getType());
  usage.is_overloaded = false; // Built-in array subscript
  usage.is_template_dependent = isTemplateDependentType(expr->getLHS()->getType()) ||
                                isTemplateDependentType(expr->getRHS()->getType());
  usage.in_system_header = isInSystemHeader(usage.location);
  usage.context_info = getContextInfo(expr);

  recordOperatorUsage(usage);
  ++statistics_.total_array_subscripts;

  return true;
}

bool OperatorDetector::VisitBinaryOperator(clang::BinaryOperator *expr) {
  OperatorUsage usage;
  usage.location = expr->getBeginLoc();
  
  // Determine operator name
  switch (expr->getOpcode()) {
    case clang::BO_Add: usage.operator_name = "+"; break;
    case clang::BO_Sub: usage.operator_name = "-"; break;
    case clang::BO_Mul: usage.operator_name = "*"; break;
    case clang::BO_Div: usage.operator_name = "/"; break;
    case clang::BO_Rem: usage.operator_name = "%"; break;
    case clang::BO_Assign: usage.operator_name = "="; break;
    case clang::BO_AddAssign: usage.operator_name = "+="; break;
    case clang::BO_SubAssign: usage.operator_name = "-="; break;
    case clang::BO_MulAssign: usage.operator_name = "*="; break;
    case clang::BO_DivAssign: usage.operator_name = "/="; break;
    case clang::BO_RemAssign: usage.operator_name = "%="; break;
    case clang::BO_EQ: usage.operator_name = "=="; break;
    case clang::BO_NE: usage.operator_name = "!="; break;
    case clang::BO_LT: usage.operator_name = "<"; break;
    case clang::BO_GT: usage.operator_name = ">"; break;
    case clang::BO_LE: usage.operator_name = "<="; break;
    case clang::BO_GE: usage.operator_name = ">="; break;
    default: usage.operator_name = "unknown"; break;
  }

  usage.lhs_type = getTypeString(expr->getLHS()->getType());
  usage.rhs_type = getTypeString(expr->getRHS()->getType());
  usage.is_overloaded = false; // Built-in binary operator
  usage.is_template_dependent = isTemplateDependentType(expr->getLHS()->getType()) ||
                                isTemplateDependentType(expr->getRHS()->getType());
  usage.in_system_header = isInSystemHeader(usage.location);
  usage.context_info = getContextInfo(expr);

  recordOperatorUsage(usage);

  // Update category counters
  if (expr->isArithmeticOp()) {
    ++statistics_.total_arithmetic_ops;
  } else if (expr->isAssignmentOp()) {
    ++statistics_.total_assignment_ops;
  } else if (expr->isComparisonOp()) {
    ++statistics_.total_comparison_ops;
  }

  return true;
}

bool OperatorDetector::VisitUnaryOperator(clang::UnaryOperator *expr) {
  OperatorUsage usage;
  usage.location = expr->getBeginLoc();
  
  // Determine operator name
  switch (expr->getOpcode()) {
    case clang::UO_Plus: usage.operator_name = "+"; break;
    case clang::UO_Minus: usage.operator_name = "-"; break;
    case clang::UO_PreInc: usage.operator_name = "++"; break;
    case clang::UO_PostInc: usage.operator_name = "++"; break;
    case clang::UO_PreDec: usage.operator_name = "--"; break;
    case clang::UO_PostDec: usage.operator_name = "--"; break;
    default: usage.operator_name = "unary"; break;
  }

  usage.lhs_type = getTypeString(expr->getSubExpr()->getType());
  usage.rhs_type = ""; // Unary operators don't have RHS
  usage.is_overloaded = false; // Built-in unary operator
  usage.is_template_dependent = isTemplateDependentType(expr->getSubExpr()->getType());
  usage.in_system_header = isInSystemHeader(usage.location);
  usage.context_info = getContextInfo(expr);

  recordOperatorUsage(usage);

  return true;
}

bool OperatorDetector::VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *expr) {
  OperatorUsage usage;
  usage.location = expr->getBeginLoc();
  usage.is_overloaded = true;
  
  // Determine operator name from overloaded operator
  switch (expr->getOperator()) {
    case clang::OO_Subscript: usage.operator_name = "[]"; break;
    case clang::OO_Plus: usage.operator_name = "+"; break;
    case clang::OO_Minus: usage.operator_name = "-"; break;
    case clang::OO_Star: usage.operator_name = "*"; break;
    case clang::OO_Slash: usage.operator_name = "/"; break;
    case clang::OO_Percent: usage.operator_name = "%"; break;
    case clang::OO_Equal: usage.operator_name = "="; break;
    case clang::OO_PlusEqual: usage.operator_name = "+="; break;
    case clang::OO_MinusEqual: usage.operator_name = "-="; break;
    case clang::OO_StarEqual: usage.operator_name = "*="; break;
    case clang::OO_SlashEqual: usage.operator_name = "/="; break;
    case clang::OO_PercentEqual: usage.operator_name = "%="; break;
    case clang::OO_EqualEqual: usage.operator_name = "=="; break;
    case clang::OO_ExclaimEqual: usage.operator_name = "!="; break;
    case clang::OO_Less: usage.operator_name = "<"; break;
    case clang::OO_Greater: usage.operator_name = ">"; break;
    case clang::OO_LessEqual: usage.operator_name = "<="; break;
    case clang::OO_GreaterEqual: usage.operator_name = ">="; break;
    default: usage.operator_name = "overloaded"; break;
  }

  // Get argument types
  if (expr->getNumArgs() > 0) {
    usage.lhs_type = getTypeString(expr->getArg(0)->getType());
  }
  if (expr->getNumArgs() > 1) {
    usage.rhs_type = getTypeString(expr->getArg(1)->getType());
  }

  usage.is_template_dependent = false;
  for (unsigned i = 0; i < expr->getNumArgs(); ++i) {
    if (isTemplateDependentType(expr->getArg(i)->getType())) {
      usage.is_template_dependent = true;
      break;
    }
  }

  usage.in_system_header = isInSystemHeader(usage.location);
  usage.context_info = getContextInfo(expr);

  recordOperatorUsage(usage);
  ++statistics_.overloaded_operators;

  // Update category counters based on operator type
  if (usage.operator_name == "[]") {
    ++statistics_.total_array_subscripts;
  } else if (usage.operator_name == "+" || usage.operator_name == "-" ||
             usage.operator_name == "*" || usage.operator_name == "/" ||
             usage.operator_name == "%") {
    ++statistics_.total_arithmetic_ops;
  }

  return true;
}

void OperatorDetector::reset() {
  operator_usages_.clear();
  statistics_.reset();
}

std::vector<OperatorUsage> 
OperatorDetector::getUsagesByOperator(const std::string &operator_name) const {
  std::vector<OperatorUsage> filtered;
  for (const auto &usage : operator_usages_) {
    if (usage.operator_name == operator_name) {
      filtered.push_back(usage);
    }
  }
  return filtered;
}

std::vector<OperatorUsage> 
OperatorDetector::getUsagesByType(const std::string &type_name) const {
  std::vector<OperatorUsage> filtered;
  for (const auto &usage : operator_usages_) {
    if (usage.lhs_type.find(type_name) != std::string::npos ||
        usage.rhs_type.find(type_name) != std::string::npos) {
      filtered.push_back(usage);
    }
  }
  return filtered;
}

std::vector<OperatorUsage> OperatorDetector::getTransformationOpportunities() const {
  std::vector<OperatorUsage> opportunities;
  
  for (const auto &usage : operator_usages_) {
    // Skip system headers
    if (usage.in_system_header) {
      continue;
    }
    
    // Skip already overloaded operators (they're already "instrumented")
    if (usage.is_overloaded) {
      continue;
    }
    
    // Include built-in operators that could benefit from instrumentation
    if (usage.operator_name == "[]" || 
        usage.operator_name == "+" || usage.operator_name == "-" ||
        usage.operator_name == "*" || usage.operator_name == "/" ||
        usage.operator_name == "%") {
      opportunities.push_back(usage);
    }
  }
  
  return opportunities;
}

void OperatorDetector::recordOperatorUsage(const OperatorUsage &usage) {
  operator_usages_.push_back(usage);
  
  // Update statistics
  ++statistics_.operator_counts[usage.operator_name];
  ++statistics_.type_usage_counts[usage.lhs_type];
  
  if (!usage.rhs_type.empty()) {
    ++statistics_.type_usage_counts[usage.rhs_type];
  }
  
  if (usage.is_template_dependent) {
    ++statistics_.template_dependent_ops;
  }
  
  if (usage.in_system_header) {
    ++statistics_.system_header_ops;
  }
}

bool OperatorDetector::isInSystemHeader(clang::SourceLocation location) const {
  return context_.getSourceManager().isInSystemHeader(location);
}

std::string OperatorDetector::getTypeString(clang::QualType type) const {
  clang::PrintingPolicy policy(context_.getLangOpts());
  policy.SuppressTagKeyword = true;
  return type.getAsString(policy);
}

std::string OperatorDetector::getContextInfo(const clang::Expr *expr) const {
  auto &source_manager = context_.getSourceManager();
  auto location = expr->getBeginLoc();
  
  if (location.isValid()) {
    auto file_entry = source_manager.getFileEntryForLoc(location);
    if (file_entry) {
      auto line = source_manager.getSpellingLineNumber(location);
      auto column = source_manager.getSpellingColumnNumber(location);
      return file_entry->getName().str() + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
  }
  
  return "unknown location";
}

bool OperatorDetector::isTemplateDependentType(clang::QualType type) const {
  return type->isDependentType() || 
         type->isInstantiationDependentType() ||
         type->isTemplateTypeParmType() || 
         type->isUndeducedType();
}

// OperatorAnalyzer implementation
OperatorAnalysisResult OperatorAnalyzer::analyzeTranslationUnit(clang::ASTContext &context) {
  OperatorAnalysisResult result;
  
  try {
    OperatorDetector detector(context);
    detector.TraverseDecl(context.getTranslationUnitDecl());
    
    result.statistics = detector.getStatistics();
    result.all_usages = detector.getOperatorUsages();
    result.transformation_candidates = filterTransformationCandidates(result.all_usages);
    result.recommendations = generateRecommendations(result.statistics);
    result.success = true;
    
  } catch (const std::exception &e) {
    result.success = false;
    result.error_message = "Analysis failed: " + std::string(e.what());
  }
  
  return result;
}

OperatorAnalysisResult OperatorAnalyzer::analyzeFunction(const clang::FunctionDecl *func,
                                                        clang::ASTContext &context) {
  OperatorAnalysisResult result;
  
  try {
    OperatorDetector detector(context);
    
    if (func->hasBody()) {
      detector.TraverseStmt(func->getBody());
    }
    
    result.statistics = detector.getStatistics();
    result.all_usages = detector.getOperatorUsages();
    result.transformation_candidates = filterTransformationCandidates(result.all_usages);
    result.recommendations = generateRecommendations(result.statistics);
    result.success = true;
    
  } catch (const std::exception &e) {
    result.success = false;
    result.error_message = "Function analysis failed: " + std::string(e.what());
  }
  
  return result;
}

std::vector<std::string> OperatorAnalyzer::generateRecommendations(const OperatorStatistics &stats) {
  std::vector<std::string> recommendations;
  
  // Array subscript recommendations
  if (stats.total_array_subscripts > 10) {
    recommendations.push_back("Consider enabling array subscript transformation for bounds checking");
  }
  
  // Arithmetic operator recommendations
  if (stats.total_arithmetic_ops > 20) {
    recommendations.push_back("High arithmetic operator usage - consider performance instrumentation");
  }
  
  // Template-dependent recommendations
  if (stats.template_dependent_ops > 5) {
    recommendations.push_back("Template-dependent operations detected - use runtime type checking");
  }
  
  // Overloaded operator recommendations
  if (stats.overloaded_operators > 0) {
    recommendations.push_back("Overloaded operators detected - they may already be instrumented");
  }
  
  // System header recommendations
  if (stats.system_header_ops > stats.total_array_subscripts / 2) {
    recommendations.push_back("Many operations in system headers - consider excluding them");
  }
  
  if (recommendations.empty()) {
    recommendations.push_back("Code appears suitable for transformation with default settings");
  }
  
  return recommendations;
}

std::vector<OperatorUsage> OperatorAnalyzer::filterTransformationCandidates(
    const std::vector<OperatorUsage> &usages) {
  std::vector<OperatorUsage> candidates;
  
  for (const auto &usage : usages) {
    // Exclude system headers
    if (usage.in_system_header) {
      continue;
    }
    
    // Exclude already overloaded operators
    if (usage.is_overloaded) {
      continue;
    }
    
    // Include transformable operators
    if (usage.operator_name == "[]" || 
        usage.operator_name == "+" || usage.operator_name == "-" ||
        usage.operator_name == "*" || usage.operator_name == "/" ||
        usage.operator_name == "%" ||
        usage.operator_name == "=" || usage.operator_name == "+=" ||
        usage.operator_name == "-=" || usage.operator_name == "*=" ||
        usage.operator_name == "/=" || usage.operator_name == "%=" ||
        usage.operator_name == "==" || usage.operator_name == "!=" ||
        usage.operator_name == "<" || usage.operator_name == ">" ||
        usage.operator_name == "<=" || usage.operator_name == ">=") {
      candidates.push_back(usage);
    }
  }
  
  return candidates;
}

} // namespace optiweave::analysis