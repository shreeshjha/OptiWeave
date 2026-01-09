#ifndef OPTIWEAVE_ANALYSIS_DEPENDENCY_GRAPH_HPP
#define OPTIWEAVE_ANALYSIS_DEPENDENCY_GRAPH_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <llvm/Support/raw_ostream.h>

namespace optiweave {
namespace analysis {

/**
 * Represents a single file node in the dependency graph
 */
struct DependencyNode {
    std::string filepath;                        // Absolute or canonical path
    std::unordered_set<std::string> includes;    // Files this file includes
    std::unordered_set<std::string> included_by; // Files that include this file
    bool is_system_header = false;               // Is this a system header?
    int include_depth = -1;                      // Distance from root files (-1 = not calculated)

    // Metrics
    size_t fan_out = 0;  // Number of files this includes (coupling out)
    size_t fan_in = 0;   // Number of files that include this (coupling in)
};

/**
 * Dependency graph tracking file inclusion relationships
 */
class DependencyGraph {
public:
    DependencyGraph() = default;

    /**
     * Add a file to the graph
     */
    void add_file(const std::string& filepath, bool is_system_header = false);

    /**
     * Record an include relationship: from_file includes to_file
     */
    void add_include(const std::string& from_file, const std::string& to_file);

    /**
     * Get a specific node (returns nullptr if not found)
     */
    const DependencyNode* get_node(const std::string& filepath) const;

    /**
     * Get all files in the graph
     */
    std::vector<std::string> get_all_files() const;

    /**
     * Detect circular dependencies (include cycles)
     * Returns a list of cycles, where each cycle is a list of filepaths
     */
    std::vector<std::vector<std::string>> detect_circular_dependencies() const;

    /**
     * Calculate include depths from root files (files not included by any other file)
     */
    void calculate_include_depths();

    /**
     * Calculate coupling metrics (fan-in, fan-out)
     */
    void calculate_metrics();

    /**
     * Export dependency graph to DOT format (GraphViz)
     */
    void export_dot(const std::string& filename, bool include_system_headers = false) const;

    /**
     * Export dependency graph to JSON format
     */
    void export_json(const std::string& filename, bool include_system_headers = false) const;

    /**
     * Print statistics to output stream
     */
    void print_statistics(llvm::raw_ostream& os, bool include_system_headers = false) const;

    /**
     * Get root files (files not included by any other user file)
     */
    std::vector<std::string> get_root_files(bool include_system_headers = false) const;

    /**
     * Get leaf files (files that don't include any other user files)
     */
    std::vector<std::string> get_leaf_files(bool include_system_headers = false) const;

    /**
     * Get files with highest coupling (most dependencies)
     */
    std::vector<std::pair<std::string, size_t>> get_highest_coupling(size_t top_n = 10, bool include_system_headers = false) const;

private:
    std::unordered_map<std::string, DependencyNode> nodes_;

    // Helper for cycle detection (DFS)
    bool dfs_detect_cycle(
        const std::string& node,
        std::unordered_set<std::string>& visited,
        std::unordered_set<std::string>& recursion_stack,
        std::vector<std::string>& current_path,
        std::vector<std::vector<std::string>>& cycles
    ) const;
};

} // namespace analysis
} // namespace optiweave

#endif // OPTIWEAVE_ANALYSIS_DEPENDENCY_GRAPH_HPP
