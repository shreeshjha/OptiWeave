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

# Supported LLVM version range
set(OPTIWEAVE_LLVM_MIN_VERSION 13)
set(OPTIWEAVE_LLVM_MAX_VERSION 21)

# Build search paths dynamically for all supported versions
set(LLVM_SEARCH_PATHS "")
set(CLANG_SEARCH_PATHS "")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Search versioned LLVM installs (highest first for preference)
    foreach(ver RANGE ${OPTIWEAVE_LLVM_MAX_VERSION} ${OPTIWEAVE_LLVM_MIN_VERSION} -1)
        list(APPEND LLVM_SEARCH_PATHS  /usr/lib/llvm-${ver}/lib/cmake/llvm)
        list(APPEND CLANG_SEARCH_PATHS /usr/lib/llvm-${ver}/lib/cmake/clang)
    endforeach()
    list(APPEND LLVM_SEARCH_PATHS  /usr/local/lib/cmake/llvm)
    list(APPEND CLANG_SEARCH_PATHS /usr/local/lib/cmake/clang)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Detect Homebrew prefix (works on both Apple Silicon and Intel)
    execute_process(
        COMMAND brew --prefix
        OUTPUT_VARIABLE HOMEBREW_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE BREW_RESULT
    )
    if(NOT BREW_RESULT EQUAL 0)
        # Fallback: check common prefixes
        if(EXISTS /opt/homebrew)
            set(HOMEBREW_PREFIX /opt/homebrew)
        elseif(EXISTS /usr/local/Homebrew)
            set(HOMEBREW_PREFIX /usr/local)
        endif()
    endif()

    if(HOMEBREW_PREFIX)
        # Search versioned formulae (highest first), then unversioned
        foreach(ver RANGE ${OPTIWEAVE_LLVM_MAX_VERSION} ${OPTIWEAVE_LLVM_MIN_VERSION} -1)
            list(APPEND LLVM_SEARCH_PATHS  ${HOMEBREW_PREFIX}/opt/llvm@${ver}/lib/cmake/llvm)
            list(APPEND CLANG_SEARCH_PATHS ${HOMEBREW_PREFIX}/opt/llvm@${ver}/lib/cmake/clang)
        endforeach()
        list(APPEND LLVM_SEARCH_PATHS  ${HOMEBREW_PREFIX}/opt/llvm/lib/cmake/llvm)
        list(APPEND CLANG_SEARCH_PATHS ${HOMEBREW_PREFIX}/opt/llvm/lib/cmake/clang)
    endif()
endif()

# Try hint paths first, then fall back to CMake's default search
find_package(LLVM CONFIG REQUIRED
    HINTS ${LLVM_SEARCH_PATHS}
)
find_package(Clang CONFIG REQUIRED
    HINTS ${CLANG_SEARCH_PATHS}
)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} at ${LLVM_DIR}")
message(STATUS "Found Clang at ${Clang_DIR}")

# Validate version is within supported range
if(LLVM_VERSION_MAJOR LESS OPTIWEAVE_LLVM_MIN_VERSION)
    message(FATAL_ERROR "LLVM ${LLVM_PACKAGE_VERSION} is too old. OptiWeave requires LLVM ${OPTIWEAVE_LLVM_MIN_VERSION}+.")
elseif(LLVM_VERSION_MAJOR GREATER OPTIWEAVE_LLVM_MAX_VERSION)
    message(WARNING "LLVM ${LLVM_PACKAGE_VERSION} is newer than tested (max ${OPTIWEAVE_LLVM_MAX_VERSION}). Build may work but is untested.")
endif()

# --------------------------------------------------------------------------
# Platform-specific compiler flags
# --------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # On Linux with Clang + GCC's libstdc++, we need to point to the GCC toolchain
    # This resolves header/library lookup issues
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # Check for GCC installation on any architecture
        file(GLOB GCC_LIB_DIRS "/usr/lib/gcc/*-linux-gnu*")
        if(GCC_LIB_DIRS)
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
    src/analysis/diff_profile.cpp
    src/runtime/json_parser.cpp
    src/core/ast_visitor.cpp
    src/core/rewriter.cpp
    src/core/transformer.cpp
    src/core/patch_visitor.cpp
    src/core/bug_fix_visitor.cpp
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
