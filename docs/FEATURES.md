# OptiWeave Features Documentation

This document describes all implemented features in OptiWeave and how to use them.

---

## Feature 1.1: Operation Statistics & Counting

**Status:** ✅ COMPLETE

### Overview

Track and count every operation (array subscripts, arithmetic operations, assignments, comparisons) that executes in your program, providing a quantitative breakdown of computational work.

### Usage

```bash
# Transform and compile with statistics enabled
./build/optiweave mycode.cpp --enable-stats --compile -o myapp

# Run the instrumented program
./myapp
```

#### With CSV Export

```bash
./build/optiweave mycode.cpp --enable-stats --export-stats-csv=report.csv --compile -o myapp
./myapp
# Statistics saved to report.csv
```

#### With JSON Export

```bash
./build/optiweave mycode.cpp --enable-stats --export-stats-json=report.json --compile -o myapp
./myapp
# Statistics saved to report.json
```

### CLI Options

| Option | Description |
|--------|-------------|
| `--enable-stats` | Enable operation statistics collection at runtime |
| `--export-stats-csv <file>` | Export statistics to CSV file |
| `--export-stats-json <file>` | Export statistics to JSON file |

### Example Output

```
╔══════════════════════════════════════════════════╗
║       OptiWeave Operation Statistics             ║
╚══════════════════════════════════════════════════╝

Runtime: 0.001 seconds
Total Operations: 20

Operation Breakdown:
┌─────────────────────┬────────────┬──────────┬────────────────┐
│ Operation Type      │ Count      │ Percent  │ Ops/Second     │
├─────────────────────┼────────────┼──────────┼────────────────┤
│ Array Subscripts    │         20 │   100.0% │   27.36K ops/s │
└─────────────────────┴────────────┴──────────┴────────────────┘

Insights:
• Array Subscripts dominate (100.0%)
```

### Tracked Operations

- Array subscript operations (`arr[i]`)
- Arithmetic operations (`+`, `-`, `*`, `/`, `%`)
- Assignment operations (`=`, `+=`, `-=`, `*=`, `/=`, `%=`)
- Comparison operations (`==`, `!=`, `<`, `>`, `<=`, `>=`)

---

## Feature 1.2: High-Resolution Timing & Performance Metrics

**Status:** ✅ COMPLETE

### Overview

Measure the actual time spent in each operation type with nanosecond precision, identifying which operations consume the most CPU time.

### Usage

```bash
# Enable basic timing
./build/optiweave mycode.cpp --enable-timing --compile -o myapp
./myapp

# Enable full profiling with percentiles
./build/optiweave mycode.cpp --enable-profile --compile -o myapp
./myapp

# With CSV export
./build/optiweave mycode.cpp --enable-timing --export-timing-csv=timing.csv --compile -o myapp

# With JSON export
./build/optiweave mycode.cpp --enable-timing --export-timing-json=timing.json --compile -o myapp
```

### CLI Options

| Option | Description |
|--------|-------------|
| `--enable-timing` | Enable high-resolution timing of operations |
| `--enable-profile` | Enable full profiling with percentiles and histograms |
| `--export-timing-csv <file>` | Export timing data to CSV file |
| `--export-timing-json <file>` | Export timing data to JSON file |

### Example Output

```
╔══════════════════════════════════════════════════╗
║       OptiWeave Performance Profile              ║
╚══════════════════════════════════════════════════╝

Total Time: 125.45ms

Time Breakdown by Operation:
┌─────────────────┬─────────┬──────────┬─────────┬─────────┬─────────┬─────────┐
│ Operation       │ Count   │ Total    │ Avg     │ Min     │ Max     │ p95     │
├─────────────────┼─────────┼──────────┼─────────┼─────────┼─────────┼─────────┤
│ Array Access    │  50000  │ 66.34ms  │  1.33μs │   100ns │  45.2μs │  2.1μs  │
│ Multiplication  │  25000  │ 42.11ms  │  1.68μs │   150ns │  38.7μs │  2.5μs  │
│ Addition        │  15000  │ 17.00ms  │  1.13μs │    80ns │  28.4μs │  1.8μs  │
└─────────────────┴─────────┴──────────┴─────────┴─────────┴─────────┴─────────┘

Time Distribution:
  Array Access      ████████████████████████████ 52.9%  (66.34ms)
  Multiplication    ██████████████████           33.6%  (42.11ms)
  Addition          ████████                     13.6%  (17.00ms)

Key Insights:
• Array Access is the bottleneck (52.9% of time)
• Average Array Access time (1.33μs) suggests possible cache misses
```

### Measured Metrics

- **Total Time**: Cumulative time spent in each operation type
- **Average Time**: Mean execution time per operation
- **Min/Max**: Fastest and slowest operation execution
- **Percentiles** (with `--enable-profile`): p50, p90, p95, p99

---

## Future Features

The following features are planned but not yet implemented. See [FEATURE_PLAN.md](../FEATURE_PLAN.md) for complete details.

### Feature 1.3: Hotspot Detection & Source Location Tracking
*Status: Planned*
- Identify which specific lines of source code are performance bottlenecks
- Rank locations by time/frequency
- Flame graph integration

### Feature 1.4: Optimization Suggestions
*Status: Planned*
- AI-powered optimization recommendations
- Pattern detection and anti-pattern identification
- Automated code improvement suggestions

---

## General Usage

### Basic Transformation

```bash
# Transform only (no compilation)
./build/optiweave mycode.cpp --

# Transform and compile
./build/optiweave mycode.cpp --compile -o myapp

# Verbose output
./build/optiweave mycode.cpp --verbose --compile -o myapp
```

### Common Options

| Option | Description |
|--------|-------------|
| `--compile` | Automatically compile transformed code |
| `-o <name>` | Output executable name (requires --compile) |
| `--verbose` | Enable verbose output |
| `--dry-run` | Parse and analyze without writing changes |
| `--evaluation-safe` | Use wrappers to ensure single-evaluation of operands (default: ON) |

---

## Environment Variables

Some features support runtime configuration via environment variables:

### Statistics

- `OPTIWEAVE_STATS=1` - Enable statistics (if compiled with support)
- `OPTIWEAVE_STATS_CSV=<file>` - Export to CSV
- `OPTIWEAVE_STATS_JSON=<file>` - Export to JSON

---

## Performance Considerations

- **Statistics Overhead**: < 1% with atomic operations
- **Instrumentation**: Minimal impact on program behavior
- **Thread Safety**: All counters are thread-safe using atomic operations

---

## Example Workflow

```bash
# 1. Transform and compile with statistics
./build/optiweave examples/basic_transformation/example.cpp \
    --enable-stats \
    --export-stats-csv=stats.csv \
    --compile -o example_instrumented

# 2. Run the instrumented program
./example_instrumented

# 3. Analyze the output
cat stats.csv
```

---

*Last updated: 2025-10-06*
