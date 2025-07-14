#include "../include/optiweave/core/ast_visitor.hpp"
#include "../include/optiweave/core/rewriter.hpp"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/ArgumentsAdjusters.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include <iostream>
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

static cl::opt<bool> PrintStats("stats",
                                cl::desc("Print transformation statistics"),
                                cl::init(true), cl::cat(OptiWeaveCategory));

static cl::opt<bool>
    DryRun("dry-run", cl::desc("Parse and analyze without writing changes"),
           cl::init(false), cl::cat(OptiWeaveCategory));

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
  llvm::outs() << "Built with Clang " << CLANG_VERSION_STRING << "\n";
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

  # Transform multiple operator types
  optiweave --arithmetic-ops --assignment-ops source.cpp -- -std=c++20

  # Use custom prelude and output directory
  optiweave --prelude=my_prelude.hpp --output-dir=./transformed source.cpp --

  # Dry run to check what would be transformed
  optiweave --dry-run --stats --verbose source.cpp --

  # Transform entire project with compilation database
  optiweave --arithmetic-ops $(find src -name "*.cpp") --

For more information, see: https://github.com/optiweave/optiweave
)";
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

  // Setup prelude
  std::string prelude_path = optiweave::setupPrelude();

  // Configure transformation
  optiweave::core::TransformationConfig config;
  config.transform_array_subscripts = TransformArraySubscripts;
  config.transform_arithmetic_operators = TransformArithmetic;
  config.transform_assignment_operators = TransformAssignment;
  config.transform_comparison_operators = TransformComparison;
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
                 << (config.transform_comparison_operators ? "ON" : "OFF")
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

  // Add prelude to include path if available
  if (!prelude_path.empty()) {
    auto prelude_dir = llvm::sys::path::parent_path(prelude_path);
    std::string include_arg = "-I" + prelude_dir.str();
    Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster(include_arg));
  }

  // Add C++20 standard if not specified
  Tool.appendArgumentsAdjuster(getInsertArgumentAdjuster("-std=c++20"));

  // Create factory and run tool
  optiweave::OptiWeaveFrontendActionFactory factory(config);
  int result = Tool.run(&factory);

  if (result == 0) {
    if (Verbose) {
      llvm::errs() << "Transformation completed successfully\n";
    }
  } else {
    llvm::errs() << "Transformation failed with code: " << result << "\n";
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
