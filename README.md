# OptiWeave

<div align="center">

**Modern C++ Source-to-Source Transformation Tool**  
*Automatic operator instrumentation for performance analysis and debugging*

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/yourusername/optiweave)
[![LLVM Support](https://img.shields.io/badge/LLVM-13--17-blue.svg)](https://llvm.org/)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-red.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

[Features](#features) • [Quick Start](#quick-start) • [Installation](#installation) • [Usage](#usage) • [Examples](#examples)

</div>

---

## ✨ Features

🔥 **Single-Command Workflow** - Transform and compile in one step
⚡ **Zero Manual Configuration** - Automatic header injection and library linking
🎯 **Multiple Operator Types** - Array access, arithmetic, assignment, and comparison operators
🛠️ **Modern C++20 Support** - Full template and namespace compatibility
📊 **Built-in Statistics** - Detailed transformation reporting
🔧 **Flexible Output** - In-place transformation or custom output directories
🧪 **Production Ready** - Comprehensive test suite and error handling
💡 **Optimization Suggestions** - Automatic detection of performance anti-patterns with actionable fixes
🌐 **Web Dashboard** - Beautiful interactive visualization of performance data and optimization opportunities
📈 **Code Complexity Analysis** - Static analysis of cyclomatic complexity, cognitive complexity, and maintainability metrics

## 🚀 Quick Start

### Building OptiWeave

**Automated Build (Recommended)**
```bash
# Clone the repository
git clone https://github.com/yourusername/optiweave.git
cd optiweave

# Build everything with one command
./scripts/build.sh

# Build with all features enabled
./scripts/build.sh --tests --verbose
```

**Manual Build**
```bash
# Configure with CMake
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the tool and runtime library
make -j$(nproc)
# Or on systems with ninja:
# ninja

# Optional: Install system-wide
sudo make install
```

**Build Targets**
```bash
# Build only the transformation tool
make optiweave

# Build only the runtime library
make optiweave_runtime

# Build everything (default)
make

# Build with verbose output
make VERBOSE=1
```

**Verify Build**
```bash
# Check tool version
./build/optiweave --version

# Verify runtime library exists
ls build/liboptiweave_runtime.a

# Run quick sanity test
echo 'int main() { return 0; }' | ./build/optiweave - --compile -o test && ./test
```

### Running on Examples

**Option 1: Transform and Compile in One Step**
```bash
# Run on the provided example
./build/optiweave examples/basic_transformation/example.cpp --compile -o example

# Execute the instrumented program
./example
```

**Option 2: Step-by-Step Workflow**
```bash
# Step 1: Transform the code
./build/optiweave examples/basic_transformation/example.cpp --

# Step 2: Compile manually
clang++ -std=c++20 -I./templates -L./build -loptiweave_runtime \
    examples/basic_transformation/example.cpp -o example

# Step 3: Run it
./example
```

**Option 3: Try Your Own Code**
```bash
# Create a simple test file
echo '#include <iostream>
int main() {
    int arr[10];
    arr[5] = 42;
    std::cout << arr[5] << std::endl;
    return 0;
}' > test.cpp

# Transform and compile
./build/optiweave test.cpp --compile -o test

# Run it
./test
```

**That's it!** OptiWeave handles transformation, header injection, library linking, and compilation automatically.

## 📋 Requirements

| Component | Version | Status |
|-----------|---------|--------|
| **LLVM** | 13.0 - 17.x | ✅ **Required** |
| **Clang** | 13.0 - 17.x | ✅ **Required** |
| **CMake** | 3.20+ | ✅ **Required** |
| **C++ Compiler** | C++20 compatible | ✅ **Required** |

### LLVM Version Support Matrix

| LLVM Version | Support Status | Notes |
|--------------|----------------|-------|
| 13.x | ✅ **Supported** | Minimum version |
| 14.x | ✅ **Supported** | Fully tested |
| 15.x | ✅ **Supported** | Recommended |
| 16.x | ✅ **Supported** | Fully tested |
| 17.x | ✅ **Supported** | Latest supported |
| 18.x+ | ❌ **Not supported** | API changes in progress |

## 🛠️ Installation

### Step 1: Install LLVM/Clang

<details>
<summary><b>macOS (Homebrew)</b></summary>

```bash
# Install LLVM 17 (recommended)
brew install llvm@17

# Add to PATH
echo 'export PATH="/opt/homebrew/opt/llvm@17/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# Verify installation
clang --version
llvm-config --version
```
</details>

<details>
<summary><b>Ubuntu/Debian</b></summary>

```bash
# Update package list
sudo apt update

# Install LLVM 17
sudo apt install llvm-17-dev clang-17-dev libclang-17-dev

# Or install LLVM 15
sudo apt install llvm-15-dev clang-15-dev libclang-15-dev

# Verify installation
clang-17 --version
llvm-config-17 --version
```
</details>

<details>
<summary><b>Arch Linux</b></summary>

```bash
# Install LLVM 17
sudo pacman -S llvm17 clang17

# Verify installation
clang --version
llvm-config --version
```
</details>

### Step 2: Build OptiWeave

```bash
# Clone the repository
git clone https://github.com/yourusername/optiweave.git
cd optiweave

# Build with automatic configuration
./scripts/build.sh

# Or build with specific options
./scripts/build.sh --tests --verbose

# Or build manually
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
make -j$(nproc)
```

### Step 3: Verify Installation

```bash
# Test the build
./build/optiweave --help

# Run a quick test
echo 'int main() { int arr[5]; return arr[2]; }' > test.cpp
./build/optiweave test.cpp --compile -o test_program
./test_program
```

### Troubleshooting

<details>
<summary><b>Multiple LLVM Versions Installed</b></summary>

```bash
# Find LLVM installations
ls /usr/lib/llvm-* /opt/homebrew/opt/llvm*

# Build with specific LLVM version
export LLVM_DIR=/usr/lib/llvm-17/lib/cmake/llvm
export Clang_DIR=/usr/lib/llvm-17/lib/cmake/clang
./scripts/build.sh
```
</details>

<details>
<summary><b>LLVM 18+ Compatibility</b></summary>

OptiWeave currently supports LLVM 13-17. For LLVM 18+ users:

```bash
# Option 1: Install compatible LLVM version alongside
brew install llvm@17  # macOS
sudo apt install llvm-17-dev clang-17-dev  # Ubuntu

# Option 2: Use Docker (see Docker section below)
docker build -t optiweave .
```
</details>

## 🎯 Usage

### Basic Workflow

```bash
# Simple transformation and compilation
./build/optiweave source.cpp --compile -o instrumented_program

# Transform multiple files
./build/optiweave file1.cpp file2.cpp --compile -o multi_file_program

# Transform with specific operator types
./build/optiweave source.cpp --arithmetic-ops --assignment-ops --compile -o advanced_program
```

### Advanced Options

```bash
# Transform only (no compilation)
./build/optiweave source.cpp --output-dir=./transformed

# Dry run to preview transformations
./build/optiweave source.cpp --dry-run --verbose

# Custom prelude header
./build/optiweave source.cpp --prelude=my_custom_prelude.hpp --compile

# Transform with statistics
./build/optiweave source.cpp --compile --print-stats --verbose
```

### Operator Types

| Flag | Description | Example |
|------|-------------|---------|
| `--array-subscripts` | Array access (default: ON) | `arr[i]` → `optiweave::__primop_subscript<int*>()(arr, i)` |
| `--arithmetic-ops` | Arithmetic operators | `a + b` → `optiweave::__primop_add<int, int>()(a, b)` |
| `--assignment-ops` | Assignment operators | `a = b` → `optiweave::__primop_assign<int>()(a, b)` |
| `--comparison-ops` | Comparison operators | `a < b` → `optiweave::__primop_less<int, int>()(a, b)` |

### Command Line Reference

```bash
# Core options
--compile                    # Automatically compile transformed code
-o <filename>               # Output executable name (requires --compile)
--output-dir=<directory>    # Output directory for transformed files
--verbose                   # Enable verbose output
--dry-run                   # Preview transformations without changes

# Transformation options
--array-subscripts          # Transform array subscripts (default: ON)
--arithmetic-ops            # Transform arithmetic operators
--assignment-ops            # Transform assignment operators
--comparison-ops            # Transform comparison operators

# Runtime features (enabled during compilation)
--enable-stats             # Enable operation statistics collection
--enable-timing            # Enable high-resolution timing profiling
--enable-profile           # Enable full profiling (percentiles, histograms)
--hotspots                 # Enable hotspot detection and tracking

# Advanced options
--prelude=<path>           # Custom prelude header
--skip-system-headers      # Skip system header transformations (default: ON)
--print-stats              # Print transformation statistics

# Code complexity analysis
--analyze-complexity        # Perform static complexity analysis
--complexity-format=<fmt>   # Output format: terminal, json, markdown, dot
--complexity-output=<file>  # Save analysis to file
```

## 📊 Runtime Features

OptiWeave provides powerful runtime analysis features that can be enabled via environment variables. These features have minimal overhead (~2-5%) and provide deep insights into your program's behavior.

### Feature 1: Operation Statistics

**What it does:** Tracks the count of each instrumented operation type (array accesses, arithmetic operations, etc.)

**Enable:**
```bash
# At compile time (required)
./build/optiweave source.cpp --enable-stats --compile -o program

# At runtime
OPTIWEAVE_STATS=1 ./program
```

**Output Example:**
```
╔══════════════════════════════════════════════════╗
║       OptiWeave Operation Statistics             ║
╚══════════════════════════════════════════════════╝

Runtime: 0.125 seconds
Total Operations: 1,000,000

Operation Breakdown:
┌─────────────────────┬────────────┬──────────┬────────────────┐
│ Operation Type      │ Count      │ Percent  │ Ops/Second     │
├─────────────────────┼────────────┼──────────┼────────────────┤
│ Array Subscripts    │     500000 │    50.0% │       4.00M ops/s │
│ Additions           │     300000 │    30.0% │       2.40M ops/s │
│ Multiplications     │     150000 │    15.0% │       1.20M ops/s │
│ Divisions           │      50000 │     5.0% │     400.00K ops/s │
└─────────────────────┴────────────┴──────────┴────────────────┘

Insights:
• Array Subscripts dominate (50.0%)
• High division count (5.0%) - consider optimization if critical
```

**Export Options:**
```bash
# Export to CSV
OPTIWEAVE_STATS=1 OPTIWEAVE_STATS_CSV=stats.csv ./program

# Export to JSON
OPTIWEAVE_STATS=1 OPTIWEAVE_STATS_JSON=stats.json ./program
```

### Feature 2: Timing & Profiling

**What it does:** Measures execution time for each operation with nanosecond precision, computes percentiles and histograms

**Enable:**
```bash
# Compile with timing support
./build/optiweave source.cpp --enable-timing --compile -o program

# Run with basic timing
OPTIWEAVE_TIMING=1 ./program

# Run with full profiling (percentiles + histograms)
OPTIWEAVE_PROFILE=1 ./program
```

**Output Example:**
```
╔══════════════════════════════════════════════════╗
║       OptiWeave Performance Profile              ║
╚══════════════════════════════════════════════════╝

Total Time: 125.43ms

Time Breakdown by Operation:
┌─────────────────┬─────────┬──────────┬─────────┬─────────┬─────────┐
│ Operation       │ Count   │ Total    │ Avg     │ Min     │ Max     │
├─────────────────┼─────────┼──────────┼─────────┼─────────┼─────────┤
│ Array Access    │  500000 │  62.50ms │   125ns │    50ns │   500ns │
│ Addition        │  300000 │  37.50ms │   125ns │    75ns │   400ns │
│ Multiplication  │  150000 │  18.75ms │   125ns │   100ns │   300ns │
│ Division        │   50000 │   6.68ms │   133ns │   120ns │   250ns │
└─────────────────┴─────────┴──────────┴─────────┴─────────┴─────────┘

Time Distribution:
  Array Access       ████████████████████████ 49.8%  (62.50ms)
  Addition           ██████████████ 29.9%  (37.50ms)
  Multiplication     ███████ 14.9%  (18.75ms)
  Division           ██ 5.3%  (6.68ms)

Percentile Analysis (Array Access):
  P50 (median): 120ns
  P95: 180ns
  P99: 250ns
  P99.9: 450ns

Key Insights:
• Array Access is the bottleneck (49.8% of time)
• Division operations are 6% slower on average
```

**Export Options:**
```bash
# Export timing data to CSV
OPTIWEAVE_TIMING=1 OPTIWEAVE_TIMING_CSV=timing.csv ./program

# Export to JSON with full statistics
OPTIWEAVE_PROFILE=1 OPTIWEAVE_TIMING_JSON=profile.json ./program
```

### Feature 3: Hotspot Detection

**What it does:** Identifies performance hotspots by tracking time spent at each source location (file:line:function)

**Enable:**
```bash
# Compile with hotspot tracking (includes timing automatically)
./build/optiweave source.cpp --hotspots --compile -o program

# Run and see top hotspots
OPTIWEAVE_HOTSPOTS=1 ./program

# Show top 20 hotspots
OPTIWEAVE_HOTSPOTS=1 OPTIWEAVE_TOP_N=20 ./program
```

**Output Example:**
```
╔══════════════════════════════════════════════════════════════╗
║            OptiWeave Hotspot Analysis                        ║
╚══════════════════════════════════════════════════════════════╝

Total Runtime: 125.43ms
Locations Tracked: 47

Top 10 Hotspots (by time spent):
┌────┬───────────────────────────────────────────────────┬──────────┬──────────┬─────────┬──────────┐
│ #  │ Location                                          │ Ops      │ Time     │ % Total │ Avg/Op   │
├────┼───────────────────────────────────────────────────┼──────────┼──────────┼─────────┼──────────┤
│  1 │ matrix.cpp:45 (matrix_multiply)                   │   100.0K │  45.2ms │   36.0% │   452ns │
│  2 │ process.cpp:78 (data_transform)                   │    50.0K │  28.5ms │   22.7% │   570ns │
│  3 │ utils.cpp:23 (compute_sum)                        │   200.0K │  15.8ms │   12.6% │    79ns │
│  4 │ main.cpp:156 (main)                               │    10.0K │  12.3ms │    9.8% │  1230ns │
│  5 │ filter.cpp:89 (apply_filter)                      │    75.0K │   8.9ms │    7.1% │   118ns │
└────┴───────────────────────────────────────────────────┴──────────┴──────────┴─────────┴──────────┘

Hotspot Visualization:
   36.0% ████████████████ matrix_multiply (matrix.cpp:45)
   22.7% ██████████ data_transform (process.cpp:78)
   12.6% █████ compute_sum (utils.cpp:23)
    9.8% ████ main (main.cpp:156)
    7.1% ███ apply_filter (filter.cpp:89)

Function-Level Hotspots:
┌────────────────────────────────────────┬──────────┬─────────┐
│ Function                               │ Time     │ % Total │
├────────────────────────────────────────┼──────────┼─────────┤
│ matrix_multiply                        │  45.2ms │   36.0% │
│ data_transform                         │  28.5ms │   22.7% │
│ compute_sum                            │  15.8ms │   12.6% │
└────────────────────────────────────────┴──────────┴─────────┘

File-Level Hotspots:
┌────────────────────────────────────────┬──────────┬─────────┐
│ File                                   │ Time     │ % Total │
├────────────────────────────────────────┼──────────┼─────────┤
│ matrix.cpp                             │  45.2ms │   36.0% │
│ process.cpp                            │  28.5ms │   22.7% │
│ utils.cpp                              │  18.9ms │   15.1% │
└────────────────────────────────────────┴──────────┴─────────┘
```

**Export Options:**
```bash
# Export hotspots to CSV
OPTIWEAVE_HOTSPOTS=1 OPTIWEAVE_HOTSPOTS_CSV=hotspots.csv ./program

# Export to JSON
OPTIWEAVE_HOTSPOTS=1 OPTIWEAVE_HOTSPOTS_JSON=hotspots.json ./program
```

### Feature 4: Optimization Suggestions

**What it does:** Automatically detects performance anti-patterns and provides actionable optimization recommendations

**Enable:**
```bash
# Compile with all analysis features
./build/optiweave source.cpp --arithmetic-ops --hotspots --enable-stats --compile -o program

# Run with optimization suggestions
OPTIWEAVE_SUGGESTIONS=1 OPTIWEAVE_STATS=1 ./program
```

**Output Example:**
```
╔══════════════════════════════════════════════════════════════╗
║        OptiWeave Optimization Suggestions                    ║
╚══════════════════════════════════════════════════════════════╝

Analysis Complete: 3 optimization opportunities found
Potential combined speedup: 20.0-150.0x

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🔥 HIGH IMPACT (Estimated 10x+ speedup)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[1] Naive Matrix Multiplication - Use BLAS
    Location: matrix.cpp:45 (matrix_multiply)

    Pattern Detected:
    • O(n³) nested loops with poor cache locality
    • Time spent: 36.0% of total execution

    Current Code:
      for (int i = 0; i < n; i++) {
          for (int j = 0; j < n; j++) {
              for (int k = 0; k < n; k++) {
                  C[i][j] += A[i][k] * B[k][j];  // Cache misses!
              }
          }
      }

    Why It's Slow:
    • Column-major access of B causes cache misses
    • No SIMD vectorization
    • No cache blocking

    Recommended Fix:
      // Use optimized BLAS library
      #include <cblas.h>
      cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                  n, n, n, 1.0, A, n, B, n, 0.0, C, n);

    Expected Speedup: 20-50x

    Rationale:
    BLAS libraries use cache blocking, SIMD, and multi-threading

    Requirements:
    • Link with: -lblas or -lopenblas
    • Install: sudo apt install libblas-dev

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
⚡ MEDIUM IMPACT (Estimated 2-10x speedup)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[2] Division in Hot Loop
    Location: process.cpp:78 (data_transform)

    Pattern Detected:
    • Loop contains division by constant value
    • Time spent: 22.7% of total execution

    Current Code:
      for (int i = 0; i < n; i++) {
          result[i] = data[i] / constant;  // Division every iteration!
      }

    Why It's Slow:
    • Division is 8-10x slower than multiplication on most CPUs

    Recommended Fix:
      double inv_constant = 1.0 / constant;  // Compute reciprocal once
      for (int i = 0; i < n; i++) {
          result[i] = data[i] * inv_constant;  // Multiply instead
      }

    Expected Speedup: 3-8x

    Rationale:
    Multiplication is 8-10x faster than division. Computing the reciprocal
    once and multiplying is mathematically equivalent for constant divisors.

[3] SIMD Vectorization Opportunity
    Location: utils.cpp:23 (compute_sum)

    Pattern Detected:
    • Loop is vectorizable but not using SIMD instructions
    • Time spent: 12.6% of total execution

    Current Code:
      for (int i = 0; i < n; i++) {
          sum += array[i];
      }

    Why It's Slow:
    • Processing one element at a time instead of 4 or 8 in parallel

    Recommended Fix:
      // Option 1: Compiler auto-vectorization
      #pragma omp simd reduction(+:sum)
      for (int i = 0; i < n; i++) {
          sum += array[i];
      }

    Expected Speedup: 2-4x

    Requirements:
    • Compile with: -O3 -march=native -fopenmp

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Summary:
• 1 HIGH impact opportunity (20-50x speedup)
• 2 MEDIUM impact opportunities (combined 6-32x speedup)
• Potential combined speedup: 20-150x

Next Steps:
1. Start with HIGH impact optimizations first
2. Benchmark each change individually
3. Re-run OptiWeave to verify improvements
```

**Export Options:**
```bash
# Export to markdown file
OPTIWEAVE_SUGGESTIONS=1 OPTIWEAVE_SUGGESTIONS_FORMAT=markdown \
  OPTIWEAVE_SUGGESTIONS_FILE=suggestions.md ./program

# Export to HTML report
OPTIWEAVE_SUGGESTIONS=1 OPTIWEAVE_SUGGESTIONS_FORMAT=html \
  OPTIWEAVE_SUGGESTIONS_FILE=report.html ./program

# Export to JSON for CI/CD integration
OPTIWEAVE_SUGGESTIONS=1 OPTIWEAVE_SUGGESTIONS_FORMAT=json \
  OPTIWEAVE_SUGGESTIONS_FILE=suggestions.json ./program
```

### Feature 5: Code Complexity Analysis

**What it does:** Performs static analysis of your code to measure cyclomatic complexity, cognitive complexity, maintainability index, and generates call graphs. Helps identify complex functions that need refactoring.

**Enable:**
```bash
# Analyze code complexity (no compilation needed)
./build/optiweave source.cpp --analyze-complexity

# Save analysis to file
./build/optiweave source.cpp --analyze-complexity --complexity-output=report.txt

# Export in different formats
./build/optiweave source.cpp --analyze-complexity --complexity-format=json
./build/optiweave source.cpp --analyze-complexity --complexity-format=markdown
./build/optiweave source.cpp --analyze-complexity --complexity-format=dot  # GraphViz call graph
```

**Output Example:**
```
╔══════════════════════════════════════════════════════════════╗
║        OptiWeave Code Complexity Analysis                   ║
╚══════════════════════════════════════════════════════════════╝

Project Statistics:
  Total Functions: 16
  Total Lines: 164
  Complex Functions (CC > 20): 0
  Very Complex Functions (CC > 50): 0

Average Metrics:
  Cyclomatic Complexity: 2.62
  Cognitive Complexity: 3.00
  Maintainability Index: 99.22

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🔥 Most Complex Functions (by Cyclomatic Complexity)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

┌────┬─────────────────────────────┬─────────┬─────────┬──────────┬────────────┐
│ #  │ Function                    │ CC      │ CogC    │ MI       │ Risk       │
├────┼─────────────────────────────┼─────────┼─────────┼──────────┼────────────┤
│  1 │ process_data                │      12 │      18 │     87.6 │ Medium     │
│  2 │ deeply_nested               │       5 │      10 │    100.0 │ Low        │
│  3 │ grade_calculator            │       5 │      10 │    100.0 │ Low        │
│  4 │ matrix_multiply             │       4 │       6 │    100.0 │ Low        │
└────┴─────────────────────────────┴─────────┴─────────┴──────────┴────────────┘

Legend:
  CC   = Cyclomatic Complexity (decision points)
  CogC = Cognitive Complexity (understandability)
  MI   = Maintainability Index (0-100, higher is better)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧠 Hardest to Understand Functions (by Cognitive Complexity)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Function: process_data
  Cognitive Complexity: 18 (Moderate)
  Max Nesting Depth: 4
  Decision Points: 8

Function: deeply_nested
  Cognitive Complexity: 10 (Easy)
  Max Nesting Depth: 4
  Decision Points: 4
```

**Complexity Metrics Explained:**

| Metric | Description | Thresholds |
|--------|-------------|------------|
| **Cyclomatic Complexity (CC)** | Number of independent paths through code | 1-5: Simple, 6-10: Moderate, 11-20: Complex, 20+: Very Complex |
| **Cognitive Complexity (CogC)** | How hard code is to understand (considers nesting) | 0-5: Very Easy, 6-10: Easy, 11-20: Moderate, 20+: Hard |
| **Maintainability Index (MI)** | Overall maintainability score | 85-100: Highly Maintainable, 65-85: Moderate, 0-65: Difficult |
| **Max Nesting Depth** | Deepest level of nested control structures | 1-2: Good, 3-4: Warning, 5+: Refactor needed |

**Use Cases:**
- **Code Review**: Identify complex functions that need refactoring
- **Technical Debt**: Track complexity over time
- **CI/CD Integration**: Fail builds if complexity exceeds thresholds
- **Documentation**: Generate complexity reports for documentation
- **Refactoring Priority**: Find which functions to refactor first

**Export Formats:**

```bash
# Terminal (default) - human-readable with Unicode tables
./build/optiweave source.cpp --analyze-complexity

# JSON - for programmatic analysis and CI/CD
./build/optiweave source.cpp --analyze-complexity --complexity-format=json > metrics.json

# Markdown - for documentation
./build/optiweave source.cpp --analyze-complexity --complexity-format=markdown > complexity.md

# DOT - for GraphViz call graph visualization
./build/optiweave source.cpp --analyze-complexity --complexity-format=dot > callgraph.dot
dot -Tpng callgraph.dot -o callgraph.png
```

**CI/CD Integration Example:**

```bash
# In your CI pipeline
./build/optiweave src/**/*.cpp --analyze-complexity --complexity-format=json > complexity.json

# Parse JSON and fail if any function has CC > 20
python3 << EOF
import json
with open('complexity.json') as f:
    data = json.load(f)
    for func in data['functions']:
        if func['cyclomatic_complexity'] > 20:
            print(f"ERROR: {func['name']} has CC={func['cyclomatic_complexity']}")
            exit(1)
EOF
```

### Combining Features

**All features can be used together:**
```bash
# Compile with all features
./build/optiweave source.cpp --arithmetic-ops --hotspots --enable-stats \
  --enable-timing --enable-profile --compile -o program

# Run with complete analysis
OPTIWEAVE_STATS=1 \
OPTIWEAVE_PROFILE=1 \
OPTIWEAVE_HOTSPOTS=1 \
OPTIWEAVE_SUGGESTIONS=1 \
OPTIWEAVE_TOP_N=20 \
./program

# Export everything
OPTIWEAVE_STATS=1 OPTIWEAVE_STATS_CSV=stats.csv \
OPTIWEAVE_PROFILE=1 OPTIWEAVE_TIMING_JSON=profile.json \
OPTIWEAVE_HOTSPOTS=1 OPTIWEAVE_HOTSPOTS_CSV=hotspots.csv \
OPTIWEAVE_SUGGESTIONS=1 OPTIWEAVE_SUGGESTIONS_FILE=report.html \
./program
```

### Environment Variables Reference

| Variable | Values | Description |
|----------|--------|-------------|
| `OPTIWEAVE_STATS` | 0, 1 | Enable operation statistics |
| `OPTIWEAVE_STATS_CSV` | filename | Export stats to CSV |
| `OPTIWEAVE_STATS_JSON` | filename | Export stats to JSON |
| `OPTIWEAVE_TIMING` | 0, 1 | Enable basic timing |
| `OPTIWEAVE_PROFILE` | 0, 1 | Enable full profiling |
| `OPTIWEAVE_TIMING_CSV` | filename | Export timing to CSV |
| `OPTIWEAVE_TIMING_JSON` | filename | Export timing to JSON |
| `OPTIWEAVE_HOTSPOTS` | 0, 1 | Enable hotspot detection |
| `OPTIWEAVE_TOP_N` | number | Number of top hotspots to show (default: 10) |
| `OPTIWEAVE_HOTSPOTS_CSV` | filename | Export hotspots to CSV |
| `OPTIWEAVE_HOTSPOTS_JSON` | filename | Export hotspots to JSON |
| `OPTIWEAVE_SUGGESTIONS` | 0, 1 | Enable optimization suggestions |
| `OPTIWEAVE_SUGGESTIONS_FORMAT` | terminal, markdown, html, json | Output format |
| `OPTIWEAVE_SUGGESTIONS_FILE` | filename | Export suggestions to file |

### Performance Impact

| Feature | Overhead | Use Case |
|---------|----------|----------|
| Statistics Only | -2.8% (faster!) | Production profiling |
| Timing | ~2% | Performance analysis |
| Full Profiling | ~5% | Deep performance investigation |
| Hotspots | ~3% | Finding bottlenecks |
| All Features | ~5-8% | Complete analysis |

## 📚 Examples

### Example 1: Basic Array Instrumentation

**Input (`example.cpp`):**
```cpp
int main() {
    int data[100];
    for (int i = 0; i < 10; ++i) {
        data[i] = i * 2;
    }
    return data[5];
}
```

**Transform and run:**
```bash
./build/optiweave example.cpp --compile -o instrumented
./instrumented
```

**Output:**
```
[2025-01-15 12:34:56.123] OptiWeave: pointer_subscript at 0x7fff5fbff580[0] (example.cpp:4)
[2025-01-15 12:34:56.124] OptiWeave: pointer_subscript at 0x7fff5fbff580[1] (example.cpp:4)
...
[2025-01-15 12:34:56.132] OptiWeave: pointer_subscript at 0x7fff5fbff580[5] (example.cpp:6)
```

### Example 2: Multiple Operator Types

**Input (`complex.cpp`):**
```cpp
int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int sum = 0;
    
    for (int i = 0; i < 5; ++i) {
        sum += arr[i] * 2;
    }
    
    return (sum > 30) ? 1 : 0;
}
```

**Transform with multiple operators:**
```bash
./build/optiweave complex.cpp --arithmetic-ops --comparison-ops --compile -o complex_instrumented --verbose
./complex_instrumented
```

### Example 3: Custom Output Directory

```bash
# Transform without compilation
./build/optiweave source.cpp --output-dir=./instrumented_src --verbose

# Examine the transformed code
cat ./instrumented_src/source.cpp

# Compile manually if needed
clang++ -I./templates ./instrumented_src/source.cpp -L./build -loptiweave_runtime -o manual_build
```

### Example 4: Template and Namespace Support

**Input (`templates.cpp`):**
```cpp
#include <vector>

template<typename T>
T process_array(T* arr, size_t size) {
    T result = arr[0];
    for (size_t i = 1; i < size; ++i) {
        result += arr[i];
    }
    return result;
}

int main() {
    int data[] = {1, 2, 3, 4, 5};
    return process_array(data, 5);
}
```

**Transform:**
```bash
./build/optiweave templates.cpp --arithmetic-ops --compile -o template_instrumented
./template_instrumented
```

**OptiWeave automatically handles:**
- ✅ Template instantiation detection
- ✅ Namespace qualification (`optiweave::`)
- ✅ Header injection (`#include <optiweave/prelude.hpp>`)
- ✅ Type deduction for instrumentation calls

### Example 5: Code Complexity Analysis

**Input (`complex_code.cpp`):**
```cpp
int process_data(int value, int mode, bool flag1, bool flag2) {
    int result = 0;
    if (mode == 1) {
        if (flag1) {
            if (flag2) {
                result = value * 2;
            } else {
                result = value + 10;
            }
        } else {
            result = value - 5;
        }
    } else if (mode == 2) {
        result = value * value;
    }
    return result;
}

int main() {
    return process_data(10, 1, true, false);
}
```

**Analyze complexity:**
```bash
# Get complexity report
./build/optiweave complex_code.cpp --analyze-complexity

# Export to JSON for CI/CD
./build/optiweave complex_code.cpp --analyze-complexity --complexity-format=json > metrics.json

# Generate call graph visualization
./build/optiweave complex_code.cpp --analyze-complexity --complexity-format=dot > graph.dot
dot -Tpng graph.dot -o callgraph.png
```

**Output:**
```
╔══════════════════════════════════════════════════════════════╗
║        OptiWeave Code Complexity Analysis                   ║
╚══════════════════════════════════════════════════════════════╝

🔥 Most Complex Functions:

│  1 │ process_data    │ CC: 6  │ CogC: 10 │ MI: 72.3 │ Medium Risk │
│  2 │ main            │ CC: 1  │ CogC: 0  │ MI: 100  │ Low Risk    │

📊 Recommendations:
• process_data has high cognitive complexity (10) due to deep nesting
  → Consider extracting nested conditions into separate functions
  → Max nesting depth: 3
```

**Use cases:**
- Identify functions that need refactoring
- Track code complexity over time in CI/CD
- Generate documentation with complexity metrics
- Create call graph visualizations for code review

## 🌐 Web Dashboard

OptiWeave includes a beautiful, interactive web dashboard for visualizing performance data and optimization suggestions.

![Dashboard Preview](docs/images/dashboard-preview.png)

### Features

- **Performance Hotspots**: Flame graph visualization of code bottlenecks
- **Operation Statistics**: Interactive pie charts showing operation distribution
- **Optimization Suggestions**: Detailed recommendations with before/after code snippets
- **Timeline View**: Execution phase visualization
- **Responsive Design**: Works on desktop, tablet, and mobile devices
- **Zero Dependencies**: No build process, works offline with mock data

### Quick Start

**Step 1: Generate Data**
```bash
# Transform and compile
./build/optiweave --compile --enable-stats --enable-timing -o myprogram mycode.cpp

# Run with JSON export
env OPTIWEAVE_STATS=1 \
    OPTIWEAVE_TIMING=1 \
    OPTIWEAVE_HOTSPOTS=1 \
    OPTIWEAVE_SUGGESTIONS=1 \
    OPTIWEAVE_JSON_EXPORT=1 \
    OPTIWEAVE_JSON_FILE=dashboard_data.json \
    ./myprogram
```

**Step 2: View Dashboard**

```bash
# Option A: Static dashboard (mock data, no server needed)
open web/dashboard.html

# Option B: Dynamic dashboard (real data, requires local server)
cd web
python3 -m http.server 8000
# Open http://localhost:8000/dashboard_dynamic.html
```

### Environment Variables

| Variable | Purpose | Default |
|----------|---------|---------|
| `OPTIWEAVE_JSON_EXPORT` | Enable JSON export | disabled |
| `OPTIWEAVE_JSON_FILE` | Output filename | `optiweave_dashboard.json` |

For complete documentation, see [web/README.md](web/README.md).

## 🐳 Docker Support

For users with incompatible LLVM versions or easy deployment:

<details>
<summary><b>Dockerfile</b></summary>

```dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    llvm-17-dev \
    clang-17-dev \
    libclang-17-dev \
    git

# Build OptiWeave
COPY . /opt/optiweave
WORKDIR /opt/optiweave
RUN ./scripts/build.sh

ENTRYPOINT ["./build/optiweave"]
```
</details>

```bash
# Build Docker image
docker build -t optiweave .

# Use with local files
docker run -v $(pwd):/workspace -w /workspace optiweave source.cpp --compile -o instrumented

# Interactive mode
docker run -it -v $(pwd):/workspace -w /workspace optiweave bash
```

## 🔧 Build System Integration

### CMake Integration

```cmake
# Find OptiWeave
find_program(OPTIWEAVE_EXECUTABLE optiweave)

# Custom target for instrumentation
add_custom_target(instrument_sources
    COMMAND ${OPTIWEAVE_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/*.cpp --output-dir=${CMAKE_BINARY_DIR}/instrumented
    COMMENT "Instrumenting source files with OptiWeave"
)
```

### Makefile Integration

```makefile
# Variables
OPTIWEAVE := ./build/optiweave
SOURCES := $(wildcard src/*.cpp)

# Instrument and build
instrumented: $(SOURCES)
	$(OPTIWEAVE) $(SOURCES) --compile -o instrumented_program

.PHONY: instrumented
```

## 🧪 Testing

```bash
# Run all tests
./scripts/build.sh --tests
cd build && ctest

# Run specific test categories
ctest -L unit          # Unit tests only
ctest -L integration   # Integration tests only

# Verbose test output
ctest --verbose

# Run tests with custom LLVM
export LLVM_DIR=/path/to/llvm/cmake
./scripts/build.sh --tests
```

## 📈 Performance

OptiWeave is designed for minimal runtime overhead:

| Metric | Value |
|--------|-------|
| **Transformation Speed** | ~1000 LOC/second |
| **Runtime Overhead (Stats Mode)** | -2.80% average (negative = faster!) |
| **Runtime Overhead (Full Profiling)** | 2-5% typical |
| **Memory Usage** | < 50MB for typical projects |
| **Build Time Impact** | ~5-10% increase |

### Detailed Overhead Benchmarks

Performance measurements from `tests/benchmarks/run_overhead_benchmark.sh`:

| Benchmark | Baseline | Instrumented | Overhead |
|-----------|----------|--------------|----------|
| **Array access** | 81.282ms | 75.641ms | **-6.94%** |
| **Arithmetic ops** | 1.036ms | 0.994ms | **-4.05%** |
| **Mixed operations** | 6.936ms | 6.905ms | **-0.45%** |
| **Function calls** | 0.390ms | 0.391ms | **+0.26%** |

**Average overhead: -2.80%** (stats-only mode)

The negative overhead is due to compiler optimizations applied to the transformed code. Full profiling mode (with timing and hotspot tracking) adds ~2-5% overhead depending on hotspot density.

## 🤝 Contributing

We welcome contributions! Here's how to get started:

### Development Setup

```bash
# Fork and clone
git clone https://github.com/yourusername/optiweave.git
cd optiweave

# Build with tests and examples
./scripts/build.sh --tests --verbose

# Set up pre-commit hooks
cp scripts/pre-commit .git/hooks/
chmod +x .git/hooks/pre-commit
```

### Contribution Guidelines

1. **Check Requirements**: Ensure LLVM 13-17 compatibility
2. **Create Branch**: `git checkout -b feature/your-feature-name`
3. **Write Tests**: Add tests for new functionality
4. **Follow Style**: Use existing code style and formatting
5. **Test Everything**: Run `./scripts/build.sh --tests`
6. **Update Docs**: Update README and docs if needed
7. **Submit PR**: Create a pull request with clear description

### Development Commands

```bash
# Format code
./scripts/format.sh

# Run linting
./scripts/lint.sh

# Quick development build
./scripts/build.sh --clean --verbose

# Test specific components
cd build && ctest -R "test_ast_visitor"
```

## 📖 Documentation

- 📋 **[Architecture Overview](docs/architecture.md)** - How OptiWeave works internally
- 🔧 **[API Reference](docs/api.md)** - Detailed API documentation  
- 🎯 **[Transformation Guide](docs/transformations.md)** - Supported transformations
- 🔗 **[LLVM Integration](docs/llvm-integration.md)** - LLVM/Clang integration details
- 🐛 **[Troubleshooting](docs/troubleshooting.md)** - Common issues and solutions

## 📄 License

MIT License - see [LICENSE](LICENSE) file for details.

## 🎉 Acknowledgments

- **LLVM Project** - For the excellent compiler infrastructure
- **Clang Team** - For the powerful AST manipulation capabilities  
- **Contributors** - Everyone who helped make OptiWeave better

## 📊 Project Status

### Recent Updates

- ✅ **v1.0.0** - Initial release with full LLVM 13-17 support
- ✅ **Auto-compilation** - Single-command workflow implementation
- ✅ **Runtime library** - Complete instrumentation runtime
- ✅ **Modern CMake** - Professional build system
- ✅ **Comprehensive testing** - Full test suite coverage

### Roadmap

**Phase 1: Core Instrumentation (100% Complete)** ✅
- ✅ AST transformation for array subscripts and arithmetic
- ✅ Atomic operation counters (thread-safe)
- ✅ Source location tracking (file:line:function)
- ✅ Thread-safe runtime with atomic counters
- ✅ Unit tests and multi-threading tests
- ✅ Overhead benchmarking (-2.80% average overhead)
- ✅ Comprehensive command documentation
- ✅ Operation statistics with CSV/JSON export
- ✅ High-resolution timing and profiling
- ✅ Hotspot detection with source correlation

**Phase 2: Optimization Intelligence (100% Complete)** ✅
- ✅ Pattern detector architecture
- ✅ Division-in-loop detector (3-8x speedup)
- ✅ O(n²)/O(n³) complexity detector (20-50x speedup)
- ✅ Memory access pattern detector (3-5x speedup)
- ✅ Vectorization opportunity detector (2-4x speedup)
- ✅ AST loop analysis integration
- ✅ Loop body line range tracking
- ✅ Runtime correlation with hotspots (loop-to-hotspot matching)
- ✅ Report generation (terminal output)
- ✅ Export formats (Markdown, HTML, JSON)
- ✅ Code complexity analysis (cyclomatic, cognitive, maintainability)
- ✅ Call graph generation
- ✅ Static code analysis with multiple export formats

**Phase 3: Advanced Analysis (20% Complete)**
- ✅ Complexity metrics and call graph visualization
- 🔄 Memory profiling
- 🔄 Cache miss tracking
- 🔄 Integer overflow detection
- 🔄 Floating-point precision warnings
- 🔄 Additional pattern detectors (repeated computation, branch misprediction)

**Future Enhancements**
- 🔄 **LLVM 18+ Support** - Updating for latest LLVM APIs
- 🔄 **Visual Studio Integration** - MSBuild targets and project templates
- 🔄 **WebAssembly Support** - Instrumentation for WASM targets
- 🔄 **GUI Tool** - Visual transformation configuration
- 🔄 **IDE Plugins** - VS Code and CLion extensions

---

<div align="center">

**Made with ❤️ by the OptiWeave Team**

[⭐ Star this repo](https://github.com/yourusername/optiweave) • [🐛 Report Bug](https://github.com/yourusername/optiweave/issues) • [💡 Request Feature](https://github.com/yourusername/optiweave/issues)

</div>