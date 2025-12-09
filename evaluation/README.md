# OptiWeave Thesis Evaluation

This directory contains all evaluation materials for the MSc thesis.

## Structure

- `benchmarks/` - Benchmark suites and test programs
  - `polybench/` - Polybench/C kernels for RQ1 (overhead analysis)
  - `juliet/` - Juliet Test Suite samples for RQ2 (precision/recall)
  - `case-studies/` - Real-world projects for RQ3 (hybrid analysis)
- `scripts/` - Automation scripts
- `results/` - Raw experimental results (CSV, JSON)
- `data/` - Processed data and analysis

## Research Questions Coverage

### RQ1: Instrumentation Overhead
**Benchmarks:** Polybench/C (10 kernels)
**Baselines:** gprof, perf stat, Cppcheck
**Metrics:** Runtime overhead %, slowdown factor

### RQ2: Static Analysis Precision
**Benchmarks:** Juliet Test Suite (CWE-190, CWE-457)
**Baselines:** Cppcheck
**Metrics:** Precision, Recall, F1 score

### RQ3: Hybrid Analysis Synergy
**Benchmarks:** json-c (~15K LOC)
**Metrics:** Bugs found, optimization impact

## Timeline (4 weeks)

Week 1: Setup + baseline measurements
Week 2: RQ1 + RQ2 data collection
Week 3: RQ3 case study
Week 4: Data consolidation + analysis
