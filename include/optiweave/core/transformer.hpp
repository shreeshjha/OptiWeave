#pragma once

#include "ast_visitor.hpp"
#include <clang/Frontend/FrontendAction.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <string>
#include <vector>
#include <memory>

namespace optiweave::core {

/**
 * @brief Result of a transformation operation
 */
struct TransformationResult {
    bool success = false;
    std::string error_message;
    TransformationStats stats;
};

/**
 * @brief High-level interface for source transformation
 */
class SourceTransformer {
public:
    /**
     * @brief Transform a list of source files
     */
    static TransformationResult transformFiles(
        const std::vector<std::string> &sources,
        const std::vector<std::string> &compiler_args,
        const TransformationConfig &config = {});
    
    /**
     * @brief Transform an entire project using compilation database
     */
    static TransformationResult transformProject(
        const std::string &compilation_database_path,
        const TransformationConfig &config = {});
};

/**
 * @brief Frontend action for transformations
 */
class TransformationAction : public clang::ASTFrontendAction {
public:
    explicit TransformationAction(const TransformationConfig &config = {})
        : config_(config) {}
    
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
        clang::CompilerInstance &CI, clang::StringRef file) override;
    
    void EndSourceFileAction() override;

private:
    TransformationConfig config_;
    clang::Rewriter rewriter_;
};

} // namespace optiweave::core