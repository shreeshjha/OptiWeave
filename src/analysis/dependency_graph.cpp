#include "optiweave/analysis/dependency_graph.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <queue>
#include <stack>

namespace optiweave {
namespace analysis {

void DependencyGraph::add_file(const std::string& filepath, bool is_system_header) {
    if (nodes_.find(filepath) == nodes_.end()) {
        DependencyNode node;
        node.filepath = filepath;
        node.is_system_header = is_system_header;
        nodes_[filepath] = node;
    }
}

void DependencyGraph::add_include(const std::string& from_file, const std::string& to_file) {
    // Ensure both files exist in the graph
    add_file(from_file, false);  // from_file is user code

    // Add the include relationship
    nodes_[from_file].includes.insert(to_file);

    // Only add reverse relationship if to_file exists
    if (nodes_.find(to_file) != nodes_.end()) {
        nodes_[to_file].included_by.insert(from_file);
    }
}

const DependencyNode* DependencyGraph::get_node(const std::string& filepath) const {
    auto it = nodes_.find(filepath);
    if (it != nodes_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> DependencyGraph::get_all_files() const {
    std::vector<std::string> files;
    files.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        files.push_back(pair.first);
    }
    return files;
}

bool DependencyGraph::dfs_detect_cycle(
    const std::string& node,
    std::unordered_set<std::string>& visited,
    std::unordered_set<std::string>& recursion_stack,
    std::vector<std::string>& current_path,
    std::vector<std::vector<std::string>>& cycles
) const {
    visited.insert(node);
    recursion_stack.insert(node);
    current_path.push_back(node);

    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        for (const auto& neighbor : it->second.includes) {
            // Only follow edges to nodes in our graph
            if (nodes_.find(neighbor) == nodes_.end()) {
                continue;
            }

            if (recursion_stack.find(neighbor) != recursion_stack.end()) {
                // Found a cycle! Extract it from current_path
                std::vector<std::string> cycle;
                bool in_cycle = false;
                for (const auto& path_node : current_path) {
                    if (path_node == neighbor) {
                        in_cycle = true;
                    }
                    if (in_cycle) {
                        cycle.push_back(path_node);
                    }
                }
                cycle.push_back(neighbor);  // Complete the cycle
                cycles.push_back(cycle);
                // Continue to find more cycles
            } else if (visited.find(neighbor) == visited.end()) {
                dfs_detect_cycle(neighbor, visited, recursion_stack, current_path, cycles);
            }
        }
    }

    current_path.pop_back();
    recursion_stack.erase(node);
    return false;
}

std::vector<std::vector<std::string>> DependencyGraph::detect_circular_dependencies() const {
    std::vector<std::vector<std::string>> cycles;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursion_stack;
    std::vector<std::string> current_path;

    for (const auto& pair : nodes_) {
        if (visited.find(pair.first) == visited.end()) {
            dfs_detect_cycle(pair.first, visited, recursion_stack, current_path, cycles);
        }
    }

    return cycles;
}

void DependencyGraph::calculate_include_depths() {
    // BFS from root files (files not included by any other user file)
    std::queue<std::string> to_process;

    // Find root files and initialize their depths
    for (auto& pair : nodes_) {
        if (pair.second.included_by.empty()) {
            pair.second.include_depth = 0;
            to_process.push(pair.first);
        } else {
            pair.second.include_depth = -1;  // Not yet calculated
        }
    }

    // BFS to calculate depths
    while (!to_process.empty()) {
        std::string current = to_process.front();
        to_process.pop();

        auto it = nodes_.find(current);
        if (it == nodes_.end()) continue;

        int current_depth = it->second.include_depth;

        for (const auto& included : it->second.includes) {
            auto included_it = nodes_.find(included);
            if (included_it == nodes_.end()) continue;

            int new_depth = current_depth + 1;
            if (included_it->second.include_depth == -1 || new_depth < included_it->second.include_depth) {
                included_it->second.include_depth = new_depth;
                to_process.push(included);
            }
        }
    }
}

void DependencyGraph::calculate_metrics() {
    for (auto& pair : nodes_) {
        auto& node = pair.second;
        node.fan_out = node.includes.size();
        node.fan_in = node.included_by.size();
    }
}

std::vector<std::string> DependencyGraph::get_root_files(bool include_system_headers) const {
    std::vector<std::string> roots;
    for (const auto& pair : nodes_) {
        if (!include_system_headers && pair.second.is_system_header) {
            continue;
        }
        if (pair.second.included_by.empty()) {
            roots.push_back(pair.first);
        }
    }
    return roots;
}

std::vector<std::string> DependencyGraph::get_leaf_files(bool include_system_headers) const {
    std::vector<std::string> leaves;
    for (const auto& pair : nodes_) {
        if (!include_system_headers && pair.second.is_system_header) {
            continue;
        }
        if (pair.second.includes.empty()) {
            leaves.push_back(pair.first);
        }
    }
    return leaves;
}

std::vector<std::pair<std::string, size_t>> DependencyGraph::get_highest_coupling(size_t top_n, bool include_system_headers) const {
    std::vector<std::pair<std::string, size_t>> coupling;

    for (const auto& pair : nodes_) {
        if (!include_system_headers && pair.second.is_system_header) {
            continue;
        }
        size_t total_coupling = pair.second.fan_in + pair.second.fan_out;
        coupling.push_back({pair.first, total_coupling});
    }

    // Sort by coupling (descending)
    std::sort(coupling.begin(), coupling.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Return top N
    if (coupling.size() > top_n) {
        coupling.resize(top_n);
    }

    return coupling;
}

void DependencyGraph::export_dot(const std::string& filename, bool include_system_headers) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }

    out << "digraph DependencyGraph {\n";
    out << "    rankdir=LR;\n";
    out << "    node [shape=box, style=rounded];\n\n";

    // Write nodes with colors based on type
    for (const auto& pair : nodes_) {
        if (!include_system_headers && pair.second.is_system_header) {
            continue;
        }

        const auto& node = pair.second;
        std::string label = node.filepath;

        // Shorten path for display
        size_t last_slash = label.find_last_of("/\\");
        if (last_slash != std::string::npos) {
            label = label.substr(last_slash + 1);
        }

        out << "    \"" << node.filepath << "\" [label=\"" << label << "\"";

        if (node.is_system_header) {
            out << ", color=gray, fontcolor=gray";
        } else if (node.included_by.empty()) {
            out << ", color=green, fontcolor=green";  // Root file
        } else if (node.includes.empty()) {
            out << ", color=blue, fontcolor=blue";    // Leaf file
        }

        out << "];\n";
    }

    out << "\n";

    // Write edges
    for (const auto& pair : nodes_) {
        if (!include_system_headers && pair.second.is_system_header) {
            continue;
        }

        const auto& node = pair.second;
        for (const auto& included : node.includes) {
            // Skip system headers if requested
            auto included_it = nodes_.find(included);
            if (!include_system_headers && included_it != nodes_.end() && included_it->second.is_system_header) {
                continue;
            }

            out << "    \"" << node.filepath << "\" -> \"" << included << "\";\n";
        }
    }

    out << "}\n";
    out.close();
}

void DependencyGraph::export_json(const std::string& filename, bool include_system_headers) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }

    out << "{\n";
    out << "  \"files\": [\n";

    bool first_file = true;
    for (const auto& pair : nodes_) {
        if (!include_system_headers && pair.second.is_system_header) {
            continue;
        }

        if (!first_file) {
            out << ",\n";
        }
        first_file = false;

        const auto& node = pair.second;
        out << "    {\n";
        out << "      \"filepath\": \"" << node.filepath << "\",\n";
        out << "      \"is_system_header\": " << (node.is_system_header ? "true" : "false") << ",\n";
        out << "      \"include_depth\": " << node.include_depth << ",\n";
        out << "      \"fan_in\": " << node.fan_in << ",\n";
        out << "      \"fan_out\": " << node.fan_out << ",\n";

        // Includes array
        out << "      \"includes\": [";
        bool first_include = true;
        for (const auto& included : node.includes) {
            if (!include_system_headers) {
                auto it = nodes_.find(included);
                if (it != nodes_.end() && it->second.is_system_header) {
                    continue;
                }
            }
            if (!first_include) out << ", ";
            first_include = false;
            out << "\"" << included << "\"";
        }
        out << "],\n";

        // Included_by array
        out << "      \"included_by\": [";
        bool first_includer = true;
        for (const auto& includer : node.included_by) {
            if (!include_system_headers) {
                auto it = nodes_.find(includer);
                if (it != nodes_.end() && it->second.is_system_header) {
                    continue;
                }
            }
            if (!first_includer) out << ", ";
            first_includer = false;
            out << "\"" << includer << "\"";
        }
        out << "]\n";
        out << "    }";
    }

    out << "\n  ]\n";
    out << "}\n";
    out.close();
}

void DependencyGraph::print_statistics(llvm::raw_ostream& os, bool include_system_headers) const {
    size_t total_files = 0;
    size_t user_files = 0;
    size_t system_files = 0;
    size_t total_includes = 0;

    for (const auto& pair : nodes_) {
        total_files++;
        if (pair.second.is_system_header) {
            system_files++;
        } else {
            user_files++;
        }

        if (include_system_headers || !pair.second.is_system_header) {
            total_includes += pair.second.includes.size();
        }
    }

    auto roots = get_root_files(include_system_headers);
    auto leaves = get_leaf_files(include_system_headers);
    auto cycles = detect_circular_dependencies();

    os << "\n=== Dependency Graph Statistics ===\n";
    os << "Total Files: " << total_files << "\n";
    os << "  User Files: " << user_files << "\n";
    os << "  System Headers: " << system_files << "\n";
    os << "Total Include Edges: " << total_includes << "\n";
    os << "Root Files: " << roots.size() << "\n";
    os << "Leaf Files: " << leaves.size() << "\n";
    os << "Circular Dependencies: " << cycles.size() << "\n";

    if (!cycles.empty()) {
        os << "\n⚠️  Circular Dependencies Detected:\n";
        for (size_t i = 0; i < cycles.size(); ++i) {
            os << "  Cycle " << (i + 1) << ": ";
            for (size_t j = 0; j < cycles[i].size(); ++j) {
                if (j > 0) os << " -> ";
                os << cycles[i][j];
            }
            os << "\n";
        }
    }

    // Show highest coupling files
    auto high_coupling = get_highest_coupling(5, include_system_headers);
    if (!high_coupling.empty()) {
        os << "\nFiles with Highest Coupling:\n";
        for (const auto& pair : high_coupling) {
            const auto* node = get_node(pair.first);
            if (node) {
                os << "  " << pair.first << " (in: " << node->fan_in
                   << ", out: " << node->fan_out << ", total: " << pair.second << ")\n";
            }
        }
    }

    os << "\n";
}

} // namespace analysis
} // namespace optiweave
