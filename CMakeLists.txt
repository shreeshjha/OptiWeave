cmake_minimum_required(VERSION 3.20)
project(OptiWeave
    VERSION 1.0.0
    DESCRIPTION "Modern C++ source-to-source transformation tool for operator instrumentation"
    LANGUAGES CXX C
)

# Use C++20 without extensions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Release if no build type is specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# User-controllable options
option(BUILD_TESTING       "Enable building and running tests" OFF)
option(BUILD_EXAMPLES      "Enable building examples"        ON)
option(BUILD_DOCUMENTATION "Enable building documentation"   OFF)

# --------------------------------------------------------------------------
# Locate LLVM & Clang via their CMake package configurations
# --------------------------------------------------------------------------
find_package(LLVM CONFIG REQUIRED
    PATHS /opt/homebrew/opt/llvm@17/lib/cmake/llvm
          /opt/homebrew/opt/llvm/lib/cmake/llvm
    NO_DEFAULT_PATH
)                                                                                      # :contentReference[oaicite:0]{index=0}

find_package(Clang CONFIG REQUIRED
    PATHS /opt/homebrew/opt/llvm@17/lib/cmake/clang
          /opt/homebrew/opt/llvm/lib/cmake/clang
    NO_DEFAULT_PATH
)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Found Clang ${CLANG_PACKAGE_VERSION}")

# Pull in include directories for both LLVM and Clang
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# --------------------------------------------------------------------------
# Build the core library
# --------------------------------------------------------------------------
set(OPTIWEAVE_CORE_SOURCES
    src/analysis/operator_detector.cpp
    src/analysis/template_analyzer.cpp
    src/core/ast_visitor.cpp
    src/core/rewriter.cpp
    src/core/transformer.cpp
    src/Matchers/operator_matchers.cpp
    src/Matchers/type_matchers.cpp
    src/utils/diagnostic_utils.cpp
    src/utils/source_utils.cpp
)

add_library(optiweave_core STATIC ${OPTIWEAVE_CORE_SOURCES})

target_include_directories(optiweave_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(optiweave_core
    PRIVATE
      # LLVM components (unnamespaced)
      LLVMSupport     # llvm::errs(), llvm::outs(), llvm::sys::fs, etc.
      LLVMCore        # core LLVM libraries
      #LLVMOption      # llvm::cl::Option machinery

      # Clang libraries (unnamespaced)
      clangTooling    # ClangTool, CommonOptionsParser
      clangBasic      # Diagnostics, SourceManager, ASTContext setup
      clangAST        # ASTContext, Decl, Stmt, RecursiveASTVisitor
      clangFrontend   # FrontendAction, ASTConsumer
      clangRewrite    # Rewriter, DeltaTree
      #clangSerialization
      #clangDriver
      #clangParse
      #clangSema
      #clangAnalysis
      #clangEdit
      #clangLex
)                                                                                      # :contentReference[oaicite:1]{index=1}

# --------------------------------------------------------------------------
# Build the runtime library
# --------------------------------------------------------------------------
add_library(optiweave_runtime STATIC src/runtime/optiweave_runtime.cpp)
target_include_directories(optiweave_runtime PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/templates>
    $<INSTALL_INTERFACE:include>
)

# --------------------------------------------------------------------------
# Build the main executable
# --------------------------------------------------------------------------
add_executable(optiweave src/main.cpp)
target_link_libraries(optiweave
    PRIVATE
      optiweave_core
)

# --------------------------------------------------------------------------
# Output directories
# --------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# --------------------------------------------------------------------------
# Installation rules
# --------------------------------------------------------------------------
install(TARGETS optiweave_core optiweave_runtime optiweave
    EXPORT OptiWeaveTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(EXPORT OptiWeaveTargets
    NAMESPACE OptiWeave::
    DESTINATION lib/cmake/OptiWeave
)

# Install the generated package config
configure_file(cmake/OptiWeaveConfig.cmake.in
               "${CMAKE_BINARY_DIR}/OptiWeaveConfig.cmake" @ONLY)
install(FILES "${CMAKE_BINARY_DIR}/OptiWeaveConfig.cmake"
    DESTINATION lib/cmake/OptiWeave
)

# Install headers
install(DIRECTORY include/optiweave/
    DESTINATION include/optiweave
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Install templates (needed for instrumented code compilation)
install(DIRECTORY templates/
    DESTINATION include/optiweave/templates
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# --------------------------------------------------------------------------
# Examples and Tests
# --------------------------------------------------------------------------
if(BUILD_EXAMPLES)
  message(STATUS "Building examples…")
  add_subdirectory(examples)
endif()

if(BUILD_TESTING)
  message(STATUS "Building tests…")
  enable_testing()
  add_subdirectory(tests)
endif()
