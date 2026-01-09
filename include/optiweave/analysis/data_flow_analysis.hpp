#pragma once

#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Analysis/CFG.h>
#include <llvm/Support/raw_ostream.h>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace optiweave {
namespace analysis {

enum class VariableState {
  Declared,      // Variable declared but not initialized
  Initialized,   // Variable initialized
  Used,          // Variable used after initialization
  Unused,        // Variable initialized but never used
  MaybeUninitialized // Variable may be used before initialization
};

struct VariableUse {
  clang::SourceLocation location;
  std::string context; // Context where variable is used (e.g., function name)
  bool is_read;        // true if reading, false if writing

  VariableUse(clang::SourceLocation loc, const std::string& ctx, bool read)
      : location(loc), context(ctx), is_read(read) {}
};

struct VariableInfo {
  std::string name;
  std::string type;
  clang::SourceLocation declaration_loc;
  std::string function_context; // Function where variable is declared
  bool has_initializer = false;
  bool is_parameter = false;
  bool is_global = false;
  VariableState state = VariableState::Declared;
  std::vector<VariableUse> uses;
  std::vector<VariableUse> definitions;

  VariableInfo() = default;
  VariableInfo(const std::string& n, const std::string& t,
               clang::SourceLocation loc, const std::string& ctx)
      : name(n), type(t), declaration_loc(loc), function_context(ctx) {}

  void add_use(clang::SourceLocation loc, const std::string& ctx, bool is_read);
  void add_definition(clang::SourceLocation loc, const std::string& ctx);
  bool is_used() const { return !uses.empty(); }
  bool is_defined() const { return has_initializer || !definitions.empty(); }
};

struct DeadCodeInfo {
  clang::SourceLocation location;
  std::string description;
  std::string function_context;

  DeadCodeInfo(clang::SourceLocation loc, const std::string& desc,
               const std::string& ctx)
      : location(loc), description(desc), function_context(ctx) {}
};

struct RefactoringOpportunity {
  enum class Type {
    UnusedVariable,
    UninitializedVariable,
    DeadCode,
    VariableNeverRead,
    VariableNeverWritten
  };

  Type type;
  clang::SourceLocation location;
  std::string description;
  std::string suggestion;
  std::string function_context;

  RefactoringOpportunity(Type t, clang::SourceLocation loc,
                        const std::string& desc, const std::string& sugg,
                        const std::string& ctx)
      : type(t), location(loc), description(desc), suggestion(sugg),
        function_context(ctx) {}
};

class DataFlowAnalysis {
public:
  DataFlowAnalysis() = default;

  // Add variable information
  void add_variable(const clang::VarDecl* decl, const std::string& function_context);

  // Track variable uses
  void record_variable_use(const clang::DeclRefExpr* expr,
                          const std::string& function_context,
                          bool is_read);

  // Track variable definitions/writes
  void record_variable_definition(const clang::DeclRefExpr* expr,
                                  const std::string& function_context);

  // Analyze a function for data flow issues
  void analyze_function(const clang::FunctionDecl* func,
                       clang::ASTContext& context);

  // Detect issues
  void detect_uninitialized_variables();
  void detect_unused_variables();
  void detect_dead_code();

  // Generate refactoring opportunities
  void generate_refactoring_opportunities();

  // Export results
  void export_json(const std::string& filename, clang::SourceManager& sm) const;
  void export_text(const std::string& filename, clang::SourceManager& sm) const;
  void print_statistics(llvm::raw_ostream& os, clang::SourceManager& sm) const;

  // Getters
  const std::map<std::string, VariableInfo>& get_variables() const {
    return variables_;
  }
  const std::vector<RefactoringOpportunity>& get_opportunities() const {
    return refactoring_opportunities_;
  }
  const std::vector<DeadCodeInfo>& get_dead_code() const {
    return dead_code_;
  }

  // Merge another data flow analysis into this one
  void merge(const DataFlowAnalysis& other);

private:
  // Key: unique variable identifier (function_name::var_name or ::global_var_name)
  std::map<std::string, VariableInfo> variables_;
  std::vector<DeadCodeInfo> dead_code_;
  std::vector<RefactoringOpportunity> refactoring_opportunities_;

  // Helper methods
  std::string get_variable_key(const std::string& var_name,
                               const std::string& function_context) const;
  std::string get_location_string(clang::SourceLocation loc,
                                 clang::SourceManager& sm) const;

  // CFG-based analysis helpers
  void analyze_cfg(const clang::CFG* cfg, const clang::FunctionDecl* func,
                  clang::ASTContext& context);
  bool is_unreachable_block(const clang::CFGBlock* block) const;
};

} // namespace analysis
} // namespace optiweave
