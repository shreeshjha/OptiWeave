#include "../../include/optiweave/analysis/template_analyzer.hpp"
#include <clang/AST/DeclTemplate.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/Support/raw_ostream.h>
#include <sstream>

namespace optiweave::analysis {

void TemplateStatistics::print(llvm::raw_ostream &os) const {
  os << "Template Usage Statistics:\n";
  os << "  Total template functions: " << total_template_functions << "\n";
  os << "  Total template classes: " << total_template_classes << "\n";
  os << "  Total template instantiations: " << total_template_instantiations << "\n";
  os << "  Dependent operator usages: " << dependent_operator_usages << "\n";
  os << "  SFINAE candidates: " << sfinae_candidates << "\n";
  
  os << "\nTemplate name breakdown:\n";
  for (const auto &pair : template_name_counts) {
    os << "  " << pair.first << ": " << pair.second << "\n";
  }
  
  os << "\nArgument type breakdown:\n";
  for (const auto &pair : argument_type_counts) {
    os << "  " << pair.first << ": " << pair.second << "\n";
  }
}

void TemplateStatistics::reset() {
  *this = TemplateStatistics{};
}

TemplateAnalyzer::TemplateAnalyzer(clang::ASTContext &context)
    : context_(context) {}

bool TemplateAnalyzer::VisitFunctionTemplateDecl(clang::FunctionTemplateDecl *decl) {
  TemplateUsage usage;
  usage.location = decl->getBeginLoc();
  usage.template_name = decl->getNameAsString();
  usage.is_dependent = true;
  usage.is_instantiation = false;
  usage.context_info = getContextInfo(usage.location);
  
  // Check if this template function has operator usage
  if (decl->getTemplatedDecl() && decl->getTemplatedDecl()->hasBody()) {
    usage.has_operator_usage = hasOperatorUsage(decl->getTemplatedDecl()->getBody());
  }

  recordTemplateUsage(usage);
  ++statistics_.total_template_functions;

  return true;
}

bool TemplateAnalyzer::VisitClassTemplateDecl(clang::ClassTemplateDecl *decl) {
  TemplateUsage usage;
  usage.location = decl->getBeginLoc();
  usage.template_name = decl->getNameAsString();
  usage.is_dependent = true;
  usage.is_instantiation = false;
  usage.context_info = getContextInfo(usage.location);
  
  // Check if this class template has operator overloads
  if (auto *cxx_record = clang::dyn_cast<clang::CXXRecordDecl>(decl->getTemplatedDecl())) {
    for (const auto *method : cxx_record->methods()) {
      if (method->isOverloadedOperator()) {
        usage.has_operator_usage = true;
        break;
      }
    }
  }

  recordTemplateUsage(usage);
  ++statistics_.total_template_classes;

  return true;
}

bool TemplateAnalyzer::VisitTemplateSpecializationType(clang::TemplateSpecializationType *type) {
  TemplateUsage usage;
  usage.template_name = type->getTemplateName().getAsTemplateDecl()->getNameAsString();
  usage.is_dependent = type->isDependentType();
  usage.is_instantiation = true;
  usage.context_info = "template instantiation";

  // Get template arguments
  for (unsigned i = 0; i < type->getNumArgs(); ++i) {
    const auto &arg = type->getArg(i);
    if (arg.getKind() == clang::TemplateArgument::Type) {
      clang::PrintingPolicy policy(context_.getLangOpts());
      usage.template_arguments.push_back(arg.getAsType().getAsString(policy));
    }
  }

  recordTemplateUsage(usage);
  ++statistics_.total_template_instantiations;

  return true;
}

bool TemplateAnalyzer::VisitDependentScopeDeclRefExpr(clang::DependentScopeDeclRefExpr *expr) {
  TemplateUsage usage;
  usage.location = expr->getBeginLoc();
  usage.template_name = expr->getDeclName().getAsString();
  usage.is_dependent = true;
  usage.is_instantiation = false;
  usage.has_operator_usage = true; // Assume dependent expressions might involve operators
  usage.context_info = getContextInfo(usage.location);

  recordTemplateUsage(usage);
  ++statistics_.dependent_operator_usages;

  return true;
}

bool TemplateAnalyzer::VisitExpr(clang::Expr *expr) {
  // Check for expressions with dependent types that might be operator-related
  if (expr->getType()->isDependentType()) {
    // Check for array subscripts, binary operators, etc.
    if (clang::isa<clang::ArraySubscriptExpr>(expr) ||
        clang::isa<clang::BinaryOperator>(expr) ||
        clang::isa<clang::UnaryOperator>(expr) ||
        clang::isa<clang::CXXOperatorCallExpr>(expr)) {
      
      TemplateUsage usage;
      usage.location = expr->getBeginLoc();
      usage.template_name = "dependent_expression";
      usage.is_dependent = true;
      usage.is_instantiation = false;
      usage.has_operator_usage = true;
      usage.context_info = getContextInfo(usage.location);

      if (isSfinaeCandidate(expr)) {
        ++statistics_.sfinae_candidates;
      }

      recordTemplateUsage(usage);
      ++statistics_.dependent_operator_usages;
    }
  }

  return true;
}

void TemplateAnalyzer::reset() {
  template_usages_.clear();
  statistics_.reset();
}

std::vector<TemplateUsage> TemplateAnalyzer::getSfinaeCandidates() const {
  std::vector<TemplateUsage> candidates;
  for (const auto &usage : template_usages_) {
    if (usage.is_dependent && usage.has_operator_usage) {
      candidates.push_back(usage);
    }
  }
  return candidates;
}

std::vector<TemplateUsage> TemplateAnalyzer::getDependentOperatorUsages() const {
  std::vector<TemplateUsage> dependent_ops;
  for (const auto &usage : template_usages_) {
    if (usage.is_dependent && usage.has_operator_usage) {
      dependent_ops.push_back(usage);
    }
  }
  return dependent_ops;
}

void TemplateAnalyzer::recordTemplateUsage(const TemplateUsage &usage) {
  template_usages_.push_back(usage);
  
  // Update statistics
  ++statistics_.template_name_counts[usage.template_name];
  
  for (const auto &arg : usage.template_arguments) {
    ++statistics_.argument_type_counts[arg];
  }
}

bool TemplateAnalyzer::isInSystemHeader(clang::SourceLocation location) const {
  return context_.getSourceManager().isInSystemHeader(location);
}

std::vector<std::string> TemplateAnalyzer::getTemplateArgumentStrings(
    const clang::TemplateArgumentList &args) const {
  std::vector<std::string> arg_strings;
  clang::PrintingPolicy policy(context_.getLangOpts());
  
  for (unsigned i = 0; i < args.size(); ++i) {
    const auto &arg = args[i];
    if (arg.getKind() == clang::TemplateArgument::Type) {
      arg_strings.push_back(arg.getAsType().getAsString(policy));
    } else if (arg.getKind() == clang::TemplateArgument::Integral) {
      arg_strings.push_back(arg.getAsIntegral().toString(10));
    }
  }
  
  return arg_strings;
}

std::string TemplateAnalyzer::getContextInfo(clang::SourceLocation location) const {
  auto &source_manager = context_.getSourceManager();
  
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

bool TemplateAnalyzer::hasOperatorUsage(const clang::Expr *expr) const {
  if (!expr) return false;
  
  // Check if this expression or its children contain operators
  if (clang::isa<clang::ArraySubscriptExpr>(expr) ||
      clang::isa<clang::BinaryOperator>(expr) ||
      clang::isa<clang::UnaryOperator>(expr) ||
      clang::isa<clang::CXXOperatorCallExpr>(expr)) {
    return true;
  }
  
  // Recursively check children
  for (const auto *child : expr->children()) {
    if (const auto *child_expr = clang::dyn_cast<clang::Expr>(child)) {
      if (hasOperatorUsage(child_expr)) {
        return true;
      }
    }
  }
  
  return false;
}

bool TemplateAnalyzer::isSfinaeCandidate(const clang::Expr *expr) const {
  // An expression is a good SFINAE candidate if:
  // 1. It involves template-dependent types
  // 2. It uses operators that might be overloaded
  // 3. It's not in a system header
  
  if (isInSystemHeader(expr->getBeginLoc())) {
    return false;
  }
  
  if (!expr->getType()->isDependentType()) {
    return false;
  }
  
  // Check for operator usage that benefits from SFINAE detection
  return clang::isa<clang::ArraySubscriptExpr>(expr) ||
         clang::isa<clang::BinaryOperator>(expr) ||
         clang::isa<clang::CXXOperatorCallExpr>(expr);
}

// TemplateAnalysisEngine implementation
TemplateAnalysisResult TemplateAnalysisEngine::analyzeTranslationUnit(clang::ASTContext &context) {
  TemplateAnalysisResult result;
  
  try {
    TemplateAnalyzer analyzer(context);
    analyzer.TraverseDecl(context.getTranslationUnitDecl());
    
    result.statistics = analyzer.getStatistics();
    result.all_usages = analyzer.getTemplateUsages();
    result.sfinae_candidates = analyzer.getSfinaeCandidates();
    result.dependent_operators = analyzer.getDependentOperatorUsages();
    result.recommendations = recommendTransformationStrategies(result);
    result.success = true;
    
  } catch (const std::exception &e) {
    result.success = false;
    result.error_message = "Template analysis failed: " + std::string(e.what());
  }
  
  return result;
}

std::string TemplateAnalysisEngine::generateOperatorDetectionTraits(
    const std::vector<TemplateUsage> &usages) {
  std::ostringstream oss;
  std::set<std::string> generated_traits;
  
  oss << "// Auto-generated operator detection traits\n";
  oss << "#pragma once\n";
  oss << "#include <type_traits>\n\n";
  
  for (const auto &usage : usages) {
    if (usage.has_operator_usage && usage.is_dependent) {
      std::string trait_name = "has_operators_" + usage.template_name;
      
      if (generated_traits.find(trait_name) == generated_traits.end()) {
        oss << TemplateCodeGenerator::generateOperatorTrait("[]", trait_name + "_subscript") << "\n\n";
        oss << TemplateCodeGenerator::generateOperatorTrait("+", trait_name + "_plus") << "\n\n";
        generated_traits.insert(trait_name);
      }
    }
  }
  
  return oss.str();
}

std::string TemplateAnalysisEngine::generateInstrumentationSpecializations(
    const std::vector<TemplateUsage> &usages) {
  std::ostringstream oss;
  
  oss << "// Auto-generated instrumentation specializations\n";
  oss << "#pragma once\n";
  oss << "#include \"prelude.hpp\"\n\n";
  
  for (const auto &usage : usages) {
    if (usage.has_operator_usage) {
      oss << "// Specialization for " << usage.template_name << "\n";
      oss << TemplateCodeGenerator::generateSfinaeWrapper(
          "__maybe_primop_subscript", "[]", usage.template_arguments) << "\n\n";
    }
  }
  
  return oss.str();
}

std::vector<TemplateTransformationRecommendation> 
TemplateAnalysisEngine::recommendTransformationStrategies(const TemplateAnalysisResult &analysis) {
  std::vector<TemplateTransformationRecommendation> recommendations;
  
  // Analyze template complexity and recommend strategies
  for (const auto &usage : analysis.sfinae_candidates) {
    TemplateTransformationRecommendation rec;
    
    double complexity = analyzeTemplateComplexity(usage);
    
    if (complexity < 0.3) {
      rec.strategy = TemplateTransformationStrategy::CompileTime;
      rec.rationale = "Low complexity - compile-time traits recommended";
      rec.confidence_score = 0.9;
    } else if (complexity < 0.7) {
      rec.strategy = TemplateTransformationStrategy::SfinaeDetection;
      rec.rationale = "Medium complexity - SFINAE detection recommended";
      rec.confidence_score = 0.8;
    } else {
      rec.strategy = TemplateTransformationStrategy::RuntimeCheck;
      rec.rationale = "High complexity - runtime checking recommended";
      rec.confidence_score = 0.7;
    }
    
    rec.template_name = usage.template_name;
    rec.generated_code = generateTypeTrait(rec.template_name + "_trait", "[]");
    
    recommendations.push_back(rec);
  }
  
  return recommendations;
}

double TemplateAnalysisEngine::analyzeTemplateComplexity(const TemplateUsage &usage) {
  double complexity = 0.0;
  
  // Factor in number of template arguments
  complexity += usage.template_arguments.size() * 0.1;
  
  // Factor in dependency
  if (usage.is_dependent) {
    complexity += 0.3;
  }
  
  // Factor in operator usage
  if (usage.has_operator_usage) {
    complexity += 0.2;
  }
  
  // Factor in template name complexity (rough heuristic)
  complexity += usage.template_name.length() * 0.01;
  
  // Clamp to [0, 1]
  return std::min(1.0, std::max(0.0, complexity));
}

std::string TemplateAnalysisEngine::generateTypeTrait(const std::string &trait_name,
                                                     const std::string &operator_name) {
  return TemplateCodeGenerator::generateOperatorTrait(operator_name, trait_name);
}

std::string TemplateAnalysisEngine::generateSfinaeDetector(const std::string &type_name,
                                                          const std::string &operation) {
  std::ostringstream oss;
  oss << "template<typename T>\n";
  oss << "struct " << type_name << "_" << operation << "_detector {\n";
  oss << "private:\n";
  oss << "    template<typename U>\n";
  oss << "    static auto test(int) -> decltype(std::declval<U>()" << operation << ", std::true_type{});\n";
  oss << "    template<typename>\n";
  oss << "    static std::false_type test(...);\n";
  oss << "public:\n";
  oss << "    static constexpr bool value = decltype(test<T>(0))::value;\n";
  oss << "};";
  return oss.str();
}

// TemplateCodeGenerator implementation
std::string TemplateCodeGenerator::generateOperatorTrait(const std::string &operator_symbol,
                                                        const std::string &trait_name) {
  std::ostringstream oss;
  oss << "template<typename T>\n";
  oss << "struct " << trait_name << " {\n";
  oss << "private:\n";
  oss << "    template<typename U>\n";
  
  if (operator_symbol == "[]") {
    oss << "    static auto test(int) -> decltype(std::declval<U>()[0], std::true_type{});\n";
  } else if (operator_symbol == "+" || operator_symbol == "-" || 
             operator_symbol == "*" || operator_symbol == "/" || operator_symbol == "%") {
    oss << "    static auto test(int) -> decltype(std::declval<U>() " << operator_symbol 
        << " std::declval<U>(), std::true_type{});\n";
  } else {
    oss << "    static auto test(int) -> decltype(std::declval<U>() " << operator_symbol 
        << " std::declval<U>(), std::true_type{});\n";
  }
  
  oss << "    template<typename>\n";
  oss << "    static std::false_type test(...);\n";
  oss << "public:\n";
  oss << "    static constexpr bool value = decltype(test<T>(0))::value;\n";
  oss << "};";
  
  return oss.str();
}

std::string TemplateCodeGenerator::generateRuntimeTypeCheck(const std::string &type_expr,
                                                           const std::string &operator_name) {
  std::ostringstream oss;
  oss << "if constexpr (has_" << operator_name << "_operator<decltype(" << type_expr << ")>::value) {\n";
  oss << "    // Use overloaded operator\n";
  oss << "    return " << type_expr << ";\n";
  oss << "} else {\n";
  oss << "    // Use instrumented version\n";
  oss << "    return __primop_" << operator_name << "<decltype(" << type_expr << ")>()(" << type_expr << ");\n";
  oss << "}";
  
  return oss.str();
}

std::string TemplateCodeGenerator::generateTemplateSpecialization(
    const std::string &template_name,
    const std::vector<std::string> &type_parameters,
    const std::string &specialization_body) {
  std::ostringstream oss;
  
  oss << "template<";
  for (size_t i = 0; i < type_parameters.size(); ++i) {
    if (i > 0) oss << ", ";
    oss << "typename " << type_parameters[i];
  }
  oss << ">\n";
  
  oss << "struct " << template_name << "<";
  for (size_t i = 0; i < type_parameters.size(); ++i) {
    if (i > 0) oss << ", ";
    oss << type_parameters[i];
  }
  oss << "> {\n";
  oss << specialization_body << "\n";
  oss << "};";
  
  return oss.str();
}

std::string TemplateCodeGenerator::generateSfinaeWrapper(const std::string &function_name,
                                                        const std::string &operator_name,
                                                        const std::vector<std::string> &type_params) {
  std::ostringstream oss;
  
  oss << "template<typename T, bool HasOverload>\n";
  oss << "struct " << function_name << " {\n";
  oss << "    template<typename IndexType>\n";
  oss << "    constexpr auto operator()(T&& obj, IndexType&& index) const\n";
  oss << "        -> decltype(std::forward<T>(obj)" << operator_name << "std::forward<IndexType>(index)) {\n";
  oss << "        return std::forward<T>(obj)" << operator_name << "std::forward<IndexType>(index);\n";
  oss << "    }\n";
  oss << "};\n\n";
  
  oss << "template<typename T>\n";
  oss << "struct " << function_name << "<T, false> : __primop_" << operator_name << "<T> {};";
  
  return oss.str();
}

} // namespace optiweave::analysis