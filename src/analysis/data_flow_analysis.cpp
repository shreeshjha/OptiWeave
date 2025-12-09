#include "optiweave/analysis/data_flow_analysis.hpp"
#include <clang/AST/ParentMapContext.h>
#include <clang/AST/Stmt.h>
#include <clang/Analysis/CFG.h>
#include <clang/Basic/SourceManager.h>
#include <fstream>
#include <sstream>

namespace optiweave {
namespace analysis {

void VariableInfo::add_use(clang::SourceLocation loc, const std::string& ctx,
                           bool is_read) {
  uses.emplace_back(loc, ctx, is_read);
  if (state == VariableState::Declared || state == VariableState::Initialized) {
    state = VariableState::Used;
  }
}

void VariableInfo::add_definition(clang::SourceLocation loc,
                                  const std::string& ctx) {
  definitions.emplace_back(loc, ctx, false);
  if (state == VariableState::Declared) {
    state = VariableState::Initialized;
  }
}

std::string DataFlowAnalysis::get_variable_key(
    const std::string& var_name, const std::string& function_context) const {
  if (function_context.empty()) {
    return "::" + var_name; // Global variable
  }
  return function_context + "::" + var_name;
}

std::string DataFlowAnalysis::get_location_string(
    clang::SourceLocation loc, clang::SourceManager& sm) const {
  if (loc.isInvalid()) {
    return "unknown location";
  }
  auto presumed = sm.getPresumedLoc(loc);
  if (presumed.isInvalid()) {
    return "unknown location";
  }
  std::ostringstream oss;
  oss << presumed.getFilename() << ":" << presumed.getLine() << ":"
      << presumed.getColumn();
  return oss.str();
}

void DataFlowAnalysis::add_variable(const clang::VarDecl* decl,
                                   const std::string& function_context) {
  if (!decl) return;

  std::string var_name = decl->getNameAsString();
  std::string key = get_variable_key(var_name, function_context);

  VariableInfo info(var_name, decl->getType().getAsString(),
                   decl->getLocation(), function_context);

  info.has_initializer = decl->hasInit();
  info.is_parameter = clang::isa<clang::ParmVarDecl>(decl);
  info.is_global = decl->hasGlobalStorage();

  if (info.has_initializer || info.is_parameter) {
    info.state = VariableState::Initialized;
  } else {
    info.state = VariableState::Declared;
  }

  variables_[key] = info;
}

void DataFlowAnalysis::record_variable_use(const clang::DeclRefExpr* expr,
                                          const std::string& function_context,
                                          bool is_read) {
  if (!expr) return;

  const clang::ValueDecl* decl = expr->getDecl();
  if (!decl) return;

  const clang::VarDecl* var_decl = clang::dyn_cast<clang::VarDecl>(decl);
  if (!var_decl) return;

  std::string var_name = var_decl->getNameAsString();
  std::string key = get_variable_key(var_name, function_context);

  auto it = variables_.find(key);
  if (it != variables_.end()) {
    it->second.add_use(expr->getLocation(), function_context, is_read);
  }
}

void DataFlowAnalysis::record_variable_definition(
    const clang::DeclRefExpr* expr, const std::string& function_context) {
  if (!expr) return;

  const clang::ValueDecl* decl = expr->getDecl();
  if (!decl) return;

  const clang::VarDecl* var_decl = clang::dyn_cast<clang::VarDecl>(decl);
  if (!var_decl) return;

  std::string var_name = var_decl->getNameAsString();
  std::string key = get_variable_key(var_name, function_context);

  auto it = variables_.find(key);
  if (it != variables_.end()) {
    it->second.add_definition(expr->getLocation(), function_context);
  }
}

void DataFlowAnalysis::analyze_function(const clang::FunctionDecl* func,
                                       clang::ASTContext& context) {
  if (!func || !func->hasBody()) return;

  // Build CFG for the function
  clang::CFG::BuildOptions opts;
  opts.AddImplicitDtors = true;
  opts.AddInitializers = true;
  opts.AddTemporaryDtors = true;

  std::unique_ptr<clang::CFG> cfg =
      clang::CFG::buildCFG(func, func->getBody(), &context, opts);

  if (cfg) {
    analyze_cfg(cfg.get(), func, context);
  }
}

void DataFlowAnalysis::analyze_cfg(const clang::CFG* cfg,
                                  const clang::FunctionDecl* func,
                                  clang::ASTContext& context) {
  if (!cfg || !func) return;

  std::string func_name = func->getNameAsString();

  // Iterate through all CFG blocks
  for (const clang::CFGBlock* block : *cfg) {
    if (!block) continue;

    // Check if block is unreachable
    if (is_unreachable_block(block)) {
      // Mark statements in this block as dead code
      for (const clang::CFGElement& elem : *block) {
        if (auto stmt_elem = elem.getAs<clang::CFGStmt>()) {
          const clang::Stmt* stmt = stmt_elem->getStmt();
          if (stmt) {
            dead_code_.emplace_back(
                stmt->getBeginLoc(),
                "Unreachable code detected",
                func_name);
          }
        }
      }
    }
  }
}

bool DataFlowAnalysis::is_unreachable_block(
    const clang::CFGBlock* block) const {
  if (!block) return false;

  // A block is unreachable if it has no predecessors and is not the entry block
  // Entry block has BlockID 0 or is marked as entry
  if (block->pred_empty() && block->getBlockID() != 0) {
    return true;
  }

  return false;
}

void DataFlowAnalysis::detect_uninitialized_variables() {
  for (auto& pair : variables_) {
    VariableInfo& info = pair.second;

    // Skip parameters and globals (assumed initialized)
    if (info.is_parameter || info.is_global) continue;

    // Check if variable is used before initialization
    bool used_before_init = false;
    for (const auto& use : info.uses) {
      if (use.is_read && !info.has_initializer && info.definitions.empty()) {
        used_before_init = true;
        break;
      }
      // Check if use comes before first definition
      if (use.is_read && !info.definitions.empty()) {
        bool has_prior_def = false;
        for (const auto& def : info.definitions) {
          if (def.location < use.location) {
            has_prior_def = true;
            break;
          }
        }
        if (!has_prior_def && !info.has_initializer) {
          used_before_init = true;
          break;
        }
      }
    }

    if (used_before_init) {
      info.state = VariableState::MaybeUninitialized;
    }
  }
}

void DataFlowAnalysis::detect_unused_variables() {
  for (auto& pair : variables_) {
    VariableInfo& info = pair.second;

    // Skip parameters (may be intentionally unused for interface compliance)
    if (info.is_parameter) continue;

    // Check if variable is never used
    if (info.uses.empty() &&
        (info.has_initializer || !info.definitions.empty())) {
      info.state = VariableState::Unused;
    }

    // Check if variable is never read (only written)
    bool has_read = false;
    for (const auto& use : info.uses) {
      if (use.is_read) {
        has_read = true;
        break;
      }
    }
    if (!has_read && !info.uses.empty()) {
      info.state = VariableState::Unused;
    }
  }
}

void DataFlowAnalysis::detect_dead_code() {
  // Dead code detection is primarily done in analyze_cfg
  // This method can perform additional checks

  // Additional heuristics can be added here
  // For example: checking for code after return statements
}

void DataFlowAnalysis::generate_refactoring_opportunities() {
  refactoring_opportunities_.clear();

  for (const auto& pair : variables_) {
    const VariableInfo& info = pair.second;

    switch (info.state) {
      case VariableState::MaybeUninitialized:
        refactoring_opportunities_.emplace_back(
            RefactoringOpportunity::Type::UninitializedVariable,
            info.declaration_loc,
            "Variable '" + info.name + "' may be used before initialization",
            "Initialize '" + info.name + "' at declaration or before first use",
            info.function_context);
        break;

      case VariableState::Unused:
        refactoring_opportunities_.emplace_back(
            RefactoringOpportunity::Type::UnusedVariable,
            info.declaration_loc,
            "Variable '" + info.name + "' is declared but never used",
            "Remove unused variable '" + info.name + "'",
            info.function_context);
        break;

      case VariableState::Declared:
        // Variable declared but never initialized or used
        if (info.uses.empty() && info.definitions.empty()) {
          refactoring_opportunities_.emplace_back(
              RefactoringOpportunity::Type::UnusedVariable,
              info.declaration_loc,
              "Variable '" + info.name + "' is declared but never used or initialized",
              "Remove unused variable '" + info.name + "'",
              info.function_context);
        }
        break;

      default:
        break;
    }

    // Check if variable is only written but never read
    bool has_read = false;
    for (const auto& use : info.uses) {
      if (use.is_read) {
        has_read = true;
        break;
      }
    }
    if (!has_read && !info.uses.empty() && info.state != VariableState::Unused) {
      refactoring_opportunities_.emplace_back(
          RefactoringOpportunity::Type::VariableNeverRead,
          info.declaration_loc,
          "Variable '" + info.name + "' is written but never read",
          "Remove or use variable '" + info.name + "'",
          info.function_context);
    }
  }

  // Add dead code opportunities
  for (const auto& dead : dead_code_) {
    refactoring_opportunities_.emplace_back(
        RefactoringOpportunity::Type::DeadCode,
        dead.location,
        dead.description,
        "Remove unreachable code",
        dead.function_context);
  }
}

void DataFlowAnalysis::export_json(const std::string& filename,
                                  clang::SourceManager& sm) const {
  std::ofstream out(filename);
  if (!out) {
    llvm::errs() << "Failed to open file: " << filename << "\n";
    return;
  }

  out << "{\n";
  out << "  \"variables\": [\n";

  bool first_var = true;
  for (const auto& pair : variables_) {
    const VariableInfo& info = pair.second;
    if (!first_var) out << ",\n";
    first_var = false;

    out << "    {\n";
    out << "      \"name\": \"" << info.name << "\",\n";
    out << "      \"type\": \"" << info.type << "\",\n";
    out << "      \"location\": \"" << get_location_string(info.declaration_loc, sm) << "\",\n";
    out << "      \"function\": \"" << info.function_context << "\",\n";
    out << "      \"has_initializer\": " << (info.has_initializer ? "true" : "false") << ",\n";
    out << "      \"is_parameter\": " << (info.is_parameter ? "true" : "false") << ",\n";
    out << "      \"is_global\": " << (info.is_global ? "true" : "false") << ",\n";

    std::string state_str;
    switch (info.state) {
      case VariableState::Declared: state_str = "declared"; break;
      case VariableState::Initialized: state_str = "initialized"; break;
      case VariableState::Used: state_str = "used"; break;
      case VariableState::Unused: state_str = "unused"; break;
      case VariableState::MaybeUninitialized: state_str = "maybe_uninitialized"; break;
    }
    out << "      \"state\": \"" << state_str << "\",\n";

    out << "      \"uses\": " << info.uses.size() << ",\n";
    out << "      \"definitions\": " << info.definitions.size() << "\n";
    out << "    }";
  }

  out << "\n  ],\n";
  out << "  \"refactoring_opportunities\": [\n";

  bool first_opp = true;
  for (const auto& opp : refactoring_opportunities_) {
    if (!first_opp) out << ",\n";
    first_opp = false;

    out << "    {\n";

    std::string type_str;
    switch (opp.type) {
      case RefactoringOpportunity::Type::UnusedVariable: type_str = "unused_variable"; break;
      case RefactoringOpportunity::Type::UninitializedVariable: type_str = "uninitialized_variable"; break;
      case RefactoringOpportunity::Type::DeadCode: type_str = "dead_code"; break;
      case RefactoringOpportunity::Type::VariableNeverRead: type_str = "variable_never_read"; break;
      case RefactoringOpportunity::Type::VariableNeverWritten: type_str = "variable_never_written"; break;
    }
    out << "      \"type\": \"" << type_str << "\",\n";
    out << "      \"location\": \"" << get_location_string(opp.location, sm) << "\",\n";
    out << "      \"description\": \"" << opp.description << "\",\n";
    out << "      \"suggestion\": \"" << opp.suggestion << "\",\n";
    out << "      \"function\": \"" << opp.function_context << "\"\n";
    out << "    }";
  }

  out << "\n  ],\n";
  out << "  \"dead_code\": [\n";

  bool first_dead = true;
  for (const auto& dead : dead_code_) {
    if (!first_dead) out << ",\n";
    first_dead = false;

    out << "    {\n";
    out << "      \"location\": \"" << get_location_string(dead.location, sm) << "\",\n";
    out << "      \"description\": \"" << dead.description << "\",\n";
    out << "      \"function\": \"" << dead.function_context << "\"\n";
    out << "    }";
  }

  out << "\n  ]\n";
  out << "}\n";
}

void DataFlowAnalysis::export_text(const std::string& filename,
                                  clang::SourceManager& sm) const {
  std::ofstream out(filename);
  if (!out) {
    llvm::errs() << "Failed to open file: " << filename << "\n";
    return;
  }

  out << "=== Data Flow Analysis Report ===\n\n";

  // Variables summary
  out << "Variables Analyzed: " << variables_.size() << "\n";
  size_t unused = 0, uninitialized = 0, used = 0;
  for (const auto& pair : variables_) {
    switch (pair.second.state) {
      case VariableState::Unused: unused++; break;
      case VariableState::MaybeUninitialized: uninitialized++; break;
      case VariableState::Used: used++; break;
      default: break;
    }
  }
  out << "  Used: " << used << "\n";
  out << "  Unused: " << unused << "\n";
  out << "  Possibly Uninitialized: " << uninitialized << "\n\n";

  // Refactoring opportunities
  out << "Refactoring Opportunities: " << refactoring_opportunities_.size() << "\n\n";
  for (const auto& opp : refactoring_opportunities_) {
    out << "[" << get_location_string(opp.location, sm) << "]\n";
    out << "  Type: ";
    switch (opp.type) {
      case RefactoringOpportunity::Type::UnusedVariable: out << "Unused Variable"; break;
      case RefactoringOpportunity::Type::UninitializedVariable: out << "Uninitialized Variable"; break;
      case RefactoringOpportunity::Type::DeadCode: out << "Dead Code"; break;
      case RefactoringOpportunity::Type::VariableNeverRead: out << "Variable Never Read"; break;
      case RefactoringOpportunity::Type::VariableNeverWritten: out << "Variable Never Written"; break;
    }
    out << "\n";
    out << "  Description: " << opp.description << "\n";
    out << "  Suggestion: " << opp.suggestion << "\n";
    out << "  Function: " << opp.function_context << "\n\n";
  }

  // Dead code
  if (!dead_code_.empty()) {
    out << "Dead Code Detected: " << dead_code_.size() << " instances\n\n";
    for (const auto& dead : dead_code_) {
      out << "[" << get_location_string(dead.location, sm) << "]\n";
      out << "  " << dead.description << "\n";
      out << "  Function: " << dead.function_context << "\n\n";
    }
  }
}

void DataFlowAnalysis::print_statistics(llvm::raw_ostream& os,
                                       clang::SourceManager& sm) const {
  os << "\n=== Data Flow Analysis Statistics ===\n";
  os << "Total Variables Analyzed: " << variables_.size() << "\n";

  size_t unused = 0, uninitialized = 0, used = 0, declared = 0, initialized = 0;
  for (const auto& pair : variables_) {
    switch (pair.second.state) {
      case VariableState::Declared: declared++; break;
      case VariableState::Initialized: initialized++; break;
      case VariableState::Used: used++; break;
      case VariableState::Unused: unused++; break;
      case VariableState::MaybeUninitialized: uninitialized++; break;
    }
  }

  os << "  Declared but not initialized: " << declared << "\n";
  os << "  Initialized: " << initialized << "\n";
  os << "  Used: " << used << "\n";
  os << "  Unused: " << unused << "\n";
  os << "  Possibly Uninitialized: " << uninitialized << "\n\n";

  os << "Refactoring Opportunities: " << refactoring_opportunities_.size() << "\n";

  size_t by_type[5] = {0};
  for (const auto& opp : refactoring_opportunities_) {
    by_type[static_cast<size_t>(opp.type)]++;
  }
  os << "  Unused Variables: " << by_type[0] << "\n";
  os << "  Uninitialized Variables: " << by_type[1] << "\n";
  os << "  Dead Code: " << by_type[2] << "\n";
  os << "  Variables Never Read: " << by_type[3] << "\n";
  os << "  Variables Never Written: " << by_type[4] << "\n\n";

  os << "Dead Code Instances: " << dead_code_.size() << "\n";
}

void DataFlowAnalysis::merge(const DataFlowAnalysis& other) {
  // Merge variables
  for (const auto& var_pair : other.variables_) {
    const std::string& key = var_pair.first;
    const VariableInfo& other_info = var_pair.second;

    auto it = variables_.find(key);
    if (it == variables_.end()) {
      // Variable doesn't exist in this analysis, just copy it
      variables_[key] = other_info;
    } else {
      // Variable exists, merge the information
      VariableInfo& this_info = it->second;

      // Merge uses
      this_info.uses.insert(this_info.uses.end(),
                           other_info.uses.begin(), other_info.uses.end());

      // Merge definitions
      this_info.definitions.insert(this_info.definitions.end(),
                                   other_info.definitions.begin(), other_info.definitions.end());

      // Update state if other has more advanced state
      if (static_cast<int>(other_info.state) > static_cast<int>(this_info.state)) {
        this_info.state = other_info.state;
      }

      // Update flags
      this_info.has_initializer = this_info.has_initializer || other_info.has_initializer;
    }
  }

  // Merge dead code
  dead_code_.insert(dead_code_.end(), other.dead_code_.begin(), other.dead_code_.end());

  // Note: refactoring opportunities will be regenerated after merge
}

} // namespace analysis
} // namespace optiweave
