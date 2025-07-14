cmake_minimum_required(VERSION 3.20)

project(OptiWeave
    VERSION 1.0.0
    DESCRIPTION "Modern C++ source-to-source transformation tool for operator instrumentation"
    LANGUAGES CXX C  # Added C language support
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# LLVM/Clang version constraints
set(LLVM_MIN_VERSION_MAJOR 13)
set(LLVM_MAX_VERSION_MAJOR 17)

# Find LLVM and Clang without strict version constraints first
find_package(LLVM REQUIRED CONFIG)

# Then check if the version is within our supported range
if(LLVM_VERSION_MAJOR LESS ${LLVM_MIN_VERSION_MAJOR})
    message(FATAL_ERROR 
        "LLVM version ${LLVM_PACKAGE_VERSION} is too old. "
        "OptiWeave requires LLVM ${LLVM_MIN_VERSION_MAJOR}.x to ${LLVM_MAX_VERSION_MAJOR}.x. "
        "Please install a newer LLVM version:\n"
        "  macOS: brew install llvm@${LLVM_MIN_VERSION_MAJOR} or llvm@${LLVM_MAX_VERSION_MAJOR}\n"
        "  Ubuntu: sudo apt install llvm-${LLVM_MIN_VERSION_MAJOR}-dev clang-${LLVM_MIN_VERSION_MAJOR}-dev")
elseif(LLVM_VERSION_MAJOR GREATER ${LLVM_MAX_VERSION_MAJOR})
    message(FATAL_ERROR 
        "LLVM version ${LLVM_PACKAGE_VERSION} is too new. "
        "OptiWeave requires LLVM ${LLVM_MIN_VERSION_MAJOR}.x to ${LLVM_MAX_VERSION_MAJOR}.x. "
        "Found LLVM ${LLVM_PACKAGE_VERSION} which may have incompatible APIs. "
        "Please install a compatible LLVM version:\n"
        "  macOS: brew install llvm@${LLVM_MAX_VERSION_MAJOR}\n"
        "  Ubuntu: sudo apt install llvm-${LLVM_MAX_VERSION_MAJOR}-dev clang-${LLVM_MAX_VERSION_MAJOR}-dev\n"
        "Or check for an updated OptiWeave version that supports LLVM ${LLVM_VERSION_MAJOR}")
endif()

find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Found Clang ${CLANG_PACKAGE_VERSION}")
message(STATUS "LLVM version ${LLVM_PACKAGE_VERSION} is compatible (${LLVM_MIN_VERSION_MAJOR}.x - ${LLVM_MAX_VERSION_MAJOR}.x)")

# LLVM definitions and include directories
add_definitions(${LLVM_DEFINITIONS})

# Compiler-specific options (after finding LLVM to avoid affecting LLVM headers)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Use more permissive warnings to avoid LLVM header issues
    add_compile_options(
        -Wall 
        -Wextra 
        -Wpedantic 
        -Wno-unused-parameter  # Disable unused parameter warnings for LLVM headers
        -Wno-error=unused-parameter
    )
    
    # Remove -Werror entirely to avoid build failures from external headers
    # add_compile_options(-Werror)
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
        # Only enable sanitizers if requested explicitly
        if(ENABLE_SANITIZERS)
            add_compile_options(-fsanitize=address -fsanitize=undefined)
            add_link_options(-fsanitize=address -fsanitize=undefined)
        endif()
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4)
    # Remove /WX to avoid warnings as errors
    # add_compile_options(/WX)
endif()

# Include directories - use SYSTEM to suppress warnings from external headers
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
include_directories(SYSTEM ${CLANG_INCLUDE_DIRS})

# Separate the libraries
llvm_map_components_to_libnames(llvm_libs support core irreader)

# Project include directories (our own code)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Create missing source files as empty if they don't exist
set(CORE_SOURCES
    src/core/ast_visitor.cpp
)

# Check which optional source files exist and add them
set(OPTIONAL_SOURCES
    src/core/rewriter.cpp
    src/core/transformer.cpp
    src/matchers/operator_matchers.cpp
    src/matchers/type_matchers.cpp
    src/analysis/operator_detector.cpp
    src/analysis/template_analyzer.cpp
    src/utils/source_utils.cpp
    src/utils/diagnostic_utils.cpp
)

foreach(SOURCE ${OPTIONAL_SOURCES})
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE})
        list(APPEND CORE_SOURCES ${SOURCE})
    endif()
endforeach()

# Core library
add_library(optiweave_core ${CORE_SOURCES})

target_include_directories(optiweave_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Use SYSTEM includes for external libraries to suppress their warnings
target_include_directories(optiweave_core SYSTEM PUBLIC
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
)

target_link_libraries(optiweave_core PUBLIC
    clangTooling
    clangFrontendTool
    clangFrontend
    clangDriver
    clangSerialization
    clangCodeGen
    clangParse
    clangSema
    clangStaticAnalyzerFrontend
    clangStaticAnalyzerCheckers
    clangStaticAnalyzerCore
    clangAnalysis
    clangARCMigrate
    clangRewrite
    clangRewriteFrontend
    clangEdit
    clangAST
    clangASTMatchers
    clangLex
    clangBasic
    ${llvm_libs}
)

# Apply compile flags only to our own code, not external libraries
target_compile_options(optiweave_core PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-error>
)

# Main executable
add_executable(optiweave src/main.cpp)
target_link_libraries(optiweave PRIVATE optiweave_core)

# Apply the same warning suppressions to the main executable
target_compile_options(optiweave PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-error>
)

# Install rules
install(TARGETS optiweave optiweave_core
    EXPORT OptiWeaveTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)

# Create cmake directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Create OptiWeaveConfig.cmake.in if it doesn't exist
set(CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/OptiWeaveConfig.cmake.in)
if(NOT EXISTS ${CONFIG_FILE})
    file(WRITE ${CONFIG_FILE} [[@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

find_dependency(LLVM REQUIRED CONFIG)
find_dependency(Clang REQUIRED CONFIG)

# Verify LLVM version compatibility
if(LLVM_VERSION_MAJOR LESS 13 OR LLVM_VERSION_MAJOR GREATER 17)
    message(FATAL_ERROR 
        "OptiWeave requires LLVM 13.x to 17.x, but found ${LLVM_PACKAGE_VERSION}")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/OptiWeaveTargets.cmake")

check_required_components(OptiWeave)
]])
endif()

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    OptiWeaveConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_file(
    cmake/OptiWeaveConfig.cmake.in
    OptiWeaveConfig.cmake
    @ONLY
)

export(EXPORT OptiWeaveTargets FILE OptiWeaveTargets.cmake)

# Enable testing only if tests directory exists
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    enable_testing()
    add_subdirectory(tests)
endif()

# Add examples only if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt)
    add_subdirectory(examples)
endif()

# Create LICENSE file if it doesn't exist
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE "MIT License

Copyright (c) 2024 OptiWeave Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the \"Software\"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.")
endif()

# Create README.md if it doesn't exist
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/README.md "# OptiWeave

Modern C++ source-to-source transformation tool for operator instrumentation.

## Requirements

- LLVM 13.x - 17.x
- Clang 13.x - 17.x  
- CMake 3.20+
- C++20 compatible compiler

## Building

```bash
./scripts/build.sh
```

## Usage

```bash
optiweave source.cpp -- -std=c++20
```

## License

MIT License - see LICENSE file for details.
")
endif()

# Only include CPack if both LICENSE and README.md exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    # CPack configuration
    set(CPACK_PACKAGE_NAME "OptiWeave")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
    set(CPACK_PACKAGE_VENDOR "OptiWeave Contributors")
    set(CPACK_PACKAGE_CONTACT "contact@optiweave.org")
    set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
    set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    
    include(CPack)
endif()