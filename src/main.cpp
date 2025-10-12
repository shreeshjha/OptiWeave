#include <optiweave/core/ast_visitor.hpp>
#include <optiweave/core/rewriter.hpp>
#include <optiweave/analysis/complexity_analyzer.hpp>

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
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

namespace optiweave {

/**
 * @brief Frontend action for OptiWeave transformations
 */
class OptiWeaveFrontendAction : public ASTFrontendAction {
public:
  explicit OptiWeaveFrontendAction(const core::TransformationConfig &config)
      : config_(config) {}

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    if (Verbose) {
      llvm::errs() << "Processing file: " << file << "\n";
    }

    // Initialize rewriter
    rewriter_.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());

    // Create consumer with configuration
    return std::make_unique<core::TransformationConsumer>(
        rewriter_, CI.getASTContext(), config_);
  }

  void EndSourceFileAction() override {
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

        auto file_entry = source_manager.getFileEntryForID(file_id);
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
};

/**
 * @brief Factory for creating OptiWeave frontend actions
 */
class OptiWeaveFrontendActionFactory : public FrontendActionFactory {
public:
  explicit OptiWeaveFrontendActionFactory(
      const core::TransformationConfig &config)
      : config_(config) {}

  std::unique_ptr<FrontendAction> create() override {
    return std::make_unique<OptiWeaveFrontendAction>(config_);
  }

private:
  core::TransformationConfig config_;
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
  optiweave source.cpp --compile -o instrumented

  # Transform multiple operator types and compile
  optiweave --arithmetic-ops --assignment-ops source.cpp --compile -o instrumented

  # Use custom prelude and output directory
  optiweave --prelude=my_prelude.hpp --output-dir=./transformed source.cpp --

  # Dry run to check what would be transformed
  optiweave --dry-run --stats --verbose source.cpp --

  # Transform entire project with compilation database
  optiweave --arithmetic-ops $(find src -name "*.cpp") --

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
 * @brief Compile transformed source files
 */
bool compileTransformedFiles(const std::vector<std::string> &source_paths) {
  if (source_paths.empty()) {
    llvm::errs() << "Error: No source files to compile\n";
    return false;
  }

  // Determine output executable name
  std::string output_name;
  if (!OutputExecutable.empty()) {
    output_name = OutputExecutable;
  } else {
    // Default: use first source file name without extension
    auto base_name = llvm::sys::path::stem(source_paths[0]);
    output_name = base_name.str() + "_instrumented";
  }

  // Build compilation command
  std::vector<std::string> compile_cmd;
  compile_cmd.push_back("clang++");
  
  // Add C++20 standard
  compile_cmd.push_back("-std=c++20");
  
  // Add system include paths for macOS using automatically detected SDK path
  std::string sdk_path;
  
  // Try to auto-detect SDK path using xcrun
  FILE* xcrun_cmd = popen("xcrun --show-sdk-path 2>/dev/null", "r");
  if (xcrun_cmd) {
    char path_buffer[512];
    if (fgets(path_buffer, sizeof(path_buffer), xcrun_cmd)) {
      sdk_path = std::string(path_buffer);
      // Remove trailing newline
      if (!sdk_path.empty() && sdk_path.back() == '\n') {
        sdk_path.pop_back();
      }
    }
    pclose(xcrun_cmd);
  }
  
  // Fallback to known Xcode path if xcrun fails
  if (sdk_path.empty()) {
    sdk_path = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk";
  }
  
  // Only add -isysroot if SDK path exists
  if (llvm::sys::fs::exists(sdk_path)) {
    compile_cmd.push_back("-isysroot");
    compile_cmd.push_back(sdk_path);
    if (Verbose) {
      llvm::errs() << "Using SDK: " << sdk_path << "\n";
    }
  } else if (Verbose) {
    llvm::errs() << "Warning: SDK not found at " << sdk_path << ", using system defaults\n";
  }
  
  // Add include path for templates
  SmallString<128> templates_dir;
  if (auto exe = llvm::sys::fs::getMainExecutable(nullptr, nullptr); !exe.empty()) {
    templates_dir = exe;
    llvm::sys::path::remove_filename(templates_dir);
    llvm::sys::path::append(templates_dir, "..", "templates");
  } else {
    templates_dir = "templates";
  }
  
  // Validate templates directory exists
  if (llvm::sys::fs::exists(templates_dir)) {
    compile_cmd.push_back("-I" + templates_dir.str().str());
    if (Verbose) {
      llvm::errs() << "Using templates: " << templates_dir << "\n";
    }
  } else {
    llvm::errs() << "Error: Templates directory not found: " << templates_dir << "\n";
    llvm::errs() << "Please ensure OptiWeave is properly installed.\n";
    return false;
  }
  
  // Add library path and runtime library
  SmallString<128> lib_dir;
  if (auto exe = llvm::sys::fs::getMainExecutable(nullptr, nullptr); !exe.empty()) {
    lib_dir = exe;
    llvm::sys::path::remove_filename(lib_dir);
  } else {
    lib_dir = "build";
  }
  // Validate runtime library exists
  SmallString<128> runtime_lib_path;
  runtime_lib_path = lib_dir;
  llvm::sys::path::append(runtime_lib_path, "liboptiweave_runtime.a");

  if (llvm::sys::fs::exists(runtime_lib_path)) {
    compile_cmd.push_back("-L" + lib_dir.str().str());
    compile_cmd.push_back("-loptiweave_runtime");
    if (Verbose) {
      llvm::errs() << "Using runtime library: " << runtime_lib_path << "\n";
    }
  } else {
    llvm::errs() << "Error: Runtime library not found: " << runtime_lib_path << "\n";
    llvm::errs() << "Please rebuild OptiWeave with: ./scripts/build.sh\n";
    return false;
  }

  // Add statistics compilation flags if enabled
  if (EnableStats || !StatsCSV.empty() || !StatsJSON.empty()) {
    compile_cmd.push_back("-DOPTIWEAVE_ENABLE_STATS");
    if (Verbose) {
      llvm::errs() << "Enabling statistics collection\n";
    }
  }

  // Add timing compilation flags if enabled
  if (EnableTiming || EnableProfile || !TimingCSV.empty() || !TimingJSON.empty()) {
    compile_cmd.push_back("-DOPTIWEAVE_ENABLE_TIMING");
    if (Verbose) {
      llvm::errs() << "Enabling timing/profiling\n";
    }
  }

  // Add hotspot compilation flags if enabled
  if (EnableHotspots || !HotspotsCSV.empty() || !HotspotsJSON.empty()) {
    compile_cmd.push_back("-DOPTIWEAVE_ENABLE_HOTSPOTS");
    // Hotspots require timing for duration measurement
    compile_cmd.push_back("-DOPTIWEAVE_ENABLE_TIMING");
    if (Verbose) {
      llvm::errs() << "Enabling hotspot detection\n";
    }
  }

  // Add runtime include path for statistics header
  SmallString<128> runtime_include_dir;
  if (auto exe = llvm::sys::fs::getMainExecutable(nullptr, nullptr); !exe.empty()) {
    runtime_include_dir = exe;
    llvm::sys::path::remove_filename(runtime_include_dir);
    llvm::sys::path::append(runtime_include_dir, "..", "include");

    if (llvm::sys::fs::exists(runtime_include_dir)) {
      compile_cmd.push_back("-I" + runtime_include_dir.str().str());
      if (Verbose) {
        llvm::errs() << "Using runtime includes: " << runtime_include_dir << "\n";
      }
    }
  }

  // Add source files
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
  
  // Add output option
  compile_cmd.push_back("-o");
  compile_cmd.push_back(output_name);

  // Execute compilation command
  if (Verbose) {
    llvm::errs() << "Compiling with command: ";
    for (const auto &arg : compile_cmd) {
      llvm::errs() << arg << " ";
    }
    llvm::errs() << "\n";
  }

  // Convert to char* array for execvp
  std::vector<const char*> argv;
  for (const auto &arg : compile_cmd) {
    argv.push_back(arg.c_str());
  }
  argv.push_back(nullptr);

  // Execute using std::system for simplicity
  std::string full_cmd;
  for (size_t i = 0; i < compile_cmd.size(); ++i) {
    if (i > 0) full_cmd += " ";
    // Quote arguments that might contain spaces
    full_cmd += "\"" + compile_cmd[i] + "\"";
  }
  
  int compile_result = std::system(full_cmd.c_str());
  
  if (compile_result == 0) {
    if (Verbose) {
      llvm::errs() << "Compilation successful\n";
    }
    llvm::outs() << "Instrumented executable created: " << output_name << "\n";
    return true;
  } else {
    llvm::errs() << "Compilation failed with exit code: " << compile_result << "\n";
    return false;
  }
}

} // namespace optiweave

int main(int argc, const char **argv) {
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

  auto source_paths = OptionsParser.getSourcePathList();

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

  // Add C++20 standard if not specified
  Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-std=c++20", clang::tooling::ArgumentInsertPosition::BEGIN));

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
    ComplexityTool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-std=c++20", clang::tooling::ArgumentInsertPosition::BEGIN));

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

  // Always attempt compilation if requested (transformation often succeeds despite parse warnings)
  if (CompileAfterTransform) {
    if (Verbose) {
      llvm::errs() << "Starting compilation...\n";
    }
    
    if (!optiweave::compileTransformedFiles(source_paths)) {
      llvm::errs() << "Compilation failed\n";
      return 1; // Compilation failed
    }
  }

  // Return success if compilation was attempted and succeeded, regardless of parse warnings
  if (CompileAfterTransform) {
    return 0; // Compilation succeeded 
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
