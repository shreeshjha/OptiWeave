#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <ostream>
#include <iostream>
#include <llvm/Support/raw_ostream.h>

namespace optiweave {
namespace analysis {

/**
 * @brief Represents a single node in the call graph (a function)
 */
struct CallGraphNode {
    std::string function_name;      // Fully qualified function name
    std::string file;                // Source file
    int line;                        // Line number where function is defined
    bool is_template;                // Whether this is a template function
    bool is_virtual;                 // Whether this is a virtual function

    std::unordered_set<std::string> callees;  // Functions this function calls
    std::unordered_set<std::string> callers;  // Functions that call this function

    // Runtime statistics (if available)
    size_t call_count = 0;           // Number of times called (runtime)
    uint64_t total_time_ns = 0;      // Total time spent (runtime)

    CallGraphNode() : line(0), is_template(false), is_virtual(false) {}

    CallGraphNode(const std::string& name, const std::string& file_path, int line_num)
        : function_name(name), file(file_path), line(line_num),
          is_template(false), is_virtual(false) {}
};

/**
 * @brief Represents the complete call graph of a program
 *
 * The call graph tracks all function definitions and the call relationships
 * between them. It can be built during AST analysis and augmented with
 * runtime statistics during program execution.
 */
class CallGraph {
public:
    CallGraph() = default;

    /**
     * @brief Add a function definition to the call graph
     *
     * @param function_name Fully qualified function name
     * @param file Source file path
     * @param line Line number
     * @param is_template Whether this is a template function
     * @param is_virtual Whether this is a virtual function
     */
    void add_function(const std::string& function_name,
                     const std::string& file,
                     int line,
                     bool is_template = false,
                     bool is_virtual = false);

    /**
     * @brief Add a function call edge to the graph
     *
     * @param caller Function making the call
     * @param callee Function being called
     */
    void add_call(const std::string& caller, const std::string& callee);

    /**
     * @brief Update runtime statistics for a function
     *
     * @param function_name Function to update
     * @param call_count Number of times called
     * @param total_time_ns Total execution time in nanoseconds
     */
    void update_runtime_stats(const std::string& function_name,
                             size_t call_count,
                             uint64_t total_time_ns);

    /**
     * @brief Check if a function exists in the graph
     */
    bool has_function(const std::string& function_name) const;

    /**
     * @brief Get a function node
     */
    const CallGraphNode* get_function(const std::string& function_name) const;

    /**
     * @brief Get all function names in the graph
     */
    std::vector<std::string> get_all_functions() const;

    /**
     * @brief Find entry points (functions with no callers)
     */
    std::vector<std::string> find_entry_points() const;

    /**
     * @brief Find leaf functions (functions that call no other functions)
     */
    std::vector<std::string> find_leaf_functions() const;

    /**
     * @brief Detect cycles in the call graph (recursive calls)
     *
     * @return List of cycles, where each cycle is a list of function names
     */
    std::vector<std::vector<std::string>> detect_cycles() const;

    /**
     * @brief Calculate call depth for each function
     *
     * Call depth is the longest path from an entry point to the function.
     *
     * @return Map from function name to maximum call depth
     */
    std::unordered_map<std::string, int> calculate_call_depths() const;

    /**
     * @brief Export call graph to DOT format (GraphViz)
     *
     * @param filename Output file path
     * @param include_runtime_stats Whether to include runtime statistics
     */
    void export_dot(const std::string& filename, bool include_runtime_stats = false) const;

    /**
     * @brief Export call graph to JSON format
     *
     * @param filename Output file path
     */
    void export_json(const std::string& filename) const;

    /**
     * @brief Export call graph to HTML interactive visualization
     *
     * @param filename Output HTML file path
     */
    void export_html(const std::string& filename) const;

    /**
     * @brief Print call graph statistics to terminal
     */
    void print_statistics(llvm::raw_ostream& os) const;

    /**
     * @brief Get the number of functions in the graph
     */
    size_t size() const { return nodes_.size(); }

    /**
     * @brief Get the total number of call edges
     */
    size_t edge_count() const;

private:
    std::unordered_map<std::string, CallGraphNode> nodes_;

    // Helper for cycle detection (DFS-based)
    void detect_cycles_dfs(const std::string& node,
                          std::unordered_set<std::string>& visited,
                          std::unordered_set<std::string>& rec_stack,
                          std::vector<std::string>& current_path,
                          std::vector<std::vector<std::string>>& cycles) const;

    // Helper for call depth calculation
    int calculate_depth_dfs(const std::string& node,
                           std::unordered_map<std::string, int>& depths,
                           std::unordered_set<std::string>& visited) const;
};

/**
 * @brief Builder class for constructing call graphs during AST traversal
 *
 * This class is used by the AST visitor to build the call graph incrementally
 * as it traverses the AST.
 */
class CallGraphBuilder {
public:
    CallGraphBuilder() = default;

    /**
     * @brief Set the current function being analyzed
     */
    void enter_function(const std::string& function_name,
                       const std::string& file,
                       int line,
                       bool is_template = false,
                       bool is_virtual = false);

    /**
     * @brief Clear the current function context
     */
    void exit_function();

    /**
     * @brief Record a function call in the current function
     */
    void record_call(const std::string& callee);

    /**
     * @brief Get the constructed call graph
     */
    CallGraph& get_graph() { return graph_; }
    const CallGraph& get_graph() const { return graph_; }

private:
    CallGraph graph_;
    std::string current_function_;  // Currently analyzed function
};

} // namespace analysis
} // namespace optiweave
