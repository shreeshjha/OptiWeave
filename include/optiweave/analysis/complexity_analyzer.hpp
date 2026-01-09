#pragma once

#include <optiweave/analysis/code_metrics.hpp>
#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include <memory>
#include <vector>

namespace optiweave {
namespace analysis {

/// AST visitor that computes complexity metrics
class ComplexityAnalyzer : public clang::RecursiveASTVisitor<ComplexityAnalyzer> {
public:
    explicit ComplexityAnalyzer(clang::ASTContext* context);

    /// Analyze a translation unit
    CodeAnalysisResult analyze();

    /// Visit function declaration
    bool VisitFunctionDecl(clang::FunctionDecl* func);

    /// Visit statements for complexity calculation
    bool VisitIfStmt(clang::IfStmt* stmt);
    bool VisitForStmt(clang::ForStmt* stmt);
    bool VisitWhileStmt(clang::WhileStmt* stmt);
    bool VisitDoStmt(clang::DoStmt* stmt);
    bool VisitSwitchStmt(clang::SwitchStmt* stmt);
    bool VisitCaseStmt(clang::CaseStmt* stmt);
    bool VisitConditionalOperator(clang::ConditionalOperator* op);
    bool VisitBinaryOperator(clang::BinaryOperator* op);
    bool VisitCallExpr(clang::CallExpr* call);
    bool VisitReturnStmt(clang::ReturnStmt* stmt);

private:
    clang::ASTContext* context_;
    CodeAnalysisResult result_;

    /// Current function being analyzed
    FunctionMetrics* current_function_ = nullptr;

    /// Nesting depth tracker
    uint32_t current_nesting_depth_ = 0;

    /// Halstead operator/operand tracking
    std::set<std::string> halstead_operators_;
    std::set<std::string> halstead_operands_;
    uint32_t halstead_operator_count_ = 0;
    uint32_t halstead_operand_count_ = 0;

    /// Helper methods
    void compute_cyclomatic_complexity(clang::FunctionDecl* func);
    void compute_cognitive_complexity(clang::Stmt* body);
    void compute_halstead_metrics(clang::Stmt* body);
    void compute_lines_of_code(clang::FunctionDecl* func);
    void build_call_graph(clang::CallExpr* call);
    void detect_recursion();
    void compute_project_statistics();

    /// Get source location as string
    SourceLocation get_source_location(clang::Decl* decl);
    SourceLocation get_source_location(clang::Stmt* stmt);

    /// Count decision points in a statement
    uint32_t count_decision_points(clang::Stmt* stmt);

    /// Calculate nesting depth increase for cognitive complexity
    uint32_t get_nesting_increment(clang::Stmt* stmt);
};

/// Standalone function to analyze a file
CodeAnalysisResult analyze_code_complexity(
    clang::ASTContext* context
);

} // namespace analysis

/// Frontend action for complexity analysis
class ComplexityAnalysisFrontendAction : public clang::ASTFrontendAction {
public:
    ComplexityAnalysisFrontendAction(std::shared_ptr<analysis::CodeAnalysisResult> result)
        : result_(result) {}

    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance& CI,
        llvm::StringRef file) override;

    const analysis::CodeAnalysisResult& getResult() const { return *result_; }

private:
    std::shared_ptr<analysis::CodeAnalysisResult> result_;
};

/// Factory for complexity analysis actions
class ComplexityAnalysisActionFactory : public clang::tooling::FrontendActionFactory {
public:
    ComplexityAnalysisActionFactory()
        : shared_result_(std::make_shared<analysis::CodeAnalysisResult>()) {}

    std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<ComplexityAnalysisFrontendAction>(shared_result_);
    }

    const analysis::CodeAnalysisResult& getResult() const {
        return *shared_result_;
    }

private:
    std::shared_ptr<analysis::CodeAnalysisResult> shared_result_;
};

namespace analysis {

/// Export analysis results to various formats
namespace export_utils {

/// Export to terminal (human-readable)
std::string export_terminal(const CodeAnalysisResult& result);

/// Export to JSON
std::string export_json(const CodeAnalysisResult& result);

/// Export to Markdown
std::string export_markdown(const CodeAnalysisResult& result);

/// Export to HTML
std::string export_html(const CodeAnalysisResult& result);

/// Export call graph to DOT format (GraphViz)
std::string export_call_graph_dot(const CodeAnalysisResult& result);

/// Export dependency graph to DOT format
std::string export_dependency_graph_dot(const CodeAnalysisResult& result);

} // namespace export_utils

} // namespace analysis
} // namespace optiweave
