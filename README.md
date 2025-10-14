# OptiWeave

A C++ source-to-source compiler tool built on LLVM/Clang for automatic code instrumentation and static analysis.

[![LLVM Support](https://img.shields.io/badge/LLVM-13--17-blue.svg)](https://llvm.org/)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-red.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## What is OptiWeave?

OptiWeave transforms your C++ code to add instrumentation hooks, enabling runtime performance analysis and compile-time static analysis. It's useful for:

- **Performance profiling**: Find hotspots and bottlenecks in your code
- **Static analysis**: Detect bugs like integer overflow, FP precision issues, memory leaks
- **Code quality**: Measure complexity metrics and identify code smells
- **Understanding code**: Generate call graphs and dependency visualizations

## Quick Start

```bash
# Build OptiWeave
./scripts/build.sh

# Transform and compile your code
./build/optiweave your_code.cpp --compile -o program

# Run with profiling
OPTIWEAVE_STATS=1 OPTIWEAVE_HOTSPOTS=1 ./program
```

That's it! OptiWeave handles transformation, compilation, and linking automatically.

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

## Features

### Runtime Analysis

**Operation Statistics** - Track operation counts and throughput
```bash
./build/optiweave code.cpp --enable-stats --compile -o program
OPTIWEAVE_STATS=1 ./program
```

**Performance Profiling** - Measure execution time with nanosecond precision
```bash
./build/optiweave code.cpp --enable-timing --compile -o program
OPTIWEAVE_PROFILE=1 ./program
```

**Hotspot Detection** - Identify performance bottlenecks by source location
```bash
./build/optiweave code.cpp --hotspots --compile -o program
OPTIWEAVE_HOTSPOTS=1 OPTIWEAVE_TOP_N=20 ./program
```

**Cache Profiling** - Track L1/L2/L3 cache misses (Linux only)
```bash
./build/optiweave code.cpp --cache-profile --compile -o program
OPTIWEAVE_CACHE_PROFILE=1 ./program
```

**Optimization Suggestions** - Get actionable recommendations for speedups
```bash
OPTIWEAVE_SUGGESTIONS=1 ./program
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
- `--array-subscripts` - Transform array access (default: on)
- `--arithmetic-ops` - Transform arithmetic operators (+, -, *, /, %)
- `--assignment-ops` - Transform assignments (=, +=, -=, etc.)
- `--comparison-ops` - Transform comparisons (<, >, ==, !=, etc.)

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

OptiWeave is a **profiling and debugging tool** - overhead is expected and acceptable for development use.

### Runtime Overhead (measured on M1 Mac, -O3)

| Workload Type | Baseline | With Instrumentation | Overhead |
|---------------|----------|---------------------|----------|
| Array-heavy (10M ops) | 1.05 ms | 68 ms | +6,400% |
| Arithmetic-heavy | 0.005 ms | 0.006 ms | +22% |
| Mixed operations | 0.26 ms | 12 ms | +4,500% |

**Why the overhead is acceptable:**

- **This is a profiling tool** - you use it to find bottlenecks, not in production
- **Tracks every operation** - provides complete visibility into your code
- **Static analysis is free** - overflow/FP/memory detection has zero runtime cost
- **Arithmetic ops have low overhead** - only ~22% when not instrumenting array access
- **Use sampling for large workloads** - profile 1% of operations for 99% less overhead

**Typical workflow:**
1. Run with instrumentation to find hotspots (high overhead, but that's fine)
2. Identify bottlenecks from profiling data
3. Optimize those specific areas
4. Compile without instrumentation for production (zero overhead)

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
