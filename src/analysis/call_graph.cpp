#include "optiweave/analysis/call_graph.hpp"
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <stack>

namespace optiweave {
namespace analysis {

// =============================================================================
// CallGraph Implementation
// =============================================================================

void CallGraph::add_function(const std::string& function_name,
                             const std::string& file,
                             int line,
                             bool is_template,
                             bool is_virtual) {
    if (nodes_.find(function_name) == nodes_.end()) {
        CallGraphNode node(function_name, file, line);
        node.is_template = is_template;
        node.is_virtual = is_virtual;
        nodes_[function_name] = node;
    }
}

void CallGraph::add_call(const std::string& caller, const std::string& callee) {
    // Ensure both functions exist
    if (nodes_.find(caller) == nodes_.end()) {
        add_function(caller, "", 0);
    }
    if (nodes_.find(callee) == nodes_.end()) {
        add_function(callee, "", 0);
    }

    // Add the call edge
    nodes_[caller].callees.insert(callee);
    nodes_[callee].callers.insert(caller);
}

void CallGraph::update_runtime_stats(const std::string& function_name,
                                    size_t call_count,
                                    uint64_t total_time_ns) {
    if (nodes_.find(function_name) != nodes_.end()) {
        nodes_[function_name].call_count = call_count;
        nodes_[function_name].total_time_ns = total_time_ns;
    }
}

bool CallGraph::has_function(const std::string& function_name) const {
    return nodes_.find(function_name) != nodes_.end();
}

const CallGraphNode* CallGraph::get_function(const std::string& function_name) const {
    auto it = nodes_.find(function_name);
    return (it != nodes_.end()) ? &it->second : nullptr;
}

std::vector<std::string> CallGraph::get_all_functions() const {
    std::vector<std::string> functions;
    functions.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        functions.push_back(pair.first);
    }
    std::sort(functions.begin(), functions.end());
    return functions;
}

std::vector<std::string> CallGraph::find_entry_points() const {
    std::vector<std::string> entry_points;
    for (const auto& pair : nodes_) {
        if (pair.second.callers.empty()) {
            entry_points.push_back(pair.first);
        }
    }
    std::sort(entry_points.begin(), entry_points.end());
    return entry_points;
}

std::vector<std::string> CallGraph::find_leaf_functions() const {
    std::vector<std::string> leaf_functions;
    for (const auto& pair : nodes_) {
        if (pair.second.callees.empty()) {
            leaf_functions.push_back(pair.first);
        }
    }
    std::sort(leaf_functions.begin(), leaf_functions.end());
    return leaf_functions;
}

void CallGraph::detect_cycles_dfs(const std::string& node,
                                  std::unordered_set<std::string>& visited,
                                  std::unordered_set<std::string>& rec_stack,
                                  std::vector<std::string>& current_path,
                                  std::vector<std::vector<std::string>>& cycles) const {
    visited.insert(node);
    rec_stack.insert(node);
    current_path.push_back(node);

    auto it = nodes_.find(node);
    if (it != nodes_.end()) {
        for (const auto& callee : it->second.callees) {
            if (rec_stack.find(callee) != rec_stack.end()) {
                // Found a cycle
                auto cycle_start = std::find(current_path.begin(), current_path.end(), callee);
                std::vector<std::string> cycle(cycle_start, current_path.end());
                cycle.push_back(callee);  // Complete the cycle
                cycles.push_back(cycle);
            } else if (visited.find(callee) == visited.end()) {
                detect_cycles_dfs(callee, visited, rec_stack, current_path, cycles);
            }
        }
    }

    current_path.pop_back();
    rec_stack.erase(node);
}

std::vector<std::vector<std::string>> CallGraph::detect_cycles() const {
    std::vector<std::vector<std::string>> cycles;
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> rec_stack;
    std::vector<std::string> current_path;

    for (const auto& pair : nodes_) {
        if (visited.find(pair.first) == visited.end()) {
            detect_cycles_dfs(pair.first, visited, rec_stack, current_path, cycles);
        }
    }

    return cycles;
}

int CallGraph::calculate_depth_dfs(const std::string& node,
                                   std::unordered_map<std::string, int>& depths,
                                   std::unordered_set<std::string>& visited) const {
    // Check if already calculated
    if (depths.find(node) != depths.end()) {
        return depths[node];
    }

    // Check for cycles
    if (visited.find(node) != visited.end()) {
        return -1;  // Cycle detected
    }

    visited.insert(node);

    auto it = nodes_.find(node);
    if (it == nodes_.end() || it->second.callees.empty()) {
        depths[node] = 0;  // Leaf function
        visited.erase(node);
        return 0;
    }

    int max_depth = 0;
    for (const auto& callee : it->second.callees) {
        int callee_depth = calculate_depth_dfs(callee, depths, visited);
        if (callee_depth >= 0) {
            max_depth = std::max(max_depth, callee_depth + 1);
        }
    }

    depths[node] = max_depth;
    visited.erase(node);
    return max_depth;
}

std::unordered_map<std::string, int> CallGraph::calculate_call_depths() const {
    std::unordered_map<std::string, int> depths;
    std::unordered_set<std::string> visited;

    for (const auto& pair : nodes_) {
        if (depths.find(pair.first) == depths.end()) {
            calculate_depth_dfs(pair.first, depths, visited);
        }
    }

    return depths;
}

size_t CallGraph::edge_count() const {
    size_t count = 0;
    for (const auto& pair : nodes_) {
        count += pair.second.callees.size();
    }
    return count;
}

void CallGraph::export_dot(const std::string& filename, bool include_runtime_stats) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
        return;
    }

    file << "digraph CallGraph {" << std::endl;
    file << "    rankdir=LR;" << std::endl;
    file << "    node [shape=box, style=rounded];" << std::endl;
    file << std::endl;

    // Write nodes
    for (const auto& pair : nodes_) {
        const auto& node = pair.second;
        file << "    \"" << node.function_name << "\" [";

        if (include_runtime_stats && node.call_count > 0) {
            double time_ms = node.total_time_ns / 1000000.0;
            file << "label=\"" << node.function_name << "\\n"
                 << "Calls: " << node.call_count << "\\n"
                 << "Time: " << std::fixed << std::setprecision(2) << time_ms << "ms\"";

            // Color by time
            if (time_ms > 100) {
                file << ", fillcolor=red, style=\"rounded,filled\"";
            } else if (time_ms > 10) {
                file << ", fillcolor=orange, style=\"rounded,filled\"";
            } else if (time_ms > 1) {
                file << ", fillcolor=yellow, style=\"rounded,filled\"";
            }
        }

        file << "];" << std::endl;
    }

    file << std::endl;

    // Write edges
    for (const auto& pair : nodes_) {
        const auto& caller = pair.first;
        const auto& node = pair.second;

        for (const auto& callee : node.callees) {
            file << "    \"" << caller << "\" -> \"" << callee << "\"";

            if (node.is_virtual) {
                file << " [style=dashed, color=blue]";
            }

            file << ";" << std::endl;
        }
    }

    file << "}" << std::endl;
    file.close();

    std::cout << "Call graph exported to: " << filename << std::endl;
    std::cout << "Visualize with: dot -Tpng " << filename << " -o callgraph.png" << std::endl;
}

void CallGraph::export_json(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
        return;
    }

    file << "{" << std::endl;
    file << "  \"functions\": [" << std::endl;

    bool first_function = true;
    for (const auto& pair : nodes_) {
        if (!first_function) file << "," << std::endl;
        first_function = false;

        const auto& node = pair.second;
        file << "    {" << std::endl;
        file << "      \"name\": \"" << node.function_name << "\"," << std::endl;
        file << "      \"file\": \"" << node.file << "\"," << std::endl;
        file << "      \"line\": " << node.line << "," << std::endl;
        file << "      \"is_template\": " << (node.is_template ? "true" : "false") << "," << std::endl;
        file << "      \"is_virtual\": " << (node.is_virtual ? "true" : "false") << "," << std::endl;
        file << "      \"call_count\": " << node.call_count << "," << std::endl;
        file << "      \"total_time_ns\": " << node.total_time_ns << "," << std::endl;
        file << "      \"callees\": [";

        bool first_callee = true;
        for (const auto& callee : node.callees) {
            if (!first_callee) file << ", ";
            first_callee = false;
            file << "\"" << callee << "\"";
        }

        file << "]," << std::endl;
        file << "      \"callers\": [";

        bool first_caller = true;
        for (const auto& caller : node.callers) {
            if (!first_caller) file << ", ";
            first_caller = false;
            file << "\"" << caller << "\"";
        }

        file << "]" << std::endl;
        file << "    }";
    }

    file << std::endl << "  ]" << std::endl;
    file << "}" << std::endl;
    file.close();

    std::cout << "Call graph exported to: " << filename << std::endl;
}

void CallGraph::export_html(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing" << std::endl;
        return;
    }

    // Write HTML with embedded D3.js visualization
    file << R"(<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>OptiWeave Call Graph</title>
    <script src="https://d3js.org/d3.v7.min.js"></script>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        }
        .container {
            max-width: 1400px;
            margin: 0 auto;
            background: white;
            padding: 30px;
            border-radius: 15px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.3);
        }
        h1 {
            color: #333;
            border-bottom: 3px solid #667eea;
            padding-bottom: 10px;
        }
        #graph {
            border: 1px solid #ddd;
            border-radius: 8px;
            background: #f9f9f9;
        }
        .node circle {
            fill: #69b3a2;
            stroke: #333;
            stroke-width: 2px;
            cursor: pointer;
        }
        .node circle:hover {
            fill: #ffa500;
        }
        .node text {
            font: 12px sans-serif;
            pointer-events: none;
        }
        .link {
            fill: none;
            stroke: #999;
            stroke-opacity: 0.6;
            stroke-width: 2px;
        }
        .link:hover {
            stroke: #333;
            stroke-width: 3px;
        }
        .stats {
            margin-top: 20px;
            padding: 15px;
            background: #f0f0f0;
            border-radius: 8px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>📊 OptiWeave Call Graph</h1>
        <div class="stats">
            <strong>Total Functions:</strong> )" << nodes_.size() << R"(<br>
            <strong>Total Calls:</strong> )" << edge_count() << R"(<br>
            <strong>Entry Points:</strong> )" << find_entry_points().size() << R"(<br>
            <strong>Leaf Functions:</strong> )" << find_leaf_functions().size() << R"(
        </div>
        <svg id="graph" width="1300" height="800"></svg>
    </div>

    <script>
        const data = )";

    // Embed JSON data
    file << "{\"nodes\": [";
    bool first = true;
    for (const auto& pair : nodes_) {
        if (!first) file << ",";
        first = false;
        file << "{\"id\":\"" << pair.first << "\",\"name\":\"" << pair.first << "\"}";
    }
    file << "], \"links\": [";
    first = true;
    for (const auto& pair : nodes_) {
        for (const auto& callee : pair.second.callees) {
            if (!first) file << ",";
            first = false;
            file << "{\"source\":\"" << pair.first << "\",\"target\":\"" << callee << "\"}";
        }
    }
    file << "]}";

    file << R"(;

        // D3.js force-directed graph
        const svg = d3.select("#graph");
        const width = +svg.attr("width");
        const height = +svg.attr("height");

        const simulation = d3.forceSimulation(data.nodes)
            .force("link", d3.forceLink(data.links).id(d => d.id).distance(150))
            .force("charge", d3.forceManyBody().strength(-300))
            .force("center", d3.forceCenter(width / 2, height / 2));

        const link = svg.append("g")
            .selectAll("path")
            .data(data.links)
            .join("path")
            .attr("class", "link");

        const node = svg.append("g")
            .selectAll("g")
            .data(data.nodes)
            .join("g")
            .attr("class", "node")
            .call(d3.drag()
                .on("start", dragstarted)
                .on("drag", dragged)
                .on("end", dragended));

        node.append("circle")
            .attr("r", 8);

        node.append("text")
            .text(d => d.name)
            .attr("x", 12)
            .attr("y", 4);

        simulation.on("tick", () => {
            link.attr("d", d => `M${d.source.x},${d.source.y} L${d.target.x},${d.target.y}`);
            node.attr("transform", d => `translate(${d.x},${d.y})`);
        });

        function dragstarted(event) {
            if (!event.active) simulation.alphaTarget(0.3).restart();
            event.subject.fx = event.subject.x;
            event.subject.fy = event.subject.y;
        }

        function dragged(event) {
            event.subject.fx = event.x;
            event.subject.fy = event.y;
        }

        function dragended(event) {
            if (!event.active) simulation.alphaTarget(0);
            event.subject.fx = null;
            event.subject.fy = null;
        }
    </script>
</body>
</html>)";

    file.close();
    std::cout << "Interactive call graph exported to: " << filename << std::endl;
    std::cout << "Open in browser to view the visualization" << std::endl;
}

void CallGraph::print_statistics(llvm::raw_ostream& os) const {
    os << "╔══════════════════════════════════════════════════╗\n";
    os << "║          OptiWeave Call Graph Statistics         ║\n";
    os << "╚══════════════════════════════════════════════════╝\n";
    os << "\n";

    os << "Total Functions: " << nodes_.size() << "\n";
    os << "Total Call Edges: " << edge_count() << "\n";

    auto entry_points = find_entry_points();
    os << "Entry Points: " << entry_points.size() << "\n";
    if (!entry_points.empty() && entry_points.size() <= 10) {
        for (const auto& ep : entry_points) {
            os << "  - " << ep << "\n";
        }
    }

    auto leaf_functions = find_leaf_functions();
    os << "Leaf Functions: " << leaf_functions.size() << "\n";

    auto cycles = detect_cycles();
    os << "Cycles Detected: " << cycles.size() << "\n";
    if (!cycles.empty()) {
        os << "  Warning: Recursive calls detected!\n";
        for (size_t i = 0; i < cycles.size() && i < 5; ++i) {
            os << "  Cycle " << (i + 1) << ": ";
            for (size_t j = 0; j < cycles[i].size(); ++j) {
                if (j > 0) os << " -> ";
                os << cycles[i][j];
            }
            os << "\n";
        }
    }

    auto depths = calculate_call_depths();
    int max_depth = 0;
    for (const auto& pair : depths) {
        max_depth = std::max(max_depth, pair.second);
    }
    os << "Maximum Call Depth: " << max_depth << "\n";
}

// =============================================================================
// CallGraphBuilder Implementation
// =============================================================================

void CallGraphBuilder::enter_function(const std::string& function_name,
                                     const std::string& file,
                                     int line,
                                     bool is_template,
                                     bool is_virtual) {
    current_function_ = function_name;
    graph_.add_function(function_name, file, line, is_template, is_virtual);
}

void CallGraphBuilder::exit_function() {
    current_function_.clear();
}

void CallGraphBuilder::record_call(const std::string& callee) {
    if (!current_function_.empty()) {
        graph_.add_call(current_function_, callee);
    }
}

} // namespace analysis
} // namespace optiweave
