# OptiWeave

A source-to-source instrumentation framework built on LLVM/Clang for **selective operator instrumentation** in C and C++ programs.

[![LLVM Support](https://img.shields.io/badge/LLVM-13--17-blue.svg)](https://llvm.org/)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-red.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## What is OptiWeave?

OptiWeave is a **flexible instrumentation framework** that can selectively instrument any C/C++ operators at the AST level. Currently **focused on array subscript instrumentation**, the framework is designed to support instrumenting arithmetic operators, assignments, comparisons, and more based on your profiling needs.

**Current Implementation:**
- **Array subscript instrumentation**: Production-ready with 8-17% overhead
- **Arithmetic/assignment/comparison operators**: Infrastructure ready, awaiting implementation
- **Extensible prelude system**: Add custom instrumentation wrappers as needed

**Key Features:**
- **Selective instrumentation**: Choose exactly which operations to track
- **Low overhead**: 8-17% runtime overhead for array instrumentation (vs 200% for AddressSanitizer)
- **C and C++ support**: Automatic language detection and appropriate runtime selection
- **Source location tracking**: Know exactly where operations occur in your code
- **Static analysis integration**: Optional overflow detection, complexity analysis, call graphs

**Primary Use Case (Array Profiling):**
- Understanding array access patterns in legacy code
- Profiling array-heavy algorithms (image processing, numerical computing)
- Finding performance bottlenecks in data structure implementations
- Debugging array access issues with minimal overhead

## Quick Start

### For C++ Programs

```bash
# Build OptiWeave
./scripts/build.sh

# Transform your C++ code (auto-detects C++)
./build/optiweave your_code.cpp --array-subscripts

# Compile with runtime library
clang++ -std=c++17 your_code.cpp src/runtime/optiweave_runtime.cpp \
  -I./templates -I./include -o program

# Run with profiling
./program
```

### For C Programs

```bash
# Transform your C code (use -x c flag)
./build/optiweave your_code.c --array-subscripts -- -x c

# Compile with C runtime
clang your_code.c templates/optiweave/optiweave_runtime.c \
  -I./templates -o program

# Run
./program
```

The tool automatically tracks array accesses and displays hot functions on exit.

## Requirements

- LLVM/Clang 13-17
- CMake 3.20+
- C++20 compatible compiler

### Installation

**macOS:**
```bash
brew install llvm@17
echo 'export PATH="/opt/homebrew/opt/llvm@17/bin:$PATH"' >> ~/.zshrc
```

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install llvm-17-dev clang-17-dev libclang-17-dev
```

**Arch Linux:**
```bash
sudo pacman -S llvm17 clang17
```

## Core Features

### Selective Operator Instrumentation

OptiWeave allows you to instrument different types of operations independently:

```bash
# Array subscript operations (currently implemented)
./build/optiweave code.cpp --array-subscripts

# Arithmetic operations (infrastructure ready)
./build/optiweave code.cpp --arithmetic-ops

# Assignment operations (infrastructure ready)
./build/optiweave code.cpp --assignment-ops

# Comparison operations (infrastructure ready)
./build/optiweave code.cpp --comparison-ops

# Mix and match based on your needs
./build/optiweave code.cpp --array-subscripts --arithmetic-ops
```

### Array Access Instrumentation (Production Ready)

**What gets instrumented:**
- Array read operations: `x = arr[i]`
- Array write operations: `arr[i] = value` (simple assignments only)
- Multi-dimensional arrays: `matrix[i][j]`
- Nested array accesses

**Known limitations in C:**
- Compound assignments: `arr[i] += val` (C lacks references for lvalue return)
- Struct field assignments: `arr[i].field = val` (same limitation)
- Note: `std::vector::operator[]` in C++ uses operator overloading, not array subscripts

### Runtime Profiling

The instrumented code automatically tracks:
- **Total array accesses**: Count of all subscript operations
- **Hot functions**: Which functions perform the most array operations
- **Access distribution**: Percentage breakdown by function
- **Source locations**: File and line number for each access

Example output:
```
OptiWeave Runtime Statistics Report
Overall Statistics:
  Total array accesses: 104

Hot Functions (Top Array Access):
  1. parse_string: 65 accesses (62.5%)
  2. parse_number: 24 accesses (23.1%)
  3. print_string: 12 accesses (11.5%)
```

### Static Analysis

**Integer Overflow Detection**
```bash
./build/optiweave code.cpp --detect-overflow --overflow-format=json --
```
Detects signed overflow (UB), unsigned wraparound, mixed signedness, narrowing conversions.

**Floating-Point Precision Warnings**
```bash
./build/optiweave code.cpp --fp-precision-warnings --fp-precision-format=json --
```
Catches FP equality comparisons, catastrophic cancellation, precision loss in conversions.

**Memory Profiling**
```bash
./build/optiweave code.cpp --memory-profile --memory-profile-format=json --
```
Tracks new/delete operations, detects potential memory leaks and double-frees.

**Code Complexity Analysis**
```bash
./build/optiweave code.cpp --analyze-complexity --complexity-format=json
```
Measures cyclomatic complexity, cognitive complexity, and maintainability index.

**Data Flow Analysis**
```bash
./build/optiweave code.cpp --data-flow-analysis --data-flow-format=json --
```
Finds unused variables, uninitialized usage, write-only variables, and dead code.

**Call Graph Generation**
```bash
./build/optiweave code.cpp --call-graph --call-graph-format=html --
```
Visualizes function dependencies in DOT, JSON, or interactive HTML format.

**Dependency Graph**
```bash
./build/optiweave code.cpp --dependency-graph --dependency-graph-format=json --
```
Analyzes #include relationships and detects circular dependencies.

## Examples

### Basic Usage

```cpp
// test.cpp
#include <iostream>

int main() {
    int arr[1000];
    for (int i = 0; i < 1000; i++) {
        arr[i] = i * 2;
    }
    return arr[500];
}
```

```bash
# Transform and compile
./build/optiweave test.cpp --enable-stats --compile -o test

# Run with statistics
OPTIWEAVE_STATS=1 ./test
```

### Finding Performance Issues

```bash
# Compile with all analysis features
./build/optiweave code.cpp --arithmetic-ops --hotspots --enable-stats --compile -o program

# Run with complete profiling
OPTIWEAVE_STATS=1 \
OPTIWEAVE_PROFILE=1 \
OPTIWEAVE_HOTSPOTS=1 \
OPTIWEAVE_SUGGESTIONS=1 \
./program
```

### Static Analysis

```bash
# Check for multiple issues at once
./build/optiweave code.cpp \
  --detect-overflow \
  --fp-precision-warnings \
  --memory-profile \
  --data-flow-analysis \
  --
```

### Export Data

```bash
# Export all data to files
OPTIWEAVE_STATS=1 OPTIWEAVE_STATS_JSON=stats.json \
OPTIWEAVE_PROFILE=1 OPTIWEAVE_TIMING_JSON=profile.json \
OPTIWEAVE_HOTSPOTS=1 OPTIWEAVE_HOTSPOTS_JSON=hotspots.json \
./program
```

## CLI Reference

### Transformation Options
- `--array-subscripts` - **[IMPLEMENTED]** Transform array subscript operations
- `--arithmetic-ops` - **[TODO]** Transform arithmetic operators (+, -, *, /, %)
- `--assignment-ops` - **[TODO]** Transform assignments (=, +=, -=, etc.)
- `--comparison-ops` - **[TODO]** Transform comparisons (<, >, ==, !=, etc.)

*Note: Only `--array-subscripts` is fully implemented. Other operators have infrastructure ready but need instrumentation wrappers.*

### Runtime Features
- `--enable-stats` - Enable operation statistics
- `--enable-timing` - Enable timing profiling
- `--enable-profile` - Enable full profiling with percentiles
- `--hotspots` - Enable hotspot detection
- `--cache-profile` - Enable cache profiling (Linux)

### Static Analysis
- `--detect-overflow` - Detect integer overflow issues
- `--fp-precision-warnings` - Detect FP precision issues
- `--memory-profile` - Track memory allocations/deallocations
- `--data-flow-analysis` - Analyze variable usage
- `--analyze-complexity` - Measure code complexity
- `--call-graph` - Generate call graph
- `--dependency-graph` - Analyze file dependencies

### Output Options
- `--compile` - Automatically compile transformed code
- `-o <file>` - Output executable name (requires --compile)
- `--output-dir=<dir>` - Output directory for transformed files
- `--verbose` - Enable verbose output
- `--dry-run` - Preview transformations without writing

Most analysis features support `--*-format` (text/json) and `--*-output` (filename) options.

## Environment Variables

| Variable | Description |
|----------|-------------|
| `OPTIWEAVE_STATS=1` | Enable operation statistics |
| `OPTIWEAVE_TIMING=1` | Enable basic timing |
| `OPTIWEAVE_PROFILE=1` | Enable full profiling |
| `OPTIWEAVE_HOTSPOTS=1` | Enable hotspot detection |
| `OPTIWEAVE_TOP_N=<n>` | Number of hotspots to show (default: 10) |
| `OPTIWEAVE_SUGGESTIONS=1` | Enable optimization suggestions |
| `OPTIWEAVE_CACHE_PROFILE=1` | Enable cache profiling (Linux) |
| `OPTIWEAVE_STATS_CSV=<file>` | Export stats to CSV |
| `OPTIWEAVE_STATS_JSON=<file>` | Export stats to JSON |
| `OPTIWEAVE_TIMING_JSON=<file>` | Export timing to JSON |
| `OPTIWEAVE_HOTSPOTS_JSON=<file>` | Export hotspots to JSON |
| `OPTIWEAVE_CACHE_JSON=<file>` | Export cache data to JSON |

## Performance

OptiWeave achieves **low overhead** through selective instrumentation - tracking only array subscript operations rather than all operations.

### Runtime Overhead (Real-World Measurements)

| Library | Lines | Array Subscripts | Baseline | Instrumented | Overhead |
|---------|-------|------------------|----------|--------------|----------|
| cJSON   | 2,588 | 53              | 2.20 μs  | 2.57 μs      | **16.8%** |
| Simple C| 35    | 5               | -        | -            | **~10%** |

**Compared to other tools:**
- **OptiWeave**: 8-17% overhead
- **AddressSanitizer**: 200% overhead (measured)
- **Valgrind**: 2000-10000% overhead (literature)

**Why OptiWeave is fast:**
- **Selective instrumentation**: Only tracks array subscripts, not all operations
- **Static analysis guided**: Knows exactly what to instrument at compile time
- **Minimal function calls**: Inline wrappers with simple tracking
- **No heavy runtime**: Lightweight profiling without complex bookkeeping

**When to use:**
- ✅ Development and debugging
- ✅ Understanding array access patterns
- ✅ Profiling with acceptable overhead
- ❌ Production builds (compile without instrumentation)

## Build System Integration

### CMake
```cmake
find_program(OPTIWEAVE_EXECUTABLE optiweave)

add_custom_target(instrument_sources
    COMMAND ${OPTIWEAVE_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/*.cpp
    --output-dir=${CMAKE_BINARY_DIR}/instrumented
)
```

### Makefile
```makefile
OPTIWEAVE := ./build/optiweave
instrumented: $(SOURCES)
	$(OPTIWEAVE) $(SOURCES) --compile -o instrumented_program
```

## Testing

```bash
# Build with tests
./scripts/build.sh --tests

# Run all tests
cd build && ctest

# Run specific tests
ctest -R overflow
ctest -V  # Verbose output
```

## Web Dashboard

OptiWeave includes an interactive web dashboard for visualizing performance data:

```bash
# Generate JSON data
OPTIWEAVE_JSON_EXPORT=1 OPTIWEAVE_JSON_FILE=dashboard_data.json ./program

# View dashboard
cd web && python3 -m http.server 8000
# Open http://localhost:8000/dashboard_dynamic.html
```

See [web/README.md](web/README.md) for details.

## Project Status

**Phase 1: Core Instrumentation (100% Complete)** ✅
- AST transformation and runtime library
- Thread-safe operation counters
- Source location tracking
- Statistics, timing, and hotspot detection

**Phase 2: Code Understanding (100% Complete)** ✅
- Pattern detectors (division-in-loop, O(n²)/O(n³), memory access, vectorization)
- Complexity analysis and call graphs
- Dependency tracking and circular dependency detection
- Data flow analysis

**Phase 3: Advanced Analysis (44% Complete)**
- ✅ Memory profiling
- ✅ Cache profiling (Linux)
- ✅ Integer overflow detection
- ✅ Floating-point precision warnings
- 🔄 Additional pattern detectors (repeated computation, branch prediction)

## Contributing

Contributions welcome! Please ensure:
- LLVM 13-17 compatibility
- Tests for new features
- Code follows existing style
- Documentation updates

```bash
# Development build
./scripts/build.sh --tests --verbose
cd build && ctest
```

## License

MIT License - see [LICENSE](LICENSE) file.

## Acknowledgments

Built with [LLVM](https://llvm.org/) and [Clang](https://clang.llvm.org/).
