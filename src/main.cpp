#include <optiweave/core/ast_visitor.hpp>
#include <optiweave/core/rewriter.hpp>
#include <optiweave/analysis/complexity_analyzer.hpp>
#include <optiweave/analysis/call_graph.hpp>
#include <optiweave/analysis/dependency_graph.hpp>
#include <optiweave/analysis/data_flow_analysis.hpp>
#include <optiweave/analysis/memory_profiler.hpp>
#include <optiweave/analysis/overflow_detector.hpp>
#include <optiweave/analysis/fp_precision_detector.hpp>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Tooling/ArgumentsAdjusters.h>
#include <clang/Basic/Version.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

// Command line options
static cl::OptionCategory OptiWeaveCategory("OptiWeave Options");

static cl::opt<bool> TransformArraySubscripts(
    "array-subscripts",
    cl::desc("Transform array subscript expressions (default: true)"),
    cl::init(true), cl::cat(OptiWeaveCategory));

static cl::opt<bool> TransformArithmetic(
    "arithmetic-ops",
    cl::desc("Transform arithmetic operators (+, -, *, /, %)"), cl::init(false),
    cl::cat(OptiWeaveCategory));

static cl::opt<bool> TransformAssignment(
    "assignment-ops",
    cl::desc("Transform assignment operators (=, +=, -=, etc.)"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<bool> TransformComparison(
    "comparison-ops",
    cl::desc("Transform comparison operators (<, >, ==, !=, etc.)"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string>
    PreludePath("prelude",
                cl::desc("Path to custom prelude header (default: built-in)"),
                cl::value_desc("path"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> OutputDir(
    "output-dir",
    cl::desc("Output directory for transformed files (default: overwrite)"),
    cl::value_desc("directory"), cl::cat(OptiWeaveCategory));

static cl::opt<bool> SkipSystemHeaders(
    "skip-system-headers",
    cl::desc("Skip transformations in system headers (default: true)"),
    cl::init(true), cl::cat(OptiWeaveCategory));

static cl::opt<bool> Verbose("verbose", cl::desc("Enable verbose output"),
                             cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<bool> PrintStats("print-stats",
                                cl::desc("Print transformation statistics"),
                                cl::init(true), cl::cat(OptiWeaveCategory));

static cl::opt<bool>
    DryRun("dry-run", cl::desc("Parse and analyze without writing changes"),
           cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<bool> CompileAfterTransform(
    "compile", cl::desc("Automatically compile transformed code"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> OutputExecutable(
    "o", cl::desc("Output executable name (requires --compile)"),
    cl::value_desc("filename"), cl::cat(OptiWeaveCategory));

static cl::opt<bool> PrintCompileCmd(
    "print-compile-cmd",
    cl::desc("Print the compile command for instrumented code without running it"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<bool> GenerateCompileCommands(
    "generate-compile-commands",
    cl::desc("Generate compile_commands.json for proper header resolution"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<bool> EvaluationSafe(
    "evaluation-safe",
    cl::desc("Use wrappers to ensure single-evaluation of operands (default: ON)"),
    cl::init(true), cl::cat(OptiWeaveCategory));

// Statistics options
static cl::opt<bool> EnableStats(
    "enable-stats",
    cl::desc("Enable operation statistics collection at runtime"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> StatsCSV(
    "export-stats-csv",
    cl::desc("Export statistics to CSV file"),
    cl::value_desc("filename"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> StatsJSON(
    "export-stats-json",
    cl::desc("Export statistics to JSON file"),
    cl::value_desc("filename"), cl::cat(OptiWeaveCategory));

// Timing options
static cl::opt<bool> EnableTiming(
    "enable-timing",
    cl::desc("Enable high-resolution timing of operations at runtime"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<bool> EnableProfile(
    "enable-profile",
    cl::desc("Enable full profiling with percentiles and histograms"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> TimingCSV(
    "export-timing-csv",
    cl::desc("Export timing data to CSV file"),
    cl::value_desc("filename"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> TimingJSON(
    "export-timing-json",
    cl::desc("Export timing data to JSON file"),
    cl::value_desc("filename"), cl::cat(OptiWeaveCategory));

// Hotspot options
static cl::opt<bool> EnableHotspots(
    "hotspots",
    cl::desc("Enable hotspot detection and source location tracking"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<unsigned> HotspotsTopN(
    "top",
    cl::desc("Show top N hotspots (default: 10)"),
    cl::init(10), cl::value_desc("N"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> HotspotsCSV(
    "export-hotspots-csv",
    cl::desc("Export hotspot data to CSV file"),
    cl::value_desc("filename"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> HotspotsJSON(
    "export-hotspots-json",
    cl::desc("Export hotspot data to JSON file"),
    cl::value_desc("filename"), cl::cat(OptiWeaveCategory));

// Complexity analysis options
static cl::opt<bool> AnalyzeComplexity(
    "analyze-complexity",
    cl::desc("Perform static complexity analysis (cyclomatic, cognitive, maintainability)"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> ComplexityFormat(
    "complexity-format",
    cl::desc("Output format for complexity analysis (terminal, json, markdown, dot)"),
    cl::value_desc("format"), cl::init("terminal"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> ComplexityOutput(
    "complexity-output",
    cl::desc("Output file for complexity analysis"),
    cl::value_desc("filename"), cl::cat(OptiWeaveCategory));

// Call graph options
static cl::opt<bool> EnableCallGraph(
    "call-graph",
    cl::desc("Generate call graph showing function dependencies"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> CallGraphOutput(
    "call-graph-output",
    cl::desc("Output file for call graph (default: callgraph.dot)"),
    cl::value_desc("filename"), cl::init("callgraph.dot"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> CallGraphFormat(
    "call-graph-format",
    cl::desc("Call graph format: dot, json, html (default: dot)"),
    cl::value_desc("format"), cl::init("dot"), cl::cat(OptiWeaveCategory));

// Dependency graph options
static cl::opt<bool> EnableDependencyGraph(
    "dependency-graph",
    cl::desc("Generate dependency graph showing file inclusion relationships"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> DependencyGraphOutput(
    "dependency-graph-output",
    cl::desc("Output file for dependency graph (default: dependencies.dot)"),
    cl::value_desc("filename"), cl::init("dependencies.dot"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> DependencyGraphFormat(
    "dependency-graph-format",
    cl::desc("Dependency graph format: dot, json (default: dot)"),
    cl::value_desc("format"), cl::init("dot"), cl::cat(OptiWeaveCategory));

static cl::opt<bool> IncludeSystemHeaders(
    "include-system-headers",
    cl::desc("Include system headers in dependency graph (default: false)"),
    cl::init(false), cl::cat(OptiWeaveCategory));

// Data flow analysis options
static cl::opt<bool> EnableDataFlowAnalysis(
    "data-flow-analysis",
    cl::desc("Perform data flow analysis (detect unused variables, uninitialized usage, dead code)"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> DataFlowOutput(
    "data-flow-output",
    cl::desc("Output file for data flow analysis (default: dataflow.txt)"),
    cl::value_desc("filename"), cl::init("dataflow.txt"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> DataFlowFormat(
    "data-flow-format",
    cl::desc("Data flow analysis format: text, json (default: text)"),
    cl::value_desc("format"), cl::init("text"), cl::cat(OptiWeaveCategory));

// Memory profiling options
static cl::opt<bool> EnableMemoryProfiling(
    "memory-profile",
    cl::desc("Perform memory profiling (track allocations, deallocations, detect leaks)"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> MemoryProfileOutput(
    "memory-profile-output",
    cl::desc("Output file for memory profiling (default: memory.txt)"),
    cl::value_desc("filename"), cl::init("memory.txt"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> MemoryProfileFormat(
    "memory-profile-format",
    cl::desc("Memory profiling format: text, json (default: text)"),
    cl::value_desc("format"), cl::init("text"), cl::cat(OptiWeaveCategory));

// Cache profiling options
static cl::opt<bool> EnableCacheProfiling(
    "cache-profile",
    cl::desc("Enable hardware cache profiling (L1/L2/L3 misses, requires Linux)"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> CacheProfileCSV(
    "export-cache-csv",
    cl::desc("Export cache profiling data to CSV file"),
    cl::value_desc("filename"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> CacheProfileJSON(
    "export-cache-json",
    cl::desc("Export cache profiling data to JSON file"),
    cl::value_desc("filename"), cl::cat(OptiWeaveCategory));

// Integer overflow detection options
static cl::opt<bool> EnableOverflowDetection(
    "detect-overflow",
    cl::desc("Detect potential integer overflow issues"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> OverflowOutput(
    "overflow-output",
    cl::desc("Output file for overflow detection (default: overflow.txt)"),
    cl::value_desc("filename"), cl::init("overflow.txt"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> OverflowFormat(
    "overflow-format",
    cl::desc("Overflow detection format: text, json (default: text)"),
    cl::value_desc("format"), cl::init("text"), cl::cat(OptiWeaveCategory));

// Floating-point precision warning options
static cl::opt<bool> EnableFPPrecisionWarnings(
    "fp-precision-warnings",
    cl::desc("Detect potential floating-point precision issues"),
    cl::init(false), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> FPPrecisionOutput(
    "fp-precision-output",
    cl::desc("Output file for FP precision warnings (default: fp_precision.txt)"),
    cl::value_desc("filename"), cl::init("fp_precision.txt"), cl::cat(OptiWeaveCategory));

static cl::opt<std::string> FPPrecisionFormat(
    "fp-precision-format",
    cl::desc("FP precision warning format: text, json (default: text)"),
    cl::value_desc("format"), cl::init("text"), cl::cat(OptiWeaveCategory));

namespace optiweave {

/**
 * @brief Frontend action for OptiWeave transformations
 */
class OptiWeaveFrontendAction : public ASTFrontendAction {
public:
  explicit OptiWeaveFrontendAction(const core::TransformationConfig &config,
                                   analysis::CallGraph* shared_call_graph = nullptr,
                                   analysis::DependencyGraph* shared_dependency_graph = nullptr,
                                   analysis::DataFlowAnalysis* shared_data_flow = nullptr,
                                   analysis::MemoryProfiler* shared_memory_profiler = nullptr)
      : config_(config), shared_call_graph_(shared_call_graph),
        shared_dependency_graph_(shared_dependency_graph),
        shared_data_flow_(shared_data_flow),
        shared_memory_profiler_(shared_memory_profiler), consumer_(nullptr) {}

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    if (Verbose) {
      llvm::errs() << "Processing file: " << file << "\n";
    }

    // Initialize rewriter
    rewriter_.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());

    // Register preprocessor callback for dependency tracking if enabled
    if (config_.enable_dependency_graph && shared_dependency_graph_) {
      CI.getPreprocessor().addPPCallbacks(
          std::make_unique<core::DependencyTrackerPPCallbacks>(
              shared_dependency_graph_, CI.getSourceManager()));
    }

    // Create consumer with configuration
    auto consumer = std::make_unique<core::TransformationConsumer>(
        rewriter_, CI.getASTContext(), config_);
    consumer_ = consumer.get();
    return consumer;
  }

  void EndSourceFileAction() override {
    // If call graph tracking is enabled, merge this file's graph into shared graph
    if (config_.enable_call_graph && consumer_ && shared_call_graph_) {
      auto* builder = consumer_->getCallGraphBuilder();
      if (builder) {
        // Get the call graph from this translation unit
        auto& local_graph = builder->get_graph();

        // Merge into shared graph (copy all nodes and edges)
        for (const auto& func_name : local_graph.get_all_functions()) {
          const auto* node = local_graph.get_function(func_name);
          if (node) {
            shared_call_graph_->add_function(node->function_name, node->file,
                                             node->line, node->is_template, node->is_virtual);
            for (const auto& callee : node->callees) {
              shared_call_graph_->add_call(func_name, callee);
            }
          }
        }
      }
    }

    // If data flow analysis is enabled, merge this file's analysis into shared analysis
    if (config_.enable_data_flow_analysis && consumer_ && shared_data_flow_) {
      auto* local_data_flow = consumer_->getDataFlowAnalysis();
      if (local_data_flow) {
        // Merge the local analysis into the shared analysis
        shared_data_flow_->merge(*local_data_flow);
      }
    }

    // If memory profiling is enabled, merge this file's analysis into shared profiler
    if (config_.enable_memory_profiling && consumer_ && shared_memory_profiler_) {
      auto* local_memory_profiler = consumer_->getMemoryProfiler();
      if (local_memory_profiler) {
        // Merge the local profiler into the shared profiler
        shared_memory_profiler_->merge(*local_memory_profiler);
      }
    }

    auto &source_manager = rewriter_.getSourceMgr();

    if (DryRun) {
      if (Verbose) {
        llvm::errs() << "Dry run - no files written\n";
      }
      return;
    }

    // Write transformed files
    if (OutputDir.empty()) {
      // Overwrite original files
      rewriter_.overwriteChangedFiles();
    } else {
      // Write to output directory
      for (auto i = rewriter_.buffer_begin(), e = rewriter_.buffer_end();
           i != e; ++i) {
        FileID file_id = i->first;
        const RewriteBuffer &buffer = i->second;

        auto file_entry = source_manager.getFileEntryRefForID(file_id);
        if (!file_entry)
          continue;

        auto original_path = file_entry->getName();
        auto filename = llvm::sys::path::filename(original_path);

        SmallString<128> output_path;
        llvm::sys::path::append(output_path, OutputDir, filename);

        std::error_code EC;
        raw_fd_ostream output(output_path, EC);
        if (EC) {
          llvm::errs() << "Error writing to " << output_path << ": "
                       << EC.message() << "\n";
          continue;
        }

        buffer.write(output);

        if (Verbose) {
          llvm::errs() << "Wrote transformed file: " << output_path << "\n";
        }
      }
    }
  }

private:
  Rewriter rewriter_;
  core::TransformationConfig config_;
  analysis::CallGraph* shared_call_graph_;
  analysis::DependencyGraph* shared_dependency_graph_;
  analysis::DataFlowAnalysis* shared_data_flow_;
  analysis::MemoryProfiler* shared_memory_profiler_;
  core::TransformationConsumer* consumer_;
};

/**
 * @brief Factory for creating OptiWeave frontend actions
 */
class OptiWeaveFrontendActionFactory : public FrontendActionFactory {
public:
  explicit OptiWeaveFrontendActionFactory(
      const core::TransformationConfig &config)
      : config_(config), call_graph_(), dependency_graph_(), data_flow_analysis_(), memory_profiler_() {}

  std::unique_ptr<FrontendAction> create() override {
    return std::make_unique<OptiWeaveFrontendAction>(config_, &call_graph_, &dependency_graph_, &data_flow_analysis_, &memory_profiler_);
  }

  analysis::CallGraph& getCallGraph() { return call_graph_; }
  analysis::DependencyGraph& getDependencyGraph() { return dependency_graph_; }
  analysis::DataFlowAnalysis& getDataFlowAnalysis() { return data_flow_analysis_; }
  analysis::MemoryProfiler& getMemoryProfiler() { return memory_profiler_; }

private:
  core::TransformationConfig config_;
  analysis::CallGraph call_graph_;
  analysis::DependencyGraph dependency_graph_;
  analysis::DataFlowAnalysis data_flow_analysis_;
  analysis::MemoryProfiler memory_profiler_;
};

/**
 * @brief Setup include paths for prelude
 */
std::string setupPrelude() {
  if (!PreludePath.empty()) {
    if (llvm::sys::fs::exists(PreludePath)) {
      return PreludePath;
    } else {
      llvm::errs() << "Warning: Prelude file not found: " << PreludePath
                   << "\n";
    }
  }

  // Try to find built-in prelude relative to executable
  SmallString<128> exe_path;
  if (auto exe = llvm::sys::fs::getMainExecutable(nullptr, nullptr);
      !exe.empty()) {
    exe_path = exe;
    llvm::sys::path::remove_filename(exe_path);
    llvm::sys::path::append(exe_path, "..", "templates", "prelude.hpp");

    if (llvm::sys::fs::exists(exe_path)) {
      return exe_path.str().str();
    }
  }

  // Try current directory
  if (llvm::sys::fs::exists("templates/prelude.hpp")) {
    return "templates/prelude.hpp";
  }

  // Built-in fallback
  llvm::errs() << "Warning: Using built-in prelude (no external file found)\n";
  return "";
}

/**
 * @brief Validate and create output directory if needed
 */
bool validateOutputDirectory() {
  if (OutputDir.empty()) {
    return true; // Overwrite mode
  }

  std::error_code EC = llvm::sys::fs::create_directories(OutputDir);
  if (EC) {
    llvm::errs() << "Error creating output directory: " << EC.message() << "\n";
    return false;
  }

  // Check if directory is writable
  if (!llvm::sys::fs::can_write(OutputDir)) {
    llvm::errs() << "Error: Output directory is not writable: " << OutputDir
                 << "\n";
    return false;
  }

  return true;
}

/**
 * @brief Print version information
 */
void printVersion() {
  llvm::outs()
      << "OptiWeave v1.0.0 - Modern C++ Operator Instrumentation Tool\n";
  llvm::outs() << "Built with Clang " << clang::getClangFullVersion() << "\n";
  llvm::outs() << "Copyright (c) 2024 OptiWeave Contributors\n";
}

/**
 * @brief Print usage examples
 */
void printUsage() {
  llvm::outs() << R"(
Usage Examples:
  # Transform array subscripts only (default)
  optiweave source.cpp -- -std=c++20

  # Transform and compile in one step
  optiweave source.cpp --compile -o instrumented -- -std=c++20

  # Show the compile command without running it
  optiweave source.cpp --print-compile-cmd -- -std=c++20

  # Transform multiple operator types and compile
  optiweave --arithmetic-ops --assignment-ops source.cpp --compile -o instrumented -- -std=c++20

  # Use custom prelude and output directory
  optiweave --prelude=my_prelude.hpp --output-dir=./transformed source.cpp --

  # Dry run to check what would be transformed
  optiweave --dry-run --stats --verbose source.cpp --

  # Transform entire project with compilation database
  optiweave --arithmetic-ops $(find src -name "*.cpp") --

  # Generate call graph in DOT format (visualize with GraphViz)
  optiweave source.cpp --call-graph --call-graph-output=callgraph.dot --
  dot -Tpng callgraph.dot -o callgraph.png

  # Generate call graph in JSON format
  optiweave source.cpp --call-graph --call-graph-format=json --call-graph-output=callgraph.json --

  # Generate interactive HTML call graph visualization
  optiweave source.cpp --call-graph --call-graph-format=html --call-graph-output=callgraph.html --

  # Generate dependency graph in DOT format (visualize with GraphViz)
  optiweave source.cpp --dependency-graph --dependency-graph-output=dependencies.dot --
  dot -Tpng dependencies.dot -o dependencies.png

  # Generate dependency graph in JSON format
  optiweave source.cpp --dependency-graph --dependency-graph-format=json --dependency-graph-output=dependencies.json --

  # Include system headers in dependency graph
  optiweave source.cpp --dependency-graph --include-system-headers --verbose --

  # Combine transformation with call graph and complexity analysis
  optiweave source.cpp --call-graph --analyze-complexity --verbose --

  # Generate both call graph and dependency graph
  optiweave source.cpp --call-graph --dependency-graph --verbose --

  # Perform data flow analysis (detect unused variables, uninitialized usage)
  optiweave source.cpp --data-flow-analysis --data-flow-output=dataflow.txt --

  # Data flow analysis with JSON output
  optiweave source.cpp --data-flow-analysis --data-flow-format=json --data-flow-output=dataflow.json --

  # Perform memory profiling (track allocations, deallocations, detect leaks)
  optiweave source.cpp --memory-profile --memory-profile-output=memory.txt --

  # Memory profiling with JSON output
  optiweave source.cpp --memory-profile --memory-profile-format=json --memory-profile-output=memory.json --

For more information, see: https://github.com/optiweave/optiweave
)";
}

/**
 * @brief Generate compile_commands.json for proper header resolution
 */
bool generateCompileCommands(const std::vector<std::string> &source_paths) {
  if (source_paths.empty()) {
    llvm::errs() << "Error: No source files to generate compile commands for\n";
    return false;
  }

  // Auto-detect SDK path
  std::string sdk_path;
  FILE* xcrun_cmd = popen("xcrun --show-sdk-path 2>/dev/null", "r");
  if (xcrun_cmd) {
    char path_buffer[512];
    if (fgets(path_buffer, sizeof(path_buffer), xcrun_cmd)) {
      sdk_path = std::string(path_buffer);
      if (!sdk_path.empty() && sdk_path.back() == '\n') {
        sdk_path.pop_back();
      }
    }
    pclose(xcrun_cmd);
  }
  
  if (sdk_path.empty()) {
    sdk_path = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk";
  }

  // Get templates directory
  SmallString<128> templates_dir;
  if (auto exe = llvm::sys::fs::getMainExecutable(nullptr, nullptr); !exe.empty()) {
    templates_dir = exe;
    llvm::sys::path::remove_filename(templates_dir);
    llvm::sys::path::append(templates_dir, "..", "templates");
  } else {
    templates_dir = "templates";
  }

  // Create compile_commands.json
  std::error_code EC;
  raw_fd_ostream compile_commands("compile_commands.json", EC);
  if (EC) {
    llvm::errs() << "Error creating compile_commands.json: " << EC.message() << "\n";
    return false;
  }

  compile_commands << "[\n";
  
  for (size_t i = 0; i < source_paths.size(); ++i) {
    SmallString<256> absolute_path;
    auto ec = llvm::sys::fs::real_path(source_paths[i], absolute_path);
    if (ec) {
      llvm::errs() << "Warning: Could not resolve path for " << source_paths[i] << ": " << ec.message() << "\n";
      continue;
    }

    // Get current working directory
    SmallString<128> cwd;
    llvm::sys::fs::current_path(cwd);

    compile_commands << "  {\n";
    compile_commands << "    \"directory\": \"" << cwd.str() << "\",\n";
    compile_commands << "    \"command\": \"clang++ -std=c++20";
    
    // Add SDK if it exists
    if (llvm::sys::fs::exists(sdk_path)) {
      compile_commands << " -isysroot " << sdk_path;
    }
    
    // Add templates include
    if (llvm::sys::fs::exists(templates_dir)) {
      compile_commands << " -I" << templates_dir.str();
    }
    
    compile_commands << " " << absolute_path.str() << "\",\n";
    compile_commands << "    \"file\": \"" << absolute_path.str() << "\"\n";
    compile_commands << "  }";
    
    if (i < source_paths.size() - 1) {
      compile_commands << ",";
    }
    compile_commands << "\n";
  }
  
  compile_commands << "]\n";
  compile_commands.close();

  if (Verbose) {
    llvm::errs() << "Generated compile_commands.json with " << source_paths.size() 
                 << " entries\n";
    llvm::errs() << "Using SDK: " << sdk_path << "\n";
    llvm::errs() << "Using templates: " << templates_dir << "\n";
  }

  return true;
}

/**
 * @brief Build the compile command for instrumented source files.
 * @return The command as a vector of arguments, or empty on error.
 */
std::vector<std::string> buildCompileCommand(const std::vector<std::string> &source_paths) {
  std::vector<std::string> compile_cmd;

  if (source_paths.empty()) {
    llvm::errs() << "Error: No source files to compile\n";
    return {};
  }

  // Determine output executable name
  std::string output_name;
  if (!OutputExecutable.empty()) {
    output_name = OutputExecutable;
  } else {
    auto base_name = llvm::sys::path::stem(source_paths[0]);
    output_name = base_name.str() + "_instrumented";
  }

  compile_cmd.push_back("clang++");
  compile_cmd.push_back("-std=c++20");

  // Auto-detect SDK path (macOS)
  std::string sdk_path;
  FILE* xcrun_cmd = popen("xcrun --show-sdk-path 2>/dev/null", "r");
  if (xcrun_cmd) {
    char path_buffer[512];
    if (fgets(path_buffer, sizeof(path_buffer), xcrun_cmd)) {
      sdk_path = std::string(path_buffer);
      if (!sdk_path.empty() && sdk_path.back() == '\n') {
        sdk_path.pop_back();
      }
    }
    pclose(xcrun_cmd);
  }
  if (sdk_path.empty()) {
    sdk_path = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk";
  }
  if (llvm::sys::fs::exists(sdk_path)) {
    compile_cmd.push_back("-isysroot");
    compile_cmd.push_back(sdk_path);
  }

  // Templates include path
  SmallString<128> templates_dir;
  if (auto exe = llvm::sys::fs::getMainExecutable(nullptr, nullptr); !exe.empty()) {
    templates_dir = exe;
    llvm::sys::path::remove_filename(templates_dir);
    llvm::sys::path::append(templates_dir, "..", "templates");
  } else {
    templates_dir = "templates";
  }
  if (llvm::sys::fs::exists(templates_dir)) {
    compile_cmd.push_back("-I" + templates_dir.str().str());
  } else {
    llvm::errs() << "Error: Templates directory not found: " << templates_dir << "\n";
    llvm::errs() << "Please ensure OptiWeave is properly installed.\n";
    return {};
  }

  // Runtime library path
  SmallString<128> lib_dir;
  if (auto exe = llvm::sys::fs::getMainExecutable(nullptr, nullptr); !exe.empty()) {
    lib_dir = exe;
    llvm::sys::path::remove_filename(lib_dir);
  } else {
    lib_dir = "build";
  }
  SmallString<128> runtime_lib_path(lib_dir);
  llvm::sys::path::append(runtime_lib_path, "liboptiweave_runtime.a");
  if (llvm::sys::fs::exists(runtime_lib_path)) {
    compile_cmd.push_back("-L" + lib_dir.str().str());
    compile_cmd.push_back("-loptiweave_runtime");
  } else {
    llvm::errs() << "Error: Runtime library not found: " << runtime_lib_path << "\n";
    llvm::errs() << "Please rebuild OptiWeave with: ./scripts/build.sh\n";
    return {};
  }

  // Feature flags
  if (EnableStats || !StatsCSV.empty() || !StatsJSON.empty()) {
    compile_cmd.push_back("-DOPTIWEAVE_ENABLE_STATS");
  }
  if (EnableTiming || EnableProfile || !TimingCSV.empty() || !TimingJSON.empty()) {
    compile_cmd.push_back("-DOPTIWEAVE_ENABLE_TIMING");
  }
  if (EnableHotspots || !HotspotsCSV.empty() || !HotspotsJSON.empty()) {
    compile_cmd.push_back("-DOPTIWEAVE_ENABLE_HOTSPOTS");
    compile_cmd.push_back("-DOPTIWEAVE_ENABLE_TIMING");
  }
  if (EnableCacheProfiling || !CacheProfileCSV.empty() || !CacheProfileJSON.empty()) {
    compile_cmd.push_back("-DOPTIWEAVE_ENABLE_CACHE_PROFILE");
  }

  // Runtime include path
  SmallString<128> runtime_include_dir;
  if (auto exe = llvm::sys::fs::getMainExecutable(nullptr, nullptr); !exe.empty()) {
    runtime_include_dir = exe;
    llvm::sys::path::remove_filename(runtime_include_dir);
    llvm::sys::path::append(runtime_include_dir, "..", "include");
    if (llvm::sys::fs::exists(runtime_include_dir)) {
      compile_cmd.push_back("-I" + runtime_include_dir.str().str());
    }
  }

  // Source files
  for (const auto &source : source_paths) {
    if (OutputDir.empty()) {
      compile_cmd.push_back(source);
    } else {
      auto filename = llvm::sys::path::filename(source);
      SmallString<128> transformed_path;
      llvm::sys::path::append(transformed_path, OutputDir, filename);
      compile_cmd.push_back(transformed_path.str().str());
    }
  }

  compile_cmd.push_back("-o");
  compile_cmd.push_back(output_name);

  return compile_cmd;
}

/**
 * @brief Format a compile command vector as a shell-safe string
 */
std::string formatCompileCommand(const std::vector<std::string> &compile_cmd) {
  std::string result;
  for (size_t i = 0; i < compile_cmd.size(); ++i) {
    if (i > 0) result += " ";
    // Quote arguments that contain spaces
    if (compile_cmd[i].find(' ') != std::string::npos) {
      result += "\"" + compile_cmd[i] + "\"";
    } else {
      result += compile_cmd[i];
    }
  }
  return result;
}

/**
 * @brief Compile transformed source files
 */
bool compileTransformedFiles(const std::vector<std::string> &source_paths) {
  auto compile_cmd = buildCompileCommand(source_paths);
  if (compile_cmd.empty()) {
    return false;
  }

  std::string full_cmd = formatCompileCommand(compile_cmd);

  if (Verbose) {
    llvm::errs() << "Compiling: " << full_cmd << "\n";
  }

  int compile_result = std::system(full_cmd.c_str());

  if (compile_result == 0) {
    if (Verbose) {
      llvm::errs() << "Compilation successful\n";
    }
    // Find the output name (last argument)
    llvm::outs() << "Instrumented executable created: " << compile_cmd.back() << "\n";
    return true;
  } else {
    llvm::errs() << "Compilation failed with exit code: " << compile_result << "\n";
    return false;
  }
}

} // namespace optiweave

int main(int argc, const char **argv) {
  // Early detection of C vs C++ based on -x flag (before OptionsParser)
  bool user_specified_c = false;
  for (int i = 1; i < argc - 1; ++i) {
    if (std::string(argv[i]) == "-x" && std::string(argv[i + 1]) == "c") {
      user_specified_c = true;
      break;
    }
  }

  // Parse command line arguments
  auto ExpectedParser =
      CommonOptionsParser::create(argc, argv, OptiWeaveCategory);
  if (!ExpectedParser) {
    llvm::errs() << "Error parsing command line: " << ExpectedParser.takeError()
                 << "\n";
    return 1;
  }

  CommonOptionsParser &OptionsParser = ExpectedParser.get();

  // Print version if requested
  if (argc == 2 &&
      (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-V")) {
    optiweave::printVersion();
    return 0;
  }

  // Print usage if requested
  if (argc == 2 &&
      (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
    optiweave::printUsage();
    return 0;
  }

  // Validate output directory
  if (!optiweave::validateOutputDirectory()) {
    return 1;
  }

  // Validate compile options
  if (CompileAfterTransform && DryRun) {
    llvm::errs() << "Error: --compile cannot be used with --dry-run\n";
    return 1;
  }
  if (PrintCompileCmd && DryRun) {
    llvm::errs() << "Error: --print-compile-cmd cannot be used with --dry-run\n";
    return 1;
  }

  auto source_paths = OptionsParser.getSourcePathList();

  // Detect if we're processing C or C++ files
  bool is_c_file = user_specified_c; // Use early detection from before OptionsParser
  if (!source_paths.empty()) {
    llvm::StringRef first_file(source_paths[0]);
    // Also check file extension
    if (first_file.ends_with(".c")) {
      is_c_file = true;
    }

    if (Verbose) {
      llvm::errs() << "Detected file type: " << (is_c_file ? "C" : "C++") << "\n";
    }
  }

  // Generate compilation database if requested or if none exists and we need one
  bool should_generate_compile_commands = GenerateCompileCommands;
  
  if (!should_generate_compile_commands) {
    // Auto-generate if no compile_commands.json exists and we're about to transform
    if (!llvm::sys::fs::exists("compile_commands.json")) {
      should_generate_compile_commands = true;
      if (Verbose) {
        llvm::errs() << "No compile_commands.json found, generating automatically...\n";
      }
    }
  }

  if (should_generate_compile_commands) {
    if (!optiweave::generateCompileCommands(source_paths)) {
      if (GenerateCompileCommands) {
        // If explicitly requested, this is an error
        return 1;
      } else {
        // If auto-generated, just warn and continue
        llvm::errs() << "Warning: Could not generate compile_commands.json, continuing anyway...\n";
      }
    }
  }

  // Setup prelude
  std::string prelude_path = optiweave::setupPrelude();

  // Set loop info file path for optimization suggestions
  // When compiling with -o, place loop info next to the executable
  if (CompileAfterTransform && !OutputExecutable.empty()) {
    llvm::SmallString<256> loop_info_path(OutputExecutable);
    llvm::sys::path::remove_filename(loop_info_path);
    if (loop_info_path.empty()) {
      loop_info_path = ".";
    }
    llvm::sys::path::append(loop_info_path, ".optiweave_loop_info.bin");
    setenv("OPTIWEAVE_LOOP_INFO_FILE", loop_info_path.c_str(), 1);
    if (Verbose) {
      llvm::errs() << "Loop info will be saved to: " << loop_info_path << "\n";
    }
  }

  // Configure transformation
  optiweave::core::TransformationConfig config;
  config.transform_array_subscripts = TransformArraySubscripts;
  config.transform_arithmetic_operators = TransformArithmetic;
  config.transform_assignment_operators = TransformAssignment;
  config.transform_comparisons_operators = TransformComparison;
  config.evaluation_safe_wrappers = EvaluationSafe;
  config.skip_system_headers = SkipSystemHeaders;
  config.enable_call_graph = EnableCallGraph;
  config.enable_dependency_graph = EnableDependencyGraph;
  config.enable_data_flow_analysis = EnableDataFlowAnalysis;
  config.enable_memory_profiling = EnableMemoryProfiling;
  config.is_c_language = is_c_file; // Set based on detected language
  config.prelude_path = prelude_path;

  if (Verbose) {
    llvm::errs() << "OptiWeave Configuration:\n";
    llvm::errs() << "  Array subscripts: "
                 << (config.transform_array_subscripts ? "ON" : "OFF") << "\n";
    llvm::errs() << "  Arithmetic ops: "
                 << (config.transform_arithmetic_operators ? "ON" : "OFF")
                 << "\n";
    llvm::errs() << "  Assignment ops: "
                 << (config.transform_assignment_operators ? "ON" : "OFF")
                 << "\n";
    llvm::errs() << "  Comparison ops: "
                 << (config.transform_comparisons_operators ? "ON" : "OFF")
                 << "\n";
    llvm::errs() << "  Skip system headers: "
                 << (config.skip_system_headers ? "ON" : "OFF") << "\n";
    llvm::errs() << "  Prelude path: "
                 << (prelude_path.empty() ? "built-in" : prelude_path) << "\n";
    llvm::errs() << "  Output directory: "
                 << (OutputDir.empty() ? "overwrite" : OutputDir.getValue())
                 << "\n";
    llvm::errs() << "  Dry run: " << (DryRun ? "ON" : "OFF") << "\n";
  }

  // Create ClangTool
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  // The compilation database will be automatically picked up by the ClangTool
  // Focus on the key fix: continue compilation even with parse warnings

  // Add templates directory to include path for optiweave/prelude.hpp
  SmallString<128> templates_dir;
  if (auto exe = llvm::sys::fs::getMainExecutable(nullptr, nullptr); !exe.empty()) {
    templates_dir = exe;
    llvm::sys::path::remove_filename(templates_dir);
    llvm::sys::path::append(templates_dir, "..", "templates");
  } else {
    templates_dir = "templates";
  }
  
  if (llvm::sys::fs::exists(templates_dir)) {
    std::string include_arg = "-I" + templates_dir.str().str();
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(include_arg.c_str(), clang::tooling::ArgumentInsertPosition::BEGIN));
    if (Verbose) {
      llvm::errs() << "Added include path: " << templates_dir << "\n";
    }
  }

  // Add appropriate language standard if not specified
  const char* lang_std = is_c_file ? "-std=c11" : "-std=c++20";
  Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(lang_std, clang::tooling::ArgumentInsertPosition::BEGIN));

  // Add Clang resource directory for compiler intrinsics (stdarg.h, etc.)
  std::string resource_dir;
  FILE* clang_resource_cmd = popen("clang++ -print-resource-dir 2>/dev/null", "r");
  if (clang_resource_cmd) {
    char path_buffer[512];
    if (fgets(path_buffer, sizeof(path_buffer), clang_resource_cmd)) {
      resource_dir = std::string(path_buffer);
      if (!resource_dir.empty() && resource_dir.back() == '\n') {
        resource_dir.pop_back();
      }
    }
    pclose(clang_resource_cmd);
  }

  if (!resource_dir.empty() && llvm::sys::fs::exists(resource_dir)) {
    std::string resource_arg = "-resource-dir=" + resource_dir;
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(resource_arg.c_str(), clang::tooling::ArgumentInsertPosition::BEGIN));
    if (Verbose) {
      llvm::errs() << "Using Clang resource directory: " << resource_dir << "\n";
    }
  }

  // Create factory and run tool
  optiweave::OptiWeaveFrontendActionFactory factory(config);
  int result = Tool.run(&factory);

  if (result == 0) {
    if (Verbose) {
      llvm::errs() << "Transformation completed successfully\n";
    }
  } else {
    // Check if transformation actually succeeded despite parse errors
    // Parse errors often occur due to header resolution but transformation can still work
    if (Verbose) {
      llvm::errs() << "Parse errors encountered (code: " << result << "), but transformation may have succeeded\n";
    }
  }

  // Perform complexity analysis if requested
  if (AnalyzeComplexity) {
    if (Verbose) {
      llvm::errs() << "Performing complexity analysis...\n";
    }

    // Create a new tool for complexity analysis
    ClangTool ComplexityTool(OptionsParser.getCompilations(),
                             OptionsParser.getSourcePathList());

    // Add templates directory to include path (same as transformation tool)
    if (llvm::sys::fs::exists(templates_dir)) {
      std::string include_arg = "-I" + templates_dir.str().str();
      ComplexityTool.appendArgumentsAdjuster(getInsertArgumentAdjuster(include_arg.c_str(), clang::tooling::ArgumentInsertPosition::BEGIN));
      if (Verbose) {
        llvm::errs() << "Added include path for complexity analysis: " << templates_dir << "\n";
      }
    }

    // Add C++20 standard
    ComplexityTool.appendArgumentsAdjuster(getInsertArgumentAdjuster(lang_std, clang::tooling::ArgumentInsertPosition::BEGIN));

    // Add Clang resource directory for compiler intrinsics (same as transformation tool)
    if (!resource_dir.empty() && llvm::sys::fs::exists(resource_dir)) {
      std::string resource_arg = "-resource-dir=" + resource_dir;
      ComplexityTool.appendArgumentsAdjuster(getInsertArgumentAdjuster(resource_arg.c_str(), clang::tooling::ArgumentInsertPosition::BEGIN));
      if (Verbose) {
        llvm::errs() << "Using Clang resource directory for complexity analysis: " << resource_dir << "\n";
      }
    }

    // Run complexity analysis
    optiweave::ComplexityAnalysisActionFactory complexity_factory;

    int analysis_result = ComplexityTool.run(&complexity_factory);

    if (Verbose) {
      llvm::errs() << "Complexity analysis return code: " << analysis_result << "\n";
    }

    if (analysis_result == 0) {
      const auto& result = complexity_factory.getResult();

      if (Verbose) {
        llvm::errs() << "Functions analyzed: " << result.functions.size() << "\n";
        llvm::errs() << "Exporting in format: " << ComplexityFormat << "\n";
      }

      // Export results based on format
      std::string output;
      if (ComplexityFormat == "json") {
        output = optiweave::analysis::export_utils::export_json(result);
      } else if (ComplexityFormat == "markdown") {
        output = optiweave::analysis::export_utils::export_markdown(result);
      } else if (ComplexityFormat == "dot") {
        output = optiweave::analysis::export_utils::export_call_graph_dot(result);
      } else {
        // Default: terminal
        output = optiweave::analysis::export_utils::export_terminal(result);
      }

      // Write to file or stdout
      if (!ComplexityOutput.empty()) {
        std::ofstream out(ComplexityOutput);
        if (out.is_open()) {
          out << output;
          out.close();
          llvm::errs() << "Complexity analysis exported to: " << ComplexityOutput << "\n";
        } else {
          llvm::errs() << "Error: Could not write to " << ComplexityOutput << "\n";
          llvm::errs() << output;  // Print to stderr as fallback
        }
      } else {
        // Print to stderr
        llvm::errs() << output;
      }
    } else {
      llvm::errs() << "Warning: Complexity analysis encountered errors\n";
    }
  }

  // Export call graph if requested
  if (EnableCallGraph) {
    if (Verbose) {
      llvm::errs() << "Exporting call graph...\n";
    }

    auto& call_graph = factory.getCallGraph();

    // Export based on format
    std::string format = CallGraphFormat;
    std::string output_file = CallGraphOutput;

    try {
      if (format == "json") {
        call_graph.export_json(output_file);
      } else if (format == "html") {
        call_graph.export_html(output_file);
      } else {
        // Default: DOT format
        call_graph.export_dot(output_file, false);
      }

      llvm::errs() << "Call graph exported to: " << output_file << "\n";

      if (Verbose) {
        call_graph.print_statistics(llvm::outs());
      }
    } catch (const std::exception& e) {
      llvm::errs() << "Error exporting call graph: " << e.what() << "\n";
    }
  }

  // Export dependency graph if requested
  if (EnableDependencyGraph) {
    if (Verbose) {
      llvm::errs() << "Exporting dependency graph...\n";
    }

    auto& dependency_graph = factory.getDependencyGraph();

    // Calculate metrics before export
    dependency_graph.calculate_include_depths();
    dependency_graph.calculate_metrics();

    // Export based on format
    std::string format = DependencyGraphFormat;
    std::string output_file = DependencyGraphOutput;
    bool include_system = IncludeSystemHeaders;

    try {
      if (format == "json") {
        dependency_graph.export_json(output_file, include_system);
      } else {
        // Default: DOT format
        dependency_graph.export_dot(output_file, include_system);
      }

      llvm::errs() << "Dependency graph exported to: " << output_file << "\n";

      if (Verbose) {
        dependency_graph.print_statistics(llvm::outs(), include_system);
      }
    } catch (const std::exception& e) {
      llvm::errs() << "Error exporting dependency graph: " << e.what() << "\n";
    }
  }

  // Export data flow analysis if requested
  if (EnableDataFlowAnalysis) {
    if (Verbose) {
      llvm::errs() << "Exporting data flow analysis...\n";
    }

    auto& data_flow = factory.getDataFlowAnalysis();

    // Run detection algorithms on the merged data
    data_flow.detect_uninitialized_variables();
    data_flow.detect_unused_variables();
    data_flow.detect_dead_code();
    data_flow.generate_refactoring_opportunities();

    // Export based on format
    std::string format = DataFlowFormat;
    std::string output_file = DataFlowOutput;

    // We need a source manager for export - create a minimal one
    // LLVM 21+ changed DiagnosticOptions to non-refcounted
#if LLVM_VERSION_MAJOR >= 21
    clang::DiagnosticOptions diag_opts;
    clang::TextDiagnosticPrinter *diag_printer = new clang::TextDiagnosticPrinter(llvm::errs(), diag_opts);
    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diag_id(new clang::DiagnosticIDs());
    clang::DiagnosticsEngine diags(diag_id, diag_opts, diag_printer);
#else
    llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> diag_opts(new clang::DiagnosticOptions());
    clang::TextDiagnosticPrinter *diag_printer = new clang::TextDiagnosticPrinter(llvm::errs(), diag_opts.get());
    llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diag_id(new clang::DiagnosticIDs());
    clang::DiagnosticsEngine diags(diag_id, diag_opts, diag_printer);
#endif

    clang::FileSystemOptions file_system_opts;
    clang::FileManager file_mgr(file_system_opts);
    clang::SourceManager source_mgr(diags, file_mgr);

    try {
      if (format == "json") {
        data_flow.export_json(output_file, source_mgr);
      } else {
        // Default: text format
        data_flow.export_text(output_file, source_mgr);
      }

      llvm::errs() << "Data flow analysis exported to: " << output_file << "\n";

      if (Verbose) {
        data_flow.print_statistics(llvm::outs(), source_mgr);
      }
    } catch (const std::exception& e) {
      llvm::errs() << "Error exporting data flow analysis: " << e.what() << "\n";
    }
  }

  // Export memory profiling if requested
  if (EnableMemoryProfiling) {
    if (Verbose) {
      llvm::errs() << "Exporting memory profiling...\n";
    }

    auto& memory_profiler = factory.getMemoryProfiler();

    // Run analysis on the merged data
    memory_profiler.analyze();

    // Export based on format
    std::string format = MemoryProfileFormat;
    std::string output_file = MemoryProfileOutput;

    try {
      if (format == "json") {
        memory_profiler.export_json(output_file);
      } else {
        // Default: text format
        memory_profiler.export_text(output_file);
      }

      llvm::errs() << "Memory profiling exported to: " << output_file << "\n";

      if (Verbose) {
        memory_profiler.print_statistics(llvm::outs());
      }
    } catch (const std::exception& e) {
      llvm::errs() << "Error exporting memory profiling: " << e.what() << "\n";
    }
  }

  // Perform overflow detection if requested
  if (EnableOverflowDetection) {
    if (Verbose) {
      llvm::errs() << "Performing integer overflow detection...\n";
    }

    // Create a new tool for overflow detection
    ClangTool OverflowTool(OptionsParser.getCompilations(),
                           OptionsParser.getSourcePathList());

    // Add templates directory to include path (same as transformation tool)
    if (llvm::sys::fs::exists(templates_dir)) {
      std::string include_arg = "-I" + templates_dir.str().str();
      OverflowTool.appendArgumentsAdjuster(getInsertArgumentAdjuster(include_arg.c_str(), clang::tooling::ArgumentInsertPosition::BEGIN));
      if (Verbose) {
        llvm::errs() << "Added include path for overflow detection: " << templates_dir << "\n";
      }
    }

    // Add C++20 standard
    OverflowTool.appendArgumentsAdjuster(getInsertArgumentAdjuster(lang_std, clang::tooling::ArgumentInsertPosition::BEGIN));

    // Add Clang resource directory
    if (!resource_dir.empty() && llvm::sys::fs::exists(resource_dir)) {
      std::string resource_arg = "-resource-dir=" + resource_dir;
      OverflowTool.appendArgumentsAdjuster(getInsertArgumentAdjuster(resource_arg.c_str(), clang::tooling::ArgumentInsertPosition::BEGIN));
      if (Verbose) {
        llvm::errs() << "Using Clang resource directory for overflow detection: " << resource_dir << "\n";
      }
    }

    // Create frontend action that collects overflow issues
    class OverflowDetectionAction : public clang::ASTFrontendAction {
    public:
      explicit OverflowDetectionAction(std::vector<optiweave::analysis::OverflowIssue>* shared_issues)
        : shared_issues_(shared_issues) {}

      std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
          clang::CompilerInstance& CI, llvm::StringRef file) override {

        class OverflowConsumer : public clang::ASTConsumer {
        public:
          OverflowConsumer(clang::ASTContext& context,
                          std::vector<optiweave::analysis::OverflowIssue>* shared_issues)
            : detector_(context), shared_issues_(shared_issues) {}

          void HandleTranslationUnit(clang::ASTContext& context) override {
            detector_.TraverseDecl(context.getTranslationUnitDecl());

            // Merge issues into shared list
            if (shared_issues_) {
              const auto& issues = detector_.get_issues();
              shared_issues_->insert(shared_issues_->end(), issues.begin(), issues.end());
            }
          }

        private:
          optiweave::analysis::OverflowDetector detector_;
          std::vector<optiweave::analysis::OverflowIssue>* shared_issues_;
        };

        return std::make_unique<OverflowConsumer>(CI.getASTContext(), shared_issues_);
      }

    private:
      std::vector<optiweave::analysis::OverflowIssue>* shared_issues_;
    };

    class OverflowDetectionFactory : public clang::tooling::FrontendActionFactory {
    public:
      OverflowDetectionFactory() {}

      std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<OverflowDetectionAction>(&shared_issues_);
      }

      const std::vector<optiweave::analysis::OverflowIssue>& getIssues() const {
        return shared_issues_;
      }

    private:
      std::vector<optiweave::analysis::OverflowIssue> shared_issues_;
    };

    OverflowDetectionFactory overflow_factory;
    int overflow_result = OverflowTool.run(&overflow_factory);

    if (Verbose) {
      llvm::errs() << "Overflow detection return code: " << overflow_result << "\n";
    }

    const auto& issues = overflow_factory.getIssues();

    if (Verbose) {
      llvm::errs() << "Issues detected: " << issues.size() << "\n";
    }

    // Export results based on format
    std::string format = OverflowFormat;
    std::string output_file = OverflowOutput;

    try {
      if (format == "json") {
        // Create a temporary detector just for export (we have the issues)
        // We'll write JSON manually
        std::error_code EC;
        llvm::raw_fd_ostream out(output_file, EC);
        if (EC) {
          throw std::runtime_error("Could not open file for writing: " + output_file);
        }

        out << "{\n";
        out << "  \"total_issues\": " << issues.size() << ",\n";
        out << "  \"issues\": [\n";

        for (size_t i = 0; i < issues.size(); ++i) {
          const auto& issue = issues[i];
          out << "    {\n";
          out << "      \"type\": \"" << optiweave::analysis::overflow_type_to_string(issue.type) << "\",\n";
          out << "      \"severity\": \"" << optiweave::analysis::severity_to_string(issue.severity) << "\",\n";
          out << "      \"file\": \"" << issue.file << "\",\n";
          out << "      \"line\": " << issue.line << ",\n";
          out << "      \"column\": " << issue.column << ",\n";
          out << "      \"function\": \"" << issue.function << "\",\n";
          out << "      \"description\": \"" << issue.description << "\",\n";
          out << "      \"suggestion\": \"" << issue.suggestion << "\"\n";
          out << "    }";
          if (i < issues.size() - 1) {
            out << ",";
          }
          out << "\n";
        }

        out << "  ]\n";
        out << "}\n";
        out.flush();
      } else {
        // Text format
        std::error_code EC;
        llvm::raw_fd_ostream out(output_file, EC);
        if (EC) {
          throw std::runtime_error("Could not open file for writing: " + output_file);
        }

        out << "=== Integer Overflow Detection Report ===\n\n";
        out << "Total issues found: " << issues.size() << "\n\n";

        // Group by severity
        size_t critical = 0, warning = 0, info = 0;
        for (const auto& issue : issues) {
          if (issue.severity == optiweave::analysis::OverflowSeverity::CRITICAL) critical++;
          else if (issue.severity == optiweave::analysis::OverflowSeverity::WARNING) warning++;
          else info++;
        }

        out << "Critical: " << critical << "\n";
        out << "Warning: " << warning << "\n";
        out << "Info: " << info << "\n\n";

        // Print all issues
        for (const auto& issue : issues) {
          out << "[" << optiweave::analysis::severity_to_string(issue.severity) << "] ";
          out << optiweave::analysis::overflow_type_to_string(issue.type) << "\n";
          out << "  Location: " << issue.file << ":" << issue.line << ":" << issue.column;
          if (!issue.function.empty()) {
            out << " (in " << issue.function << ")";
          }
          out << "\n";
          out << "  Description: " << issue.description << "\n";
          if (!issue.suggestion.empty()) {
            out << "  Suggestion: " << issue.suggestion << "\n";
          }
          out << "\n";
        }

        out.flush();
      }

      llvm::errs() << "Overflow detection exported to: " << output_file << "\n";

      // Print summary to stdout
      if (Verbose && issues.size() > 0) {
        llvm::outs() << "\nOverflow Detection Summary:\n";
        llvm::outs() << "  Total issues: " << issues.size() << "\n";

        // Count by severity
        size_t critical = 0, warning = 0, info = 0;
        for (const auto& issue : issues) {
          if (issue.severity == optiweave::analysis::OverflowSeverity::CRITICAL) critical++;
          else if (issue.severity == optiweave::analysis::OverflowSeverity::WARNING) warning++;
          else info++;
        }

        llvm::outs() << "  Critical: " << critical << "\n";
        llvm::outs() << "  Warning: " << warning << "\n";
        llvm::outs() << "  Info: " << info << "\n";
      }
    } catch (const std::exception& e) {
      llvm::errs() << "Error exporting overflow detection: " << e.what() << "\n";
    }
  }

  // Perform FP precision warnings if requested
  if (EnableFPPrecisionWarnings) {
    if (Verbose) {
      llvm::errs() << "Performing floating-point precision analysis...\n";
    }

    // Create a new tool for FP precision detection
    ClangTool FPPrecisionTool(OptionsParser.getCompilations(),
                              OptionsParser.getSourcePathList());

    // Add templates directory to include path
    if (llvm::sys::fs::exists(templates_dir)) {
      std::string include_arg = "-I" + templates_dir.str().str();
      FPPrecisionTool.appendArgumentsAdjuster(getInsertArgumentAdjuster(include_arg.c_str(), clang::tooling::ArgumentInsertPosition::BEGIN));
    }

    // Add C++20 standard
    FPPrecisionTool.appendArgumentsAdjuster(getInsertArgumentAdjuster(lang_std, clang::tooling::ArgumentInsertPosition::BEGIN));

    // Add Clang resource directory
    if (!resource_dir.empty() && llvm::sys::fs::exists(resource_dir)) {
      std::string resource_arg = "-resource-dir=" + resource_dir;
      FPPrecisionTool.appendArgumentsAdjuster(getInsertArgumentAdjuster(resource_arg.c_str(), clang::tooling::ArgumentInsertPosition::BEGIN));
    }

    // Create frontend action for FP precision detection
    class FPPrecisionAction : public clang::ASTFrontendAction {
    public:
      explicit FPPrecisionAction(std::vector<optiweave::analysis::FPPrecisionIssue>* shared_issues)
        : shared_issues_(shared_issues) {}

      std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
          clang::CompilerInstance& CI, llvm::StringRef file) override {

        class FPPrecisionConsumer : public clang::ASTConsumer {
        public:
          FPPrecisionConsumer(clang::ASTContext& context,
                             std::vector<optiweave::analysis::FPPrecisionIssue>* shared_issues)
            : detector_(context), shared_issues_(shared_issues) {}

          void HandleTranslationUnit(clang::ASTContext& context) override {
            detector_.TraverseDecl(context.getTranslationUnitDecl());

            if (shared_issues_) {
              const auto& issues = detector_.get_issues();
              shared_issues_->insert(shared_issues_->end(), issues.begin(), issues.end());
            }
          }

        private:
          optiweave::analysis::FPPrecisionDetector detector_;
          std::vector<optiweave::analysis::FPPrecisionIssue>* shared_issues_;
        };

        return std::make_unique<FPPrecisionConsumer>(CI.getASTContext(), shared_issues_);
      }

    private:
      std::vector<optiweave::analysis::FPPrecisionIssue>* shared_issues_;
    };

    class FPPrecisionFactory : public clang::tooling::FrontendActionFactory {
    public:
      std::unique_ptr<clang::FrontendAction> create() override {
        return std::make_unique<FPPrecisionAction>(&shared_issues_);
      }

      const std::vector<optiweave::analysis::FPPrecisionIssue>& getIssues() const {
        return shared_issues_;
      }

    private:
      std::vector<optiweave::analysis::FPPrecisionIssue> shared_issues_;
    };

    FPPrecisionFactory fp_factory;
    int fp_result = FPPrecisionTool.run(&fp_factory);

    const auto& fp_issues = fp_factory.getIssues();

    if (Verbose) {
      llvm::errs() << "FP precision issues detected: " << fp_issues.size() << "\n";
    }

    // Export results
    std::string format = FPPrecisionFormat;
    std::string output_file = FPPrecisionOutput;

    try {
      if (format == "json") {
        // Write JSON manually
        std::error_code EC;
        llvm::raw_fd_ostream out(output_file, EC);
        if (EC) {
          throw std::runtime_error("Could not open file for writing: " + output_file);
        }

        out << "{\n";
        out << "  \"total_issues\": " << fp_issues.size() << ",\n";
        out << "  \"issues\": [\n";

        for (size_t i = 0; i < fp_issues.size(); ++i) {
          const auto& issue = fp_issues[i];
          out << "    {\n";
          out << "      \"type\": \"fp_precision_issue\",\n";
          out << "      \"severity\": \"" << (issue.severity == optiweave::analysis::FPPrecisionSeverity::CRITICAL ? "critical" :
                                                issue.severity == optiweave::analysis::FPPrecisionSeverity::WARNING ? "warning" : "info") << "\",\n";
          out << "      \"file\": \"" << issue.file << "\",\n";
          out << "      \"line\": " << issue.line << ",\n";
          out << "      \"column\": " << issue.column << ",\n";
          out << "      \"function\": \"" << issue.function_name << "\",\n";
          out << "      \"description\": \"" << issue.description << "\",\n";
          out << "      \"suggestion\": \"" << issue.suggestion << "\"";
          if (!issue.expression_text.empty()) {
            out << ",\n      \"expression\": \"" << issue.expression_text << "\"";
          }
          out << "\n    }";
          if (i < fp_issues.size() - 1) {
            out << ",";
          }
          out << "\n";
        }

        out << "  ]\n";
        out << "}\n";
        out.flush();
      } else {
        // Write text format
        std::error_code EC;
        llvm::raw_fd_ostream out(output_file, EC);
        if (EC) {
          throw std::runtime_error("Could not open file for writing: " + output_file);
        }

        out << "=== Floating-Point Precision Warning Report ===\n\n";
        out << "Total issues found: " << fp_issues.size() << "\n\n";

        // Count by severity
        size_t critical = 0, warning = 0, info = 0;
        for (const auto& issue : fp_issues) {
          if (issue.severity == optiweave::analysis::FPPrecisionSeverity::CRITICAL) critical++;
          else if (issue.severity == optiweave::analysis::FPPrecisionSeverity::WARNING) warning++;
          else info++;
        }

        out << "Critical: " << critical << "\n";
        out << "Warning: " << warning << "\n";
        out << "Info: " << info << "\n\n";

        // Print all issues
        for (const auto& issue : fp_issues) {
          out << "[" << (issue.severity == optiweave::analysis::FPPrecisionSeverity::CRITICAL ? "critical" :
                         issue.severity == optiweave::analysis::FPPrecisionSeverity::WARNING ? "warning" : "info") << "] ";
          out << "FP precision issue\n";
          out << "  Location: " << issue.file << ":" << issue.line << ":" << issue.column;
          if (!issue.function_name.empty()) {
            out << " (in " << issue.function_name << ")";
          }
          out << "\n";
          out << "  Description: " << issue.description << "\n";
          if (!issue.suggestion.empty()) {
            out << "  Suggestion: " << issue.suggestion << "\n";
          }
          if (!issue.expression_text.empty()) {
            out << "  Expression: " << issue.expression_text << "\n";
          }
          out << "\n";
        }

        out.flush();
      }

      llvm::errs() << "FP precision warnings exported to: " << output_file << "\n";

      if (Verbose && fp_issues.size() > 0) {
        llvm::outs() << "\nFP Precision Summary:\n";
        llvm::outs() << "  Total issues: " << fp_issues.size() << "\n";

        size_t critical = 0, warning = 0, info = 0;
        for (const auto& issue : fp_issues) {
          if (issue.severity == optiweave::analysis::FPPrecisionSeverity::CRITICAL) critical++;
          else if (issue.severity == optiweave::analysis::FPPrecisionSeverity::WARNING) warning++;
          else info++;
        }

        llvm::outs() << "  Critical: " << critical << "\n";
        llvm::outs() << "  Warning: " << warning << "\n";
        llvm::outs() << "  Info: " << info << "\n";
      }
    } catch (const std::exception& e) {
      llvm::errs() << "Error exporting FP precision warnings: " << e.what() << "\n";
    }
  }

  // Handle --print-compile-cmd: show the command and exit
  if (PrintCompileCmd) {
    auto compile_cmd = optiweave::buildCompileCommand(source_paths);
    if (compile_cmd.empty()) {
      return 1;
    }
    llvm::outs() << optiweave::formatCompileCommand(compile_cmd) << "\n";
    return 0;
  }

  // Always attempt compilation if requested (transformation often succeeds despite parse warnings)
  if (CompileAfterTransform) {
    if (Verbose) {
      llvm::errs() << "Starting compilation...\n";
    }

    if (!optiweave::compileTransformedFiles(source_paths)) {
      llvm::errs() << "Compilation failed\n";
      return 1;
    }
    return 0;
  }

  // Check if any instrumentation transforms were applied (not analysis-only)
  bool did_transform = TransformArraySubscripts || TransformArithmetic ||
                        TransformAssignment || TransformComparison;
  bool analysis_only = AnalyzeComplexity || EnableCallGraph || EnableDependencyGraph ||
                       EnableDataFlowAnalysis || EnableMemoryProfiling ||
                       EnableOverflowDetection || EnableFPPrecisionWarnings;

  // Print compile hint after successful transformation (not dry-run, not analysis-only)
  if (result == 0 && did_transform && !DryRun && !analysis_only) {
    llvm::errs() << "\nTransformation complete. To compile the instrumented code:\n";
    llvm::errs() << "  optiweave " << source_paths[0]
                 << " --compile -o " << llvm::sys::path::stem(source_paths[0]).str()
                 << "_instrumented -- -std=c++20\n";
    llvm::errs() << "\nOr to see the raw compile command:\n";
    llvm::errs() << "  optiweave " << source_paths[0]
                 << " --print-compile-cmd -- -std=c++20\n";
  }

  return result;
}

// Additional helper for integration with build systems
extern "C" {
/**
 * @brief C API for integrating with build systems
 * @param argc Number of arguments
 * @param argv Argument array
 * @return 0 on success, non-zero on failure
 */
int optiweave_transform_files(int argc, const char **argv) {
  return main(argc, argv);
}

/**
 * @brief Get OptiWeave version string
 * @return Version string
 */
const char *optiweave_version() { return "1.0.0"; }
}
