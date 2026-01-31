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

# Platform-specific LLVM paths
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Linux: Check for LLVM 17 installation (required version)
    set(LLVM_SEARCH_PATHS
        /usr/lib/llvm-17/lib/cmake/llvm
        /usr/local/lib/cmake/llvm
    )
    set(CLANG_SEARCH_PATHS
        /usr/lib/llvm-17/lib/cmake/clang
        /usr/local/lib/cmake/clang
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # macOS: Homebrew LLVM installations
    set(LLVM_SEARCH_PATHS
        /opt/homebrew/opt/llvm@17/lib/cmake/llvm
        /opt/homebrew/opt/llvm/lib/cmake/llvm
        /usr/local/opt/llvm@17/lib/cmake/llvm
        /usr/local/opt/llvm/lib/cmake/llvm
    )
    set(CLANG_SEARCH_PATHS
        /opt/homebrew/opt/llvm@17/lib/cmake/clang
        /opt/homebrew/opt/llvm/lib/cmake/clang
        /usr/local/opt/llvm@17/lib/cmake/clang
        /usr/local/opt/llvm/lib/cmake/clang
    )
else()
    message(WARNING "Unknown platform: ${CMAKE_SYSTEM_NAME}. Using default LLVM search paths.")
    set(LLVM_SEARCH_PATHS "")
    set(CLANG_SEARCH_PATHS "")
endif()

find_package(LLVM CONFIG REQUIRED
    PATHS ${LLVM_SEARCH_PATHS}
    NO_DEFAULT_PATH
)

find_package(Clang CONFIG REQUIRED
    PATHS ${CLANG_SEARCH_PATHS}
    NO_DEFAULT_PATH
)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Found Clang ${CLANG_PACKAGE_VERSION}")

# --------------------------------------------------------------------------
# Platform-specific compiler flags
# --------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # On Linux with Clang + GCC's libstdc++, we need to point to the GCC toolchain
    # This resolves header/library lookup issues
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Find GCC installation for libstdc++ headers
        if(EXISTS "/usr/lib/gcc/x86_64-linux-gnu")
            message(STATUS "Linux detected: Adding GCC toolchain flag for Clang")
            add_compile_options("--gcc-toolchain=/usr")
        endif()
    endif()
endif()

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
    src/analysis/pattern_detector.cpp
    src/analysis/complexity_analyzer.cpp
    src/analysis/complexity_export.cpp
    src/analysis/call_graph.cpp
    src/analysis/dependency_graph.cpp
    src/analysis/data_flow_analysis.cpp
    src/analysis/memory_profiler.cpp
    src/analysis/overflow_detector.cpp
    src/analysis/fp_precision_detector.cpp
    src/core/ast_visitor.cpp
    src/core/rewriter.cpp
    src/core/transformer.cpp
    src/runtime/loop_info_serializer.cpp
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

# Determine LLVM linking mode - on Linux with shared LLVM, use the shared library
# to avoid duplicate symbol registration issues
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND TARGET LLVM)
    set(LLVM_LIBS LLVM)
else()
    set(LLVM_LIBS LLVMSupport LLVMCore)
endif()

target_link_libraries(optiweave_core
    PRIVATE
      # LLVM libraries (platform-dependent linking)
      ${LLVM_LIBS}

      # Clang libraries (unnamespaced)
      clangTooling    # ClangTool, CommonOptionsParser
      clangBasic      # Diagnostics, SourceManager, ASTContext setup
      clangAST        # ASTContext, Decl, Stmt, RecursiveASTVisitor
      clangFrontend   # FrontendAction, ASTConsumer
      clangRewrite    # Rewriter, DeltaTree
)

# --------------------------------------------------------------------------
# Build the runtime library
# --------------------------------------------------------------------------
add_library(optiweave_runtime STATIC
    src/runtime/optiweave_runtime.cpp
    src/runtime/statistics.cpp
    src/runtime/c_compat/statistics_c.c
    src/runtime/timing.cpp
    src/runtime/hotspot_tracker.cpp
    src/runtime/optimization_suggestions.cpp
    src/runtime/loop_info_serializer.cpp
    src/runtime/json_exporter.cpp
    src/runtime/cache_profiler.cpp
    src/analysis/pattern_detector.cpp
)
target_include_directories(optiweave_runtime PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/templates>
    $<INSTALL_INTERFACE:include>
)
# Always compile runtime with statistics, timing, hotspot, suggestions, and cache profiling support (users can enable/disable at runtime via env vars)
target_compile_definitions(optiweave_runtime PRIVATE
    OPTIWEAVE_ENABLE_STATS
    OPTIWEAVE_ENABLE_TIMING
    OPTIWEAVE_ENABLE_HOTSPOTS
    OPTIWEAVE_ENABLE_SUGGESTIONS
    OPTIWEAVE_ENABLE_CACHE_PROFILE
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
