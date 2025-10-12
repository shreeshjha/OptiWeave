# OptiWeave Command Reference

Complete guide to using OptiWeave for C++ performance profiling and instrumentation.

---

## Table of Contents

- [Quick Start](#quick-start)
- [Transformer Command](#transformer-command)
- [Compilation Flags](#compilation-flags)
- [Runtime Environment Variables](#runtime-environment-variables)
- [Complete Workflow Examples](#complete-workflow-examples)
- [Output Formats](#output-formats)
- [Troubleshooting](#troubleshooting)

---

## Quick Start

```bash
# 1. Transform your C++ code
./build/optiweave input.cpp --array-subscripts --arithmetic-ops -o output.cpp

# 2. Compile with profiling enabled
g++ -std=c++20 -O2 \
  -DOPTIWEAVE_ENABLE_STATS \
  -DOPTIWEAVE_ENABLE_TIMING \
  -DOPTIWEAVE_ENABLE_HOTSPOTS \
  -I./templates -I./include \
  output.cpp \
  build/liboptiweave_runtime.a \
  -o myprogram

# 3. Run with profiling
export OPTIWEAVE_STATS=1
export OPTIWEAVE_HOTSPOTS_HTML=report.html
./myprogram

# 4. View results
open report.html
```

---

## Transformer Command

### Synopsis

```
optiweave [options] <input-file> [-o <output-file>]
```

### Description

The OptiWeave transformer performs source-to-source transformation of C++ code, instrumenting operations for performance profiling and analysis.

### Options

#### Transformation Options

| Flag | Description |
|------|-------------|
| `--array-subscripts` | Instrument array subscript operations (`arr[i]`) |
| `--arithmetic-ops` | Instrument arithmetic operators (`+`, `-`, `*`, `/`, `%`) |
| `--assignment-ops` | Instrument assignment operators (`=`, `+=`, `-=`, etc.) |
| `--comparison-ops` | Instrument comparison operators (`==`, `!=`, `<`, `>`, etc.) |
| `--all` | Enable all transformations (array, arithmetic, assignment, comparison) |

#### Output Options

| Flag | Argument | Description |
|------|----------|-------------|
| `-o` | `<file>` | Output file path (default: modifies input in-place) |
| `--stdout` | - | Write transformed code to stdout |

#### Configuration Options

| Flag | Description |
|------|-------------|
| `--skip-system-headers` | Don't transform code in system headers (default: enabled) |
| `--evaluation-safe` | Use evaluation-safe wrappers to prevent double-evaluation (default: enabled) |

#### Information Options

| Flag | Description |
|------|-------------|
| `--help`, `-h` | Display help message |
| `--version`, `-v` | Display version information |
| `--verbose` | Enable verbose transformation output |

### Examples

**Transform array subscripts only:**
```bash
./build/optiweave mycode.cpp --array-subscripts -o mycode_instrumented.cpp
```

**Transform arithmetic and array operations:**
```bash
./build/optiweave mycode.cpp --arithmetic-ops --array-subscripts -o mycode_instrumented.cpp
```

**Transform everything:**
```bash
./build/optiweave mycode.cpp --all -o mycode_instrumented.cpp
```

**Output to stdout:**
```bash
./build/optiweave mycode.cpp --arithmetic-ops --stdout > transformed.cpp
```

### Transformation Statistics

After transformation, OptiWeave prints statistics:

```
=== Transformation Complete ===
Transformation Statistics:
  Array subscripts transformed: 42
  Arithmetic operators transformed: 156
  Template instantiations skipped: 3
  Errors encountered: 0
```

---

## Compilation Flags

### Required Compiler Flags

```bash
-std=c++20        # C++20 or later required
-I./templates     # Include OptiWeave prelude
-I./include       # Include OptiWeave runtime headers
```

### Linking

```bash
build/liboptiweave_runtime.a    # Link against OptiWeave runtime library
```

### Feature Enable Flags

Enable specific profiling features at compile-time:

| Flag | Description | Overhead |
|------|-------------|----------|
| `-DOPTIWEAVE_ENABLE_STATS` | Enable operation counting | ~0.5% |
| `-DOPTIWEAVE_ENABLE_TIMING` | Enable timing measurements | ~2-5% |
| `-DOPTIWEAVE_ENABLE_HOTSPOTS` | Enable hotspot tracking | ~3-7% |

**Recommendation:** Enable only the features you need to minimize overhead.

### Example Compilation Commands

**Statistics only (minimal overhead):**
```bash
g++ -std=c++20 -O2 \
  -DOPTIWEAVE_ENABLE_STATS \
  -I./templates -I./include \
  mycode_instrumented.cpp \
  build/liboptiweave_runtime.a \
  -o myprogram
```

**Full profiling (all features):**
```bash
g++ -std=c++20 -O2 \
  -DOPTIWEAVE_ENABLE_STATS \
  -DOPTIWEAVE_ENABLE_TIMING \
  -DOPTIWEAVE_ENABLE_HOTSPOTS \
  -I./templates -I./include \
  mycode_instrumented.cpp \
  build/liboptiweave_runtime.a \
  -o myprogram
```

**Multi-threaded programs:**
```bash
g++ -std=c++20 -O2 -pthread \
  -DOPTIWEAVE_ENABLE_STATS \
  -DOPTIWEAVE_ENABLE_TIMING \
  -DOPTIWEAVE_ENABLE_HOTSPOTS \
  -I./templates -I./include \
  mycode_instrumented.cpp \
  build/liboptiweave_runtime.a \
  -o myprogram
```

---

## Runtime Environment Variables

Control OptiWeave's behavior at runtime using environment variables.

### Statistics Control

| Variable | Values | Default | Description |
|----------|--------|---------|-------------|
| `OPTIWEAVE_STATS` | `0`, `1` | `0` | Enable statistics output |
| `OPTIWEAVE_STATS_CSV` | `<path>` | - | Export statistics to CSV file |
| `OPTIWEAVE_STATS_JSON` | `<path>` | - | Export statistics to JSON file |

**Example:**
```bash
export OPTIWEAVE_STATS=1
export OPTIWEAVE_STATS_CSV=stats.csv
export OPTIWEAVE_STATS_JSON=stats.json
./myprogram
```

### Timing Control

| Variable | Values | Default | Description |
|----------|--------|---------|-------------|
| `OPTIWEAVE_TIMING` | `0`, `1` | `0` | Enable basic timing statistics |
| `OPTIWEAVE_PROFILE` | `0`, `1` | `0` | Enable detailed profiling with percentiles |
| `OPTIWEAVE_TIMING_SAMPLE_RATE` | `0.0-1.0` | `0.01` | Sampling rate (1% default) |
| `OPTIWEAVE_TIMING_CSV` | `<path>` | - | Export timing data to CSV |
| `OPTIWEAVE_TIMING_JSON` | `<path>` | - | Export timing data to JSON |

**Example:**
```bash
export OPTIWEAVE_PROFILE=1
export OPTIWEAVE_TIMING_SAMPLE_RATE=0.1  # 10% sampling
export OPTIWEAVE_TIMING_CSV=timing.csv
./myprogram
```

### Hotspot Tracking Control

| Variable | Values | Default | Description |
|----------|--------|---------|-------------|
| `OPTIWEAVE_HOTSPOTS_TOP_N` | `<number>` | `10` | Number of top hotspots to display |
| `OPTIWEAVE_HOTSPOTS_CSV` | `<path>` | - | Export hotspots to CSV |
| `OPTIWEAVE_HOTSPOTS_JSON` | `<path>` | - | Export hotspots to JSON |
| `OPTIWEAVE_HOTSPOTS_FLAMEGRAPH` | `<path>` | - | Export flame graph data |
| `OPTIWEAVE_HOTSPOTS_HTML` | `<path>` | - | Generate HTML report |

**Example:**
```bash
export OPTIWEAVE_HOTSPOTS_TOP_N=20
export OPTIWEAVE_HOTSPOTS_HTML=hotspots.html
export OPTIWEAVE_HOTSPOTS_FLAMEGRAPH=flame.txt
./myprogram
```

### All-in-One Configuration

```bash
# Enable everything with exports
export OPTIWEAVE_STATS=1
export OPTIWEAVE_STATS_CSV=stats.csv
export OPTIWEAVE_PROFILE=1
export OPTIWEAVE_TIMING_CSV=timing.csv
export OPTIWEAVE_HOTSPOTS_TOP_N=20
export OPTIWEAVE_HOTSPOTS_HTML=report.html
export OPTIWEAVE_HOTSPOTS_FLAMEGRAPH=flame.txt
./myprogram
```

**Or use inline:**
```bash
OPTIWEAVE_STATS=1 \
OPTIWEAVE_PROFILE=1 \
OPTIWEAVE_HOTSPOTS_HTML=report.html \
./myprogram
```

---

## Complete Workflow Examples

### Example 1: Basic Array Performance Analysis

```bash
# 1. Create a simple test program
cat > test.cpp << 'EOF'
#include <iostream>

int main() {
    int arr[1000];
    int sum = 0;

    for (int i = 0; i < 1000; i++) {
        arr[i] = i;
    }

    for (int i = 0; i < 1000; i++) {
        sum += arr[i];
    }

    std::cout << "Sum: " << sum << std::endl;
    return 0;
}
EOF

# 2. Transform the code
./build/optiweave test.cpp --array-subscripts -o test_instrumented.cpp

# 3. Compile with profiling
g++ -std=c++20 -O2 \
  -DOPTIWEAVE_ENABLE_STATS \
  -DOPTIWEAVE_ENABLE_HOTSPOTS \
  -I./templates -I./include \
  test_instrumented.cpp \
  build/liboptiweave_runtime.a \
  -o test

# 4. Run and view results
OPTIWEAVE_STATS=1 OPTIWEAVE_HOTSPOTS_HTML=report.html ./test

# 5. Open the HTML report
open report.html  # macOS
# or
xdg-open report.html  # Linux
```

### Example 2: Matrix Multiplication Performance

```bash
# 1. Transform matrix multiplication code
./build/optiweave matrix_mult.cpp --all -o matrix_mult_instrumented.cpp

# 2. Compile with full profiling
g++ -std=c++20 -O3 \
  -DOPTIWEAVE_ENABLE_STATS \
  -DOPTIWEAVE_ENABLE_TIMING \
  -DOPTIWEAVE_ENABLE_HOTSPOTS \
  -I./templates -I./include \
  matrix_mult_instrumented.cpp \
  build/liboptiweave_runtime.a \
  -o matrix_mult

# 3. Run with detailed profiling
export OPTIWEAVE_STATS=1
export OPTIWEAVE_PROFILE=1
export OPTIWEAVE_HOTSPOTS_TOP_N=20
export OPTIWEAVE_HOTSPOTS_HTML=matrix_report.html
export OPTIWEAVE_HOTSPOTS_FLAMEGRAPH=matrix_flame.txt
export OPTIWEAVE_STATS_CSV=matrix_stats.csv
./matrix_mult

# 4. View flame graph at speedscope.app
# Upload matrix_flame.txt to https://www.speedscope.app/

# 5. View HTML report
open matrix_report.html
```

### Example 3: Multi-threaded Application

```bash
# 1. Transform parallel code
./build/optiweave parallel_app.cpp --arithmetic-ops --array-subscripts -o parallel_instrumented.cpp

# 2. Compile with threading support
g++ -std=c++20 -O2 -pthread \
  -DOPTIWEAVE_ENABLE_STATS \
  -DOPTIWEAVE_ENABLE_TIMING \
  -DOPTIWEAVE_ENABLE_HOTSPOTS \
  -I./templates -I./include \
  parallel_instrumented.cpp \
  build/liboptiweave_runtime.a \
  -o parallel_app

# 3. Run with profiling
OPTIWEAVE_STATS=1 \
OPTIWEAVE_PROFILE=1 \
OPTIWEAVE_HOTSPOTS_HTML=parallel_report.html \
./parallel_app

# 4. Check thread-safety in report
cat parallel_report.html
```

### Example 4: Comparing Optimizations

```bash
# Baseline: no optimizations
./build/optiweave code.cpp --all -o code_O0.cpp
g++ -std=c++20 -O0 -DOPTIWEAVE_ENABLE_STATS -I./templates -I./include \
    code_O0.cpp build/liboptiweave_runtime.a -o code_O0
OPTIWEAVE_STATS=1 OPTIWEAVE_STATS_CSV=stats_O0.csv ./code_O0

# With -O2
g++ -std=c++20 -O2 -DOPTIWEAVE_ENABLE_STATS -I./templates -I./include \
    code_O0.cpp build/liboptiweave_runtime.a -o code_O2
OPTIWEAVE_STATS=1 OPTIWEAVE_STATS_CSV=stats_O2.csv ./code_O2

# With -O3
g++ -std=c++20 -O3 -DOPTIWEAVE_ENABLE_STATS -I./templates -I./include \
    code_O0.cpp build/liboptiweave_runtime.a -o code_O3
OPTIWEAVE_STATS=1 OPTIWEAVE_STATS_CSV=stats_O3.csv ./code_O3

# Compare CSV files
diff stats_O0.csv stats_O2.csv
diff stats_O2.csv stats_O3.csv
```

---

## Output Formats

### Console Output

OptiWeave prints formatted reports to the console at program exit:

```
╔══════════════════════════════════════════════════╗
║       OptiWeave Operation Statistics             ║
╚══════════════════════════════════════════════════╝

Runtime: 0.123 seconds
Total Operations: 1,234,567

Operation Breakdown:
┌─────────────────────┬────────────┬──────────┬────────────────┐
│ Operation Type      │ Count      │ Percent  │ Ops/Second     │
├─────────────────────┼────────────┼──────────┼────────────────┤
│ Multiplications     │     500000 │    40.5% │       4.07M ops/s │
│ Additions           │     400000 │    32.4% │       3.25M ops/s │
│ Array Subscripts    │     334567 │    27.1% │       2.72M ops/s │
└─────────────────────┴────────────┴──────────┴────────────────┘

Insights:
• Multiplications dominate (40.5%)
• High arithmetic intensity (2.69 arithmetic ops per memory access) - compute-bound
```

### CSV Format

**Statistics CSV:**
```csv
Operation,Count,Percentage,OpsPerSecond
array_subscript,334567,27.1,2720000
addition,400000,32.4,3250000
multiplication,500000,40.5,4070000
```

**Hotspots CSV:**
```csv
Rank,File,Line,Function,Operations,Time_ns,Percent,Avg_ns
1,mycode.cpp,45,matrix_mult,1000000,1234567890,45.2,1234
2,mycode.cpp,78,vector_add,500000,654321098,23.9,1308
```

### JSON Format

**Statistics JSON:**
```json
{
  "runtime_seconds": 0.123,
  "total_operations": 1234567,
  "operations": {
    "array_subscript": {
      "count": 334567,
      "percentage": 27.1,
      "ops_per_second": 2720000
    },
    "addition": {
      "count": 400000,
      "percentage": 32.4,
      "ops_per_second": 3250000
    }
  }
}
```

**Hotspots JSON:**
```json
{
  "total_runtime_ns": 1234567890,
  "locations_tracked": 42,
  "hotspots": [
    {
      "rank": 1,
      "file": "mycode.cpp",
      "line": 45,
      "function": "matrix_mult",
      "operation_count": 1000000,
      "total_time_ns": 1234567890,
      "percentage": 45.2,
      "avg_time_ns": 1234
    }
  ]
}
```

### Flame Graph Format

**Folded stacks format** (compatible with flamegraph.pl and speedscope.app):

```
mycode.cpp;matrix_mult 1234567890
mycode.cpp;vector_add 654321098
mycode.cpp;process_data 234567890
```

**Usage with speedscope.app:**
1. Generate flame graph: `export OPTIWEAVE_HOTSPOTS_FLAMEGRAPH=flame.txt`
2. Run your program
3. Upload `flame.txt` to https://www.speedscope.app/

### HTML Report

Interactive HTML report with:
- Summary statistics
- Top 20 hotspots table
- Function-level breakdown
- File-level breakdown
- Bar charts
- Responsive design

**Generate:**
```bash
export OPTIWEAVE_HOTSPOTS_HTML=report.html
./myprogram
open report.html
```

---

## Troubleshooting

### Common Issues

#### 1. Transformation Errors

**Problem:** `Error while processing file.cpp`

**Solutions:**
- Ensure your code compiles with standard C++ compiler first
- Check for missing include paths
- Verify LLVM/Clang version compatibility (13-17 supported)

```bash
# Test if code compiles normally first
g++ -std=c++20 -c mycode.cpp

# Then transform
./build/optiweave mycode.cpp --all -o mycode_instrumented.cpp
```

#### 2. Compilation Errors

**Problem:** `fatal error: 'optiweave/prelude.hpp' file not found`

**Solution:** Add include paths:
```bash
g++ -I./templates -I./include ...
```

**Problem:** `undefined reference to 'optiweave::statistics::g_counters'`

**Solution:** Link the runtime library:
```bash
g++ ... build/liboptiweave_runtime.a
```

#### 3. No Output at Runtime

**Problem:** Program runs but no statistics printed

**Solutions:**
- Enable profiling with environment variables:
  ```bash
  export OPTIWEAVE_STATS=1
  ```
- Ensure you compiled with feature flags:
  ```bash
  -DOPTIWEAVE_ENABLE_STATS
  ```

#### 4. Source Locations Show `:0`

**Problem:** Hotspot tracking shows `:0` instead of actual file:line

**Solution:** This is fixed in the latest version. Rebuild OptiWeave from source.

#### 5. High Overhead

**Problem:** Instrumented program runs much slower

**Solutions:**
- Reduce sampling rate:
  ```bash
  export OPTIWEAVE_TIMING_SAMPLE_RATE=0.001  # 0.1% sampling
  ```
- Enable only statistics (disable timing/hotspots):
  ```bash
  # Compile with only:
  -DOPTIWEAVE_ENABLE_STATS
  ```
- Use -O2 or -O3 optimization:
  ```bash
  g++ -O3 ...
  ```

#### 6. Multi-threading Issues

**Problem:** Race conditions or incorrect counts in threaded programs

**Solutions:**
- Ensure you compile with `-pthread`:
  ```bash
  g++ -pthread ...
  ```
- OptiWeave uses atomic operations - should work correctly
- Run the multi-threading test to verify:
  ```bash
  ./tests/unit/test_multithreading
  ```

---

## Best Practices

### 1. Start Simple

Begin with statistics only, then add timing/hotspots as needed:

```bash
# Phase 1: Just count operations
-DOPTIWEAVE_ENABLE_STATS

# Phase 2: Add timing
-DOPTIWEAVE_ENABLE_STATS -DOPTIWEAVE_ENABLE_TIMING

# Phase 3: Full profiling
-DOPTIWEAVE_ENABLE_STATS -DOPTIWEAVE_ENABLE_TIMING -DOPTIWEAVE_ENABLE_HOTSPOTS
```

### 2. Use Appropriate Sampling

For production profiling, use low sampling rates:

```bash
export OPTIWEAVE_TIMING_SAMPLE_RATE=0.001  # 0.1% for production
export OPTIWEAVE_TIMING_SAMPLE_RATE=0.01   # 1% for development
export OPTIWEAVE_TIMING_SAMPLE_RATE=0.1    # 10% for detailed analysis
```

### 3. Export Data for Analysis

Always export to files for later analysis:

```bash
export OPTIWEAVE_STATS_CSV=stats.csv
export OPTIWEAVE_STATS_JSON=stats.json
export OPTIWEAVE_HOTSPOTS_HTML=report.html
export OPTIWEAVE_HOTSPOTS_FLAMEGRAPH=flame.txt
```

### 4. Compare Before/After

Profile both original and optimized versions:

```bash
# Before optimization
OPTIWEAVE_STATS_CSV=before.csv ./program_v1

# After optimization
OPTIWEAVE_STATS_CSV=after.csv ./program_v2

# Compare
diff before.csv after.csv
```

### 5. Focus on Hotspots

Use hotspot tracking to find bottlenecks:

```bash
export OPTIWEAVE_HOTSPOTS_TOP_N=20
export OPTIWEAVE_HOTSPOTS_HTML=hotspots.html
./myprogram
open hotspots.html
# Focus optimization efforts on top 20% of hotspots
```

---

## Quick Reference Card

```bash
# Transform
./build/optiweave input.cpp --all -o output.cpp

# Compile
g++ -std=c++20 -O2 -DOPTIWEAVE_ENABLE_{STATS,TIMING,HOTSPOTS} \
    -I./templates -I./include output.cpp build/liboptiweave_runtime.a -o prog

# Run
OPTIWEAVE_STATS=1 OPTIWEAVE_PROFILE=1 OPTIWEAVE_HOTSPOTS_HTML=report.html ./prog

# View
open report.html
```

---

## Version Information

```bash
./build/optiweave --version
```

```
OptiWeave v0.1-alpha
Source-to-source C++ instrumentation framework
Built with LLVM 17.0.0
```

---

## Getting Help

```bash
./build/optiweave --help
```

For more information:
- GitHub: https://github.com/anthropics/optiweave
- Issues: https://github.com/anthropics/optiweave/issues
- Documentation: See `docs/` directory

---

**Last Updated:** 2025-10-07
**OptiWeave Version:** 0.1-alpha
