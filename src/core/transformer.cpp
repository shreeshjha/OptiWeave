#include "../../include/optiweave/core/transformer.hpp"
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/raw_ostream.h>

namespace optiweave::core {

TransformationResult SourceTransformer::transformFiles(
    const std::vector<std::string> &sources,
    const std::vector<std::string> &compiler_args,
    const TransformationConfig &config) {
    
    TransformationResult result;
    
    try {
        // Create a compilation database from arguments
        auto compilation_db = std::make_unique<clang::tooling::FixedCompilationDatabase>(
            ".", compiler_args);
        
        clang::tooling::ClangTool tool(*compilation_db, sources);
        
        // Add arguments adjuster for include paths - fixed for LLVM 17
        if (!config.include_paths.empty()) {
            std::vector<std::string> include_args;
            for (const auto& path : config.include_paths) {
                include_args.push_back("-I" + path);
            }
            auto adjuster = clang::tooling::getInsertArgumentAdjuster(
                include_args, clang::tooling::ArgumentInsertPosition::BEGIN);
            tool.appendArgumentsAdjuster(adjuster);
        }
        
        // Run transformation
        int error_code = tool.run(
            clang::tooling::newFrontendActionFactory<TransformationAction>().get());
        
        result.success = (error_code == 0);
        if (!result.success) {
            result.error_message = "Transformation failed with error code: " + 
                                 std::to_string(error_code);
        }
        
    } catch (const std::exception &e) {
        result.success = false;
        result.error_message = std::string("Exception during transformation: ") + e.what();
    }
    
    return result;
}

TransformationResult SourceTransformer::transformProject(
    const std::string &compilation_database_path,
    const TransformationConfig &config) {
    
    TransformationResult result;
    
    try {
        std::string error_message;
        auto compilation_db = clang::tooling::CompilationDatabase::autoDetectFromDirectory(
            compilation_database_path, error_message);
        
        if (!compilation_db) {
            result.success = false;
            result.error_message = "Failed to load compilation database: " + error_message;
            return result;
        }
        
        auto source_paths = compilation_db->getAllFiles();
        clang::tooling::ClangTool tool(*compilation_db, source_paths);
        
        // Run transformation
        int error_code = tool.run(
            clang::tooling::newFrontendActionFactory<TransformationAction>().get());
        
        result.success = (error_code == 0);
        if (!result.success) {
            result.error_message = "Project transformation failed with error code: " + 
                                 std::to_string(error_code);
        }
        
    } catch (const std::exception &e) {
        result.success = false;
        result.error_message = std::string("Exception during project transformation: ") + e.what();
    }
    
    return result;
}

std::unique_ptr<clang::ASTConsumer> TransformationAction::CreateASTConsumer(
    clang::CompilerInstance &CI, clang::StringRef file) {
    
    rewriter_.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return std::make_unique<TransformationConsumer>(rewriter_, CI.getASTContext(), config_);
}

void TransformationAction::EndSourceFileAction() {
    // Write the transformed source code
    auto &source_manager = rewriter_.getSourceMgr();
    auto main_file_id = source_manager.getMainFileID();
    
    if (auto buffer = rewriter_.getRewriteBufferFor(main_file_id)) {
        llvm::outs() << std::string(buffer->begin(), buffer->end());
    } else {
        // No changes were made, output original - fixed for LLVM 17
        auto file_buffer = source_manager.getBufferOrNone(main_file_id);
        if (file_buffer) {
            llvm::outs() << file_buffer->getBuffer();
        }
    }
}

} // namespace optiweave::core