# OptiWeave Demo

A guided walkthrough of OptiWeave's static analysis, instrumentation, runtime profiling, auto-patch, and auto-fix capabilities — all run against real C/C++ source files.

## Prerequisites

Build OptiWeave from the project root:

```bash
./scripts/build.sh
```

The demo expects the binary at `build/optiweave`. Optionally install [Graphviz](https://graphviz.org/) (`dot`) to render call-graph PNGs.

## How to Run

```bash
./demo/run_demo.sh
```

The script is fully self-contained. It creates a `demo/reports/` directory with all generated artifacts and cleans it on each run so results are reproducible.

## What Each Step Does

| Step | Feature | Description |
|------|---------|-------------|
| 1 | `--list-rules` | Prints all 21 registered rules (13 fix + 8 patch) with safety tiers |
| 2 | `--analyze-complexity` | Cyclomatic, cognitive, and Halstead metrics for `matrix.c` |
| 3 | `--call-graph` | Function call graph for `compute.cpp` (Graphviz DOT + optional PNG) |
| 4 | `--dependency-graph` | `#include` dependency graph via preprocessor callbacks |
| 5 | `--data-flow-analysis` | Detects uninitialized vars, unused vars, and dead code in `bugs.c` |
| 6 | `--detect-overflow` | Signed overflow, unsigned wraparound, shift UB, narrowing casts |
| 7 | `--fp-precision-warnings` | Exact FP equality, catastrophic cancellation, mixed precision |
| 8 | `--memory-profile` | Static malloc/free tracking, potential leak detection |
| 9 | Instrumentation | Source-to-source operator wrapping (array, arithmetic, assignment, comparison) |
| 10 | Compile + Runtime | Compiles instrumented binary, runs it with profiling env vars, generates dashboard |
| 11 | `--auto-patch` | Applies 7 optimization patches (reciprocal hoist, SIMD pragma, strength reduction, etc.) |
| 12 | `--auto-fix` | Applies 6 bug-fix kinds (overflow, negation, shift, uninitialized, unused, FP equality) |
| 13 | `--rule-config` | Threshold tuning: adjusts `min_hot_loop_time_ns` and `cache_line_size` |
| 14 | `--safety-tier` / `--min-confidence` | Graduated safety filtering — shows how tighter constraints reduce rule counts |

## Key Output Files

| File/Directory | Contents |
|---|---|
| `complexity_matrix.md` | Function complexity ratings (cyclomatic, cognitive, Halstead) |
| `callgraph.dot` / `callgraph.png` | Call graph visualization |
| `dependencies.dot` | `#include` dependency graph |
| `dataflow.json` | Uninitialized / unused variable findings |
| `overflow_matrix.json`, `overflow_bugs.txt` | Integer overflow risks |
| `fp_bugs.txt`, `fp_compute.json` | Floating-point precision issues |
| `memory.json` | Allocation / deallocation tracking |
| `instrumented_c/`, `instrumented_cpp/` | Transformed source with OptiWeave wrappers |
| `matrix_profiled` | Compiled instrumented binary |
| `dashboard.json` | Runtime hotspot dashboard (8 optimization patterns) |
| `patched/` | Auto-patched source (all 7 patchable rules applied) |
| `fixed/` | Auto-fixed source (6 bug-fix kinds applied) |

## Interpreting Results

**Complexity scores:** Cyclomatic complexity > 10 suggests a function is hard to test; cognitive complexity > 15 suggests it's hard to understand. Halstead effort combines volume and difficulty into a single maintainability metric.

**Safety tiers** control how aggressive OptiWeave is:

| Tier | Meaning | Example rules |
|------|---------|---------------|
| `safe` | Zero semantic risk | Add initializer, suppress unused-var warning |
| `mostly-safe` | Correct for well-defined code; may change UB behavior | Overflow guard, unsigned shift cast |
| `aggressive` | May change observable behavior | FP epsilon comparison, reciprocal hoisting |
| `advisory` | Comment-only, no code change | O(n^2) warning, algorithmic suggestions |

**Confidence** (0.0–1.0) indicates how certain OptiWeave is that a rule applies. Higher `--min-confidence` thresholds filter out lower-certainty matches.

## Feature Comparison

For a side-by-side comparison of OptiWeave against other C/C++ analysis tools, open the interactive comparison matrix:

```bash
open docs/comparison-matrix.html    # macOS
xdg-open docs/comparison-matrix.html  # Linux
```
