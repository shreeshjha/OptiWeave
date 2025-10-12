# OptiWeave Hotspot Tracking Examples

This directory contains examples demonstrating OptiWeave's hotspot detection and profiling capabilities.

## Quick Start

### 1. Basic Hotspot Detection

```bash
# Transform the code
./build/optiweave examples/hotspot_demo.cpp --array-subscripts --compile -o hotspot_demo

# Run with hotspot tracking enabled
OPTIWEAVE_HOTSPOTS_CSV=hotspots.csv ./hotspot_demo
```

### 2. Generate Flame Graph

```bash
# Export flame graph data
OPTIWEAVE_HOTSPOTS_FLAMEGRAPH=hotspots.txt ./hotspot_demo

# Visualize with flamegraph.pl (if installed)
flamegraph.pl hotspots.txt > flamegraph.svg
open flamegraph.svg

# Or upload hotspots.txt to https://www.speedscope.app/
```

### 3. Generate HTML Report

```bash
# Export interactive HTML report
OPTIWEAVE_HOTSPOTS_HTML=report.html ./hotspot_demo

# Open in browser
open report.html
```

### 4. Full Profiling Suite

```bash
# Enable all profiling features at once
OPTIWEAVE_STATS=1 \
OPTIWEAVE_TIMING=1 \
OPTIWEAVE_PROFILE=1 \
OPTIWEAVE_HOTSPOTS_HTML=report.html \
OPTIWEAVE_HOTSPOTS_FLAMEGRAPH=flamegraph.txt \
OPTIWEAVE_HOTSPOTS_CSV=hotspots.csv \
./hotspot_demo
```

## Environment Variables

| Variable | Description | Example |
|----------|-------------|---------|
| `OPTIWEAVE_HOTSPOTS_CSV` | Export hotspot data to CSV | `hotspots.csv` |
| `OPTIWEAVE_HOTSPOTS_JSON` | Export hotspot data to JSON | `hotspots.json` |
| `OPTIWEAVE_HOTSPOTS_FLAMEGRAPH` | Export flame graph (folded stacks) | `flame.txt` |
| `OPTIWEAVE_HOTSPOTS_HTML` | Export interactive HTML report | `report.html` |
| `OPTIWEAVE_HOTSPOTS_TOP_N` | Show top N hotspots | `20` |
| `OPTIWEAVE_STATS` | Enable operation statistics | `1` |
| `OPTIWEAVE_TIMING` | Enable timing profiling | `1` |
| `OPTIWEAVE_PROFILE` | Enable full profiling with percentiles | `1` |

## Understanding the Output

### Console Output
The default console output shows:
- Top hotspots by time spent
- Function-level aggregation
- File-level aggregation
- Visualization bars

### CSV Export
Structured data for spreadsheet analysis:
- Rank, File, Line, Function
- Operation count, Time, Percentage, Average time per operation

### Flame Graph
Compatible with:
- **flamegraph.pl**: Classic flame graph visualization
- **speedscope.app**: Modern interactive viewer (just drag & drop the .txt file)

Flame graphs show the call stack and time distribution, making it easy to spot bottlenecks.

### HTML Report
Beautiful, self-contained HTML report featuring:
- Summary statistics cards
- Top 20 hotspots table with color coding
- Function-level breakdown
- File-level breakdown
- Interactive bar charts
- Responsive design (works on mobile)

## Example Output

```
╔══════════════════════════════════════════════════════════════╗
║            OptiWeave Hotspot Analysis                        ║
╚══════════════════════════════════════════════════════════════╝

Total Runtime: 2.5s
Locations Tracked: 15

Top 10 Hotspots (by time spent):
┌────┬───────────────────────────────────────────────────┬──────────┬──────────┬─────────┬──────────┐
│ #  │ Location                                          │ Ops      │ Time     │ % Total │ Avg/Op   │
├────┼───────────────────────────────────────────────────┼──────────┼──────────┼─────────┼──────────┤
│ 1  │ hotspot_demo.cpp:12 (matrix_multiply)            │ 125.0K   │ 1,234ms  │  52.6%  │  353ns   │
│ 2  │ hotspot_demo.cpp:25 (vector_operation)           │ 500.0K   │  234ms   │  10.0%  │  468ns   │
...
```

## Tips for Effective Profiling

1. **Start with HTML reports** - They provide the most comprehensive overview
2. **Use flame graphs** for understanding call hierarchies
3. **Export CSV** for tracking performance over time
4. **Combine with timing** to understand not just what, but how long
5. **Focus on top 3-5 hotspots** - Pareto principle applies (80/20 rule)

## Building from Source

```bash
mkdir build && cd build
cmake ..
ninja optiweave_runtime
cd ..

# Transform and compile
./build/optiweave examples/hotspot_demo.cpp --array-subscripts --compile -o hotspot_demo
```

## Next Steps

- Check out `FEATURE_PLAN.md` for detailed feature documentation
- Try different workloads to see hotspot detection in action
- Integrate OptiWeave into your build system for continuous profiling
