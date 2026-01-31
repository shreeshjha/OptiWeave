#include <optiweave/analysis/complexity_analyzer.hpp>
#include <optiweave/analysis/code_metrics.hpp>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Basic/SourceManager.h>
#include <algorithm>
#include <sstream>

namespace optiweave {

// ASTConsumer that runs the complexity analyzer
class ComplexityAnalysisConsumer : public clang::ASTConsumer {
public:
    ComplexityAnalysisConsumer(clang::ASTContext* context, std::shared_ptr<analysis::CodeAnalysisResult> result)
        : context_(context), result_(result) {}

    void HandleTranslationUnit(clang::ASTContext& context) override {
        analysis::ComplexityAnalyzer analyzer(&context);
        *result_ = analyzer.analyze();
    }

private:
    clang::ASTContext* context_;
    std::shared_ptr<analysis::CodeAnalysisResult> result_;
};

// Implementation of ComplexityAnalysisFrontendAction
std::unique_ptr<clang::ASTConsumer> ComplexityAnalysisFrontendAction::CreateASTConsumer(
    clang::CompilerInstance& CI,
    llvm::StringRef file) {
    return std::make_unique<ComplexityAnalysisConsumer>(&CI.getASTContext(), result_);
}

namespace analysis {

ComplexityAnalyzer::ComplexityAnalyzer(clang::ASTContext* context)
    : context_(context) {}

CodeAnalysisResult ComplexityAnalyzer::analyze() {
    // Traverse the entire AST
    TraverseDecl(context_->getTranslationUnitDecl());

    // Post-processing
    detect_recursion();
    compute_project_statistics();

    return result_;
}

bool ComplexityAnalyzer::VisitFunctionDecl(clang::FunctionDecl* func) {
    // Skip declarations without definitions
    if (!func->hasBody()) {
        return true;
    }

    // Skip compiler-generated functions
    if (func->isImplicit()) {
        return true;
    }

    // Skip system headers
    auto& SM = context_->getSourceManager();
    if (SM.isInSystemHeader(func->getLocation())) {
        return true;
    }

    // Create new function metrics
    FunctionMetrics metrics;
    metrics.location = get_source_location(func);
    metrics.function_name = func->getNameAsString();

    if (func->getReturnType().getTypePtrOrNull()) {
        metrics.return_type = func->getReturnType().getAsString();
    }

    metrics.parameter_count = func->getNumParams();

    // Set current function for visitor callbacks
    current_function_ = &metrics;
    current_nesting_depth_ = 0;

    // Reset Halstead counters
    halstead_operators_.clear();
    halstead_operands_.clear();
    halstead_operator_count_ = 0;
    halstead_operand_count_ = 0;

    // Initialize cyclomatic complexity to 1 (one path through function)
    current_function_->complexity.cyclomatic_complexity = 1;

    // Compute various metrics
    compute_lines_of_code(func);

    // Traverse the function body to compute complexity metrics
    // This will trigger Visit* methods for statements
    TraverseStmt(func->getBody());

    compute_halstead_metrics(func->getBody());

    // Store results
    result_.functions[metrics.function_name] = *current_function_;

    current_function_ = nullptr;

    return true;
}

bool ComplexityAnalyzer::VisitIfStmt(clang::IfStmt* stmt) {
    if (current_function_) {
        current_function_->complexity.decision_points++;
        current_function_->complexity.cyclomatic_complexity++;

        // Track nesting for cognitive complexity
        current_nesting_depth_++;
        current_function_->complexity.cognitive_complexity += current_nesting_depth_;

        if (current_nesting_depth_ > current_function_->complexity.max_nesting_depth) {
            current_function_->complexity.max_nesting_depth = current_nesting_depth_;
        }

        // Traverse then and else branches with proper nesting
        TraverseStmt(stmt->getThen());
        current_nesting_depth_--;

        if (stmt->getElse()) {
            current_nesting_depth_++;
            TraverseStmt(stmt->getElse());
            current_nesting_depth_--;
        }

        return false; // We handled traversal manually
    }
    return true;
}

bool ComplexityAnalyzer::VisitForStmt(clang::ForStmt* stmt) {
    if (current_function_) {
        current_function_->complexity.decision_points++;
        current_function_->complexity.cyclomatic_complexity++;

        current_nesting_depth_++;
        current_function_->complexity.cognitive_complexity += current_nesting_depth_;

        if (current_nesting_depth_ > current_function_->complexity.max_nesting_depth) {
            current_function_->complexity.max_nesting_depth = current_nesting_depth_;
        }

        // Traverse loop body
        TraverseStmt(stmt->getBody());
        current_nesting_depth_--;

        return false; // We handled traversal manually
    }
    return true;
}

bool ComplexityAnalyzer::VisitWhileStmt(clang::WhileStmt* stmt) {
    if (current_function_) {
        current_function_->complexity.decision_points++;
        current_function_->complexity.cyclomatic_complexity++;

        current_nesting_depth_++;
        current_function_->complexity.cognitive_complexity += current_nesting_depth_;

        if (current_nesting_depth_ > current_function_->complexity.max_nesting_depth) {
            current_function_->complexity.max_nesting_depth = current_nesting_depth_;
        }

        // Traverse loop body
        TraverseStmt(stmt->getBody());
        current_nesting_depth_--;

        return false; // We handled traversal manually
    }
    return true;
}

bool ComplexityAnalyzer::VisitDoStmt(clang::DoStmt* stmt) {
    if (current_function_) {
        current_function_->complexity.decision_points++;
        current_function_->complexity.cyclomatic_complexity++;

        current_nesting_depth_++;
        current_function_->complexity.cognitive_complexity += current_nesting_depth_;

        if (current_nesting_depth_ > current_function_->complexity.max_nesting_depth) {
            current_function_->complexity.max_nesting_depth = current_nesting_depth_;
        }

        // Traverse loop body
        TraverseStmt(stmt->getBody());
        current_nesting_depth_--;

        return false; // We handled traversal manually
    }
    return true;
}

bool ComplexityAnalyzer::VisitSwitchStmt(clang::SwitchStmt* stmt) {
    if (current_function_) {
        current_function_->complexity.decision_points++;
        current_function_->complexity.cyclomatic_complexity++;

        current_nesting_depth_++;

        if (current_nesting_depth_ > current_function_->complexity.max_nesting_depth) {
            current_function_->complexity.max_nesting_depth = current_nesting_depth_;
        }
    }
    return true;
}

bool ComplexityAnalyzer::VisitCaseStmt(clang::CaseStmt* stmt) {
    if (current_function_) {
        // Each case adds to cyclomatic complexity
        current_function_->complexity.cyclomatic_complexity++;
    }
    return true;
}

bool ComplexityAnalyzer::VisitConditionalOperator(clang::ConditionalOperator* op) {
    if (current_function_) {
        current_function_->complexity.decision_points++;
        current_function_->complexity.cyclomatic_complexity++;
        current_function_->complexity.cognitive_complexity++;
    }
    return true;
}

bool ComplexityAnalyzer::VisitBinaryOperator(clang::BinaryOperator* op) {
    if (current_function_) {
        // Logical AND and OR add to cyclomatic complexity
        if (op->isLogicalOp()) {
            current_function_->complexity.cyclomatic_complexity++;
            current_function_->complexity.cognitive_complexity++;
        }

        // Track Halstead operators
        halstead_operators_.insert(op->getOpcodeStr().str());
        halstead_operator_count_++;
    }
    return true;
}

bool ComplexityAnalyzer::VisitCallExpr(clang::CallExpr* call) {
    if (current_function_) {
        build_call_graph(call);

        // Track for Halstead
        halstead_operator_count_++; // Function call is an operator
    }
    return true;
}

bool ComplexityAnalyzer::VisitReturnStmt(clang::ReturnStmt* stmt) {
    if (current_function_) {
        current_function_->complexity.return_statements++;
    }
    return true;
}

void ComplexityAnalyzer::compute_cyclomatic_complexity(clang::FunctionDecl* func) {
    // Cyclomatic complexity starts at 1 (single path)
    // It's incremented by decision points during traversal
    if (current_function_) {
        current_function_->complexity.cyclomatic_complexity++;
    }
}

void ComplexityAnalyzer::compute_cognitive_complexity(clang::Stmt* body) {
    // Cognitive complexity is computed during traversal
    // It considers nesting depth and control flow breaks
    // Implementation is in Visit* methods
}

void ComplexityAnalyzer::compute_halstead_metrics(clang::Stmt* body) {
    if (!current_function_) return;

    // Halstead metrics are collected during traversal
    current_function_->halstead.n1 = halstead_operators_.size();
    current_function_->halstead.n2 = halstead_operands_.size();
    current_function_->halstead.N1 = halstead_operator_count_;
    current_function_->halstead.N2 = halstead_operand_count_;
}

void ComplexityAnalyzer::compute_lines_of_code(clang::FunctionDecl* func) {
    if (!current_function_) return;

    clang::SourceManager& sm = context_->getSourceManager();
    clang::SourceLocation start = func->getBeginLoc();
    clang::SourceLocation end = func->getEndLoc();

    if (start.isInvalid() || end.isInvalid()) {
        return;
    }

    unsigned start_line = sm.getSpellingLineNumber(start);
    unsigned end_line = sm.getSpellingLineNumber(end);

    current_function_->lines_of_code = end_line - start_line + 1;

    // For now, approximate SLOC as LOC (proper implementation would parse and skip comments)
    current_function_->source_lines_of_code = current_function_->lines_of_code;
}

void ComplexityAnalyzer::build_call_graph(clang::CallExpr* call) {
    if (!current_function_) return;

    if (clang::FunctionDecl* callee = call->getDirectCallee()) {
        std::string callee_name = callee->getNameAsString();

        // Record that current function calls callee
        if (std::find(current_function_->calls_to.begin(),
                     current_function_->calls_to.end(),
                     callee_name) == current_function_->calls_to.end()) {
            current_function_->calls_to.push_back(callee_name);
        }

        // Create call edge
        CallEdge edge;
        edge.caller = current_function_->function_name;
        edge.callee = callee_name;
        edge.location = get_source_location(call);
        result_.call_graph.push_back(edge);
    }
}

void ComplexityAnalyzer::detect_recursion() {
    // Build reverse call graph (called_by)
    for (const auto& edge : result_.call_graph) {
        if (result_.functions.find(edge.callee) != result_.functions.end()) {
            auto& callee_func = result_.functions[edge.callee];
            if (std::find(callee_func.called_by.begin(),
                         callee_func.called_by.end(),
                         edge.caller) == callee_func.called_by.end()) {
                callee_func.called_by.push_back(edge.caller);
            }
        }
    }

    // Detect direct and indirect recursion using DFS
    std::set<std::string> visited;
    std::set<std::string> recursion_stack;

    std::function<bool(const std::string&)> has_cycle;
    has_cycle = [&](const std::string& func_name) -> bool {
        if (recursion_stack.find(func_name) != recursion_stack.end()) {
            return true; // Found cycle
        }
        if (visited.find(func_name) != visited.end()) {
            return false; // Already processed
        }

        visited.insert(func_name);
        recursion_stack.insert(func_name);

        if (result_.functions.find(func_name) != result_.functions.end()) {
            const auto& func = result_.functions[func_name];
            for (const auto& callee : func.calls_to) {
                if (has_cycle(callee)) {
                    return true;
                }
            }
        }

        recursion_stack.erase(func_name);
        return false;
    };

    for (auto& [name, func] : result_.functions) {
        visited.clear();
        recursion_stack.clear();
        func.is_recursive = has_cycle(name);
    }
}

void ComplexityAnalyzer::compute_project_statistics() {
    auto& stats = result_.project_stats;

    stats.total_functions = result_.functions.size();
    stats.total_files = result_.files.size();

    double total_cc = 0.0;
    double total_cog = 0.0;
    double total_mi = 0.0;
    uint32_t total_lines = 0;
    uint32_t total_sloc = 0;

    for (const auto& [name, func] : result_.functions) {
        total_cc += func.complexity.cyclomatic_complexity;
        total_cog += func.complexity.cognitive_complexity;
        total_mi += func.maintainability_index();
        total_lines += func.lines_of_code;
        total_sloc += func.source_lines_of_code;

        if (func.complexity.cyclomatic_complexity > 20) {
            stats.complex_functions++;
        }
        if (func.complexity.cyclomatic_complexity > 50) {
            stats.very_complex_functions++;
        }
    }

    if (stats.total_functions > 0) {
        stats.avg_cyclomatic_complexity = total_cc / stats.total_functions;
        stats.avg_cognitive_complexity = total_cog / stats.total_functions;
        stats.avg_maintainability_index = total_mi / stats.total_functions;
    }

    stats.total_lines = total_lines;
    stats.total_sloc = total_sloc;
}

SourceLocation ComplexityAnalyzer::get_source_location(clang::Decl* decl) {
    clang::SourceManager& sm = context_->getSourceManager();
    clang::SourceLocation loc = decl->getLocation();

    SourceLocation result;
    if (loc.isValid()) {
        clang::PresumedLoc presumed = sm.getPresumedLoc(loc);
        if (presumed.isValid()) {
            result.file = presumed.getFilename();
            result.line = presumed.getLine();

            if (auto* named = llvm::dyn_cast<clang::NamedDecl>(decl)) {
                result.set_function(named->getNameAsString());
            }
        }
    }

    return result;
}

SourceLocation ComplexityAnalyzer::get_source_location(clang::Stmt* stmt) {
    clang::SourceManager& sm = context_->getSourceManager();
    clang::SourceLocation loc = stmt->getBeginLoc();

    SourceLocation result;
    if (loc.isValid()) {
        clang::PresumedLoc presumed = sm.getPresumedLoc(loc);
        if (presumed.isValid()) {
            result.file = presumed.getFilename();
            result.line = presumed.getLine();

            if (current_function_) {
                result.set_function(current_function_->function_name);
            }
        }
    }

    return result;
}

CodeAnalysisResult analyze_code_complexity(clang::ASTContext* context) {
    ComplexityAnalyzer analyzer(context);
    return analyzer.analyze();
}

// Helper methods for CodeAnalysisResult
std::vector<FunctionMetrics> CodeAnalysisResult::get_most_complex(size_t top_n) const {
    std::vector<FunctionMetrics> result;
    for (const auto& [name, func] : functions) {
        result.push_back(func);
    }

    std::sort(result.begin(), result.end(),
        [](const FunctionMetrics& a, const FunctionMetrics& b) {
            return a.complexity.cyclomatic_complexity > b.complexity.cyclomatic_complexity;
        });

    if (result.size() > top_n) {
        result.resize(top_n);
    }

    return result;
}

std::vector<FunctionMetrics> CodeAnalysisResult::get_least_maintainable(size_t top_n) const {
    std::vector<FunctionMetrics> result;
    for (const auto& [name, func] : functions) {
        result.push_back(func);
    }

    std::sort(result.begin(), result.end(),
        [](const FunctionMetrics& a, const FunctionMetrics& b) {
            return a.maintainability_index() < b.maintainability_index();
        });

    if (result.size() > top_n) {
        result.resize(top_n);
    }

    return result;
}

std::vector<FunctionMetrics> CodeAnalysisResult::get_hardest_to_understand(size_t top_n) const {
    std::vector<FunctionMetrics> result;
    for (const auto& [name, func] : functions) {
        result.push_back(func);
    }

    std::sort(result.begin(), result.end(),
        [](const FunctionMetrics& a, const FunctionMetrics& b) {
            return a.complexity.cognitive_complexity > b.complexity.cognitive_complexity;
        });

    if (result.size() > top_n) {
        result.resize(top_n);
    }

    return result;
}

uint32_t CodeAnalysisResult::get_max_call_depth() const {
    uint32_t max_depth = 0;
    for (const auto& [name, func] : functions) {
        if (func.call_depth > max_depth) {
            max_depth = func.call_depth;
        }
    }
    return max_depth;
}

std::vector<std::vector<std::string>> CodeAnalysisResult::find_circular_dependencies() const {
    std::vector<std::vector<std::string>> cycles;

    // Use DFS to find cycles
    std::set<std::string> visited;
    std::vector<std::string> path;
    std::set<std::string> on_stack;

    std::function<void(const std::string&)> dfs;
    dfs = [&](const std::string& func_name) {
        if (on_stack.find(func_name) != on_stack.end()) {
            // Found cycle - extract it from path
            auto it = std::find(path.begin(), path.end(), func_name);
            if (it != path.end()) {
                std::vector<std::string> cycle(it, path.end());
                cycle.push_back(func_name); // Complete the cycle
                cycles.push_back(cycle);
            }
            return;
        }

        if (visited.find(func_name) != visited.end()) {
            return;
        }

        visited.insert(func_name);
        on_stack.insert(func_name);
        path.push_back(func_name);

        if (functions.find(func_name) != functions.end()) {
            const auto& func = functions.at(func_name);
            for (const auto& callee : func.calls_to) {
                dfs(callee);
            }
        }

        path.pop_back();
        on_stack.erase(func_name);
    };

    for (const auto& [name, func] : functions) {
        if (visited.find(name) == visited.end()) {
            dfs(name);
        }
    }

    return cycles;
}

} // namespace analysis
} // namespace optiweave
