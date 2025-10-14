#pragma once

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <cmath>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <optiweave/analysis/pattern_detector.hpp>
#include <optiweave/analysis/call_graph.hpp>
#include <optiweave/analysis/dependency_graph.hpp>
#include <optiweave/analysis/data_flow_analysis.hpp>
#include <optiweave/analysis/memory_profiler.hpp>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>

namespace optiweave::core {

/**
    @brief Configuration for AST transformation
*/
struct TransformationConfig {
  bool transform_array_subscripts = true;
  bool transform_arithmetic_operators = false;
  bool transform_assignment_operators = false;
  bool transform_comparisons_operators = false; // Fixed typo
  // Generate wrappers that ensure single-evaluation of operands
  // and preserve value categories (via lambda + auto&& temporaries)
  bool evaluation_safe_wrappers = true;
  bool preserve_templates = true;
  bool skip_system_headers = true;
  bool enable_call_graph = false;       // Enable call graph generation
  bool enable_dependency_graph = false; // Enable dependency graph generation
  bool enable_data_flow_analysis = false; // Enable data flow analysis
  bool enable_memory_profiling = false; // Enable memory profiling
  std::string prelude_path;
  std::vector<std::string> include_paths;
};

/**
    @brief Statistics collected during transformation
*/
struct TransformationStats {
  size_t array_subscripts_transformed = 0;
  size_t arithmetic_ops_transformed = 0;
  size_t template_instantiations_skipped = 0;
  size_t errors_encountered = 0;

  void reset() { *this = TransformationStats{}; }

  void print(llvm::raw_ostream &os) const;
};

/**
    @brief AST visitor for operator instrumentation
    This visitor implements a post-order traversal to ensure inner expressions
   are processed before outer ones
*/

class ModernASTVisitor : public clang::RecursiveASTVisitor<ModernASTVisitor> {
public:
  explicit ModernASTVisitor(clang::Rewriter &rewriter,
                            clang::ASTContext &context,
                            const TransformationConfig &config = {});
  ~ModernASTVisitor() = default;

  // Disable copy/move to avoid issues with references
  ModernASTVisitor(const ModernASTVisitor &) = delete;
  ModernASTVisitor &operator=(const ModernASTVisitor &) = delete;
  ModernASTVisitor(ModernASTVisitor &&) = delete;
  ModernASTVisitor &operator=(ModernASTVisitor &&) = delete;

  /**
      @brief Configure post-order traversal
      @return true to enable post-order traversal
  */

  bool shouldTraversePostOrder() const { return true; }
  /**
      @brief Visit array subscript expressions
      @param expr The array subscript expression
      @return true to continue traversal
  */

  bool VisitArraySubscriptExpr(clang::ArraySubscriptExpr *expr);

  /**
      @brief Visit binary operators (arithmetic, assignment, etc. )
      @param expr The binary operator expression
      @return true to continue traversal
  */

  bool VisitBinaryOperator(clang::BinaryOperator *expr);

  /**
      @brief Visit unary operators
      @param expr The unary operator expression
      @return true to continue traversal
  */

  bool VisitUnaryOperator(clang::UnaryOperator *expr);
  /**
      @brief Visit C++ operator call expressions (overloaded operators)
      @param expr The operator call expression
      @return true to continue traversal
  */

  bool VisitCXXOperatorCallExpr(clang::CXXOperatorCallExpr *expr);

  /**
      @brief Visit for loops
      @param stmt The for statement
      @return true to continue traversal
  */
  bool VisitForStmt(clang::ForStmt *stmt);

  /**
      @brief Visit while loops
      @param stmt The while statement
      @return true to continue traversal
  */
  bool VisitWhileStmt(clang::WhileStmt *stmt);

  /**
      @brief Visit do-while loops
      @param stmt The do-while statement
      @return true to continue traversal
  */
  bool VisitDoStmt(clang::DoStmt *stmt);

  /**
      @brief Get transformation Statistics
      @return const reference to stats
  */

  const TransformationStats &getStats() const { return stats_; }

  /**
      @brief Reset transformation statistics
  */

  void resetStats() { stats_.reset(); }

  /**
      @brief Get collected loop information
      @return const reference to loop info vector
  */
  const std::vector<optiweave::analysis::LoopInfo>& getLoopInfo() const { return loop_info_; }

  /**
      @brief Get the call graph builder
      @return reference to call graph builder
  */
  optiweave::analysis::CallGraphBuilder& getCallGraphBuilder() { return call_graph_builder_; }
  const optiweave::analysis::CallGraphBuilder& getCallGraphBuilder() const { return call_graph_builder_; }

  /**
      @brief Visit function declarations (for call graph)
      @param decl The function declaration
      @return true to continue traversal
  */
  bool VisitFunctionDecl(clang::FunctionDecl *decl);

  /**
      @brief Visit call expressions (for call graph)
      @param expr The call expression
      @return true to continue traversal
  */
  bool VisitCallExpr(clang::CallExpr *expr);

  /**
      @brief Visit variable declarations (for data flow analysis)
      @param decl The variable declaration
      @return true to continue traversal
  */
  bool VisitVarDecl(clang::VarDecl *decl);

  /**
      @brief Visit declaration reference expressions (for data flow analysis)
      @param expr The declaration reference expression
      @return true to continue traversal
  */
  bool VisitDeclRefExpr(clang::DeclRefExpr *expr);

  /**
      @brief Get the data flow analysis
      @return reference to data flow analysis
  */
  optiweave::analysis::DataFlowAnalysis& getDataFlowAnalysis() { return data_flow_analysis_; }
  const optiweave::analysis::DataFlowAnalysis& getDataFlowAnalysis() const { return data_flow_analysis_; }

  /**
      @brief Visit CXX new expressions (for memory profiling)
      @param expr The new expression
      @return true to continue traversal
  */
  bool VisitCXXNewExpr(clang::CXXNewExpr *expr);

  /**
      @brief Visit CXX delete expressions (for memory profiling)
      @param expr The delete expression
      @return true to continue traversal
  */
  bool VisitCXXDeleteExpr(clang::CXXDeleteExpr *expr);

  /**
      @brief Get the memory profiler
      @return reference to memory profiler
  */
  optiweave::analysis::MemoryProfiler& getMemoryProfiler() { return memory_profiler_; }
  const optiweave::analysis::MemoryProfiler& getMemoryProfiler() const { return memory_profiler_; }

private:
  clang::Rewriter &rewriter_;
  clang::ASTContext &context_;
  TransformationConfig config_;
  TransformationStats stats_;

  // Track processed source ranges to avoid double-processing
  std::set<std::pair<unsigned, unsigned>> processed_ranges_;

  // Loop analysis data
  std::vector<optiweave::analysis::LoopInfo> loop_info_;
  int current_loop_nesting_ = 0;
  optiweave::analysis::LoopInfo* current_loop_ = nullptr;

  // Call graph builder
  optiweave::analysis::CallGraphBuilder call_graph_builder_;
  std::string current_function_name_;

  // Data flow analysis
  optiweave::analysis::DataFlowAnalysis data_flow_analysis_;

  // Memory profiler
  optiweave::analysis::MemoryProfiler memory_profiler_;

  /**
      @brief Check if we should skip this expression based on context
      @param expr The expression to check
      @return true if should skip
  */

  bool shouldSkipExpression(const clang::Expr *expr) const;

  /**
      @brief Check if expression is in a system header
      @param expr The expression to check
      @return true if in system header
  */

  bool isInSystemHeader(const clang::Expr *expr) const;

  /**
      @brief Check if statement is in a system header
      @param stmt The statement to check
      @return true if in system header
  */

  bool isInSystemHeader(const clang::Stmt *stmt) const;

  /**
      @brief Check if expression is already processed
      @param expr The expression to check
      @return true if already processed
  */

  bool isAlreadyProcessed(const clang::Expr *expr);

  /**
      @brief Mark expression as processed
      @param expr The expression to mark
  */

  void markAsProcessed(const clang::Expr *expr);

  /**
      @brief Transform array subscript expression
      @param expr The array subscript expression
      @return true on success
  */

  bool transformArraySubscript(clang::ArraySubscriptExpr *expr);

  /**
      @brief Transform binary operator expression
      @param expr The binary operator expression
      @return true on success
  */

  bool transformBinaryOperator(clang::BinaryOperator *expr);

  /**
      @brief Generate instrumentation code for array subscript
      @param expr The array subscript expression (for source location)
      @param lhs_type The type of the left-hand side
      @param lhs_text The text of the left-hand side
      @param rhs_text The text of the right-hand side
      @return Generated instrumentation code
  */
  std::string
  generateArraySubscriptInstrumentation(const clang::ArraySubscriptExpr *expr,
                                        clang::QualType lhs_type,
                                        llvm::StringRef lhs_text,
                                        llvm::StringRef rhs_text) const;

  /**
      @brief Get source location as string literals for instrumentation
      @param loc The source location
      @return String containing "file", line, __FUNCTION__
  */
  std::string getSourceLocationLiterals(clang::SourceLocation loc) const;

  /**
      @brief Generate instrumentation code for binary operator
      @param op The binary operator kind
      @param lhs_type The type of the left-hand side
      @param rhs_type The type of the right-hand side
      @param lhs_text The text of the left-hand side
      @param rhs_text The text of the right-hand side
      @return Generated instrumentation code
   */

  std::string generateBinaryOperatorInstrumentation(
      clang::BinaryOperatorKind op, clang::QualType lhs_type,
      clang::QualType rhs_type, llvm::StringRef lhs_text,
      llvm::StringRef rhs_text) const;

  /**
      @brief Analyze loop for patterns
      @param loop The loop statement
  */
  void analyzeLoop(clang::Stmt *loop_body, clang::SourceLocation loc);

  /**
      @brief Check if statement contains divisions
      @param stmt The statement to check
      @return true if divisions found
  */
  bool containsDivisions(clang::Stmt *stmt) const;

  /**
      @brief Check if statement contains strided access
      @param stmt The statement to check
      @return true if strided access found
  */
  bool containsStridedAccess(clang::Stmt *stmt) const;

  /**
    @brief Check if type is template-dependent
    @param type The type to check
    @return true if dependent
   */

  bool isTemplateDependentType(clang::QualType type) const;

  /**
    @brief Get safe replacement text for source range
    @param range The source range
    @return Text content
   */
  std::string getSourceText(clang::SourceRange range) const;
};

/**
 * @brief Preprocessor callback for tracking #include directives
 */
class DependencyTrackerPPCallbacks : public clang::PPCallbacks {
public:
  explicit DependencyTrackerPPCallbacks(
      optiweave::analysis::DependencyGraph* dep_graph,
      clang::SourceManager& source_manager)
      : dep_graph_(dep_graph), source_manager_(source_manager) {}

  void InclusionDirective(
      clang::SourceLocation hash_loc,
      const clang::Token& include_tok,
      llvm::StringRef file_name,
      bool is_angled,
      clang::CharSourceRange filename_range,
      clang::OptionalFileEntryRef file,
      llvm::StringRef search_path,
      llvm::StringRef relative_path,
      const clang::Module* imported,
      clang::SrcMgr::CharacteristicKind file_type) override;

private:
  optiweave::analysis::DependencyGraph* dep_graph_;
  clang::SourceManager& source_manager_;
};

/**
  @brief AST Consumer that owns and manages the visitor
 */
class TransformationConsumer : public clang::ASTConsumer {
public:
  explicit TransformationConsumer(clang::Rewriter &rewriter,
                                  clang::ASTContext &context,
                                  const TransformationConfig &config = {});

  void HandleTranslationUnit(clang::ASTContext &context) override;

  const TransformationStats &getStats() const;

  /**
      @brief Get the call graph builder from the visitor
      @return pointer to call graph builder, or nullptr if visitor not created yet
  */
  optiweave::analysis::CallGraphBuilder* getCallGraphBuilder();

  /**
      @brief Get the dependency graph (if enabled)
      @return pointer to dependency graph, or nullptr if not enabled
  */
  optiweave::analysis::DependencyGraph* getDependencyGraph();

  /**
      @brief Get the data flow analysis from the visitor
      @return pointer to data flow analysis, or nullptr if visitor not created yet
  */
  optiweave::analysis::DataFlowAnalysis* getDataFlowAnalysis();

  /**
      @brief Get the memory profiler from the visitor
      @return pointer to memory profiler, or nullptr if visitor not created yet
  */
  optiweave::analysis::MemoryProfiler* getMemoryProfiler();

private:
  clang::Rewriter &rewriter_;
  std::unique_ptr<ModernASTVisitor> visitor_;
  clang::ASTContext &context_;
  TransformationConfig config_;
  std::unique_ptr<optiweave::analysis::DependencyGraph> dependency_graph_;
};

} // namespace optiweave::core
