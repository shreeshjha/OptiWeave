# OptiWeave Thesis Evaluation - Quick Start Guide

## 4-Week Aggressive Timeline

This guide helps you complete all thesis research questions in under 1 month.

## Week 1: Setup + Baseline (Days 1-7)

### Day 1-2: Build OptiWeave
```bash
cd /Users/shreesh/Dev/Github/OptiWeave
./scripts/build.sh
```

### Day 3: Set up Polybench (RQ1)
```bash
cd evaluation/scripts
./setup_polybench.sh
```

### Day 4: Set up Juliet/Synthetic Tests (RQ2)
```bash
./setup_juliet.sh
```

### Day 5: Set up json-c (RQ3)
```bash
./setup_case_study.sh
```

### Day 6-7: Install baseline tools
```bash
# macOS
brew install cppcheck

# Verify tools
which gprof    # Should be available by default
which cppcheck
perf --version  # Linux only, skip on macOS
```

## Week 2: Data Collection (Days 8-14)

### Day 8-10: RQ1 - Overhead Experiments
```bash
cd evaluation/scripts

# Run benchmarks (takes ~2-4 hours)
./run_rq1_overhead.sh

# Analyze results
python3 analyze_rq1.py
```

**Expected output:** `evaluation/results/rq1/overhead_summary.csv`

### Day 11-12: RQ2 - Precision/Recall
```bash
# Run static analysis tests
./run_rq2_precision.sh

# MANUAL STEP: Review outputs and fill in template
# Edit: evaluation/results/rq2/manual_analysis_template.csv

# Analyze results
python3 analyze_rq2.py
```

**Expected output:** `evaluation/results/rq2/precision_recall_summary.csv`

### Day 13-14: Buffer/Catch-up

## Week 3: Case Study (Days 15-21)

### Day 15-17: RQ3 - json-c Analysis
```bash
# Run hybrid analysis
./run_rq3_case_study.sh

# MANUAL STEP: Review all outputs
# Fill in: evaluation/results/rq3/case_study_findings.md
```

### Day 18-19: Document findings
- Take screenshots
- Create diagrams
- Write case study narrative

### Day 20-21: Buffer/Catch-up

## Week 4: Analysis & Documentation (Days 22-28)

### Day 22-24: Data Consolidation
- Create thesis tables
- Generate graphs (use Python matplotlib or Excel)
- Statistical analysis

### Day 25-27: Start thesis writing
- Chapter 5 (Results)
- Chapter 6 (Case Studies)

### Day 28: Buffer

## Key Files Generated

```
evaluation/
├── results/
│   ├── rq1/
│   │   ├── overhead_results.csv          # Raw data
│   │   └── overhead_summary.csv          # For thesis tables
│   ├── rq2/
│   │   ├── precision_recall_summary.csv  # For thesis tables
│   │   └── manual_analysis_template.csv
│   └── rq3/
│       └── case_study_findings.md        # For thesis Chapter 6
└── data/
    └── [processed datasets]
```

## Thesis Chapter Mapping

| Research Question | Evaluation Scripts | Thesis Chapter |
|-------------------|-------------------|----------------|
| RQ1: Overhead | `run_rq1_overhead.sh` | Chapter 5.1 |
| RQ2: Precision | `run_rq2_precision.sh` | Chapter 5.2 |
| RQ3: Hybrid Synergy | `run_rq3_case_study.sh` | Chapter 5.3 & 6 |

## Critical Success Factors

1. **Week 1 is crucial** - Get all benchmarks set up properly
2. **Automate everything** - Scripts are ready, use them
3. **Document as you go** - Don't wait until Week 4
4. **Skip RQ4** - User study is optional for 4-week timeline
5. **Focus on quality** - Better to do 3 RQs well than 4 RQs poorly

## Troubleshooting

### OptiWeave build fails
```bash
# Check LLVM/Clang installation
clang --version

# Rebuild
rm -rf build/
./scripts/build.sh
```

### Polybench benchmarks fail
- Some kernels may not compile - that's OK
- Need at least 5-7 successful benchmarks for statistically significant results

### No Juliet access
- Use the synthetic test cases in `evaluation/benchmarks/juliet/`
- Create more synthetic tests based on known CWE patterns
- Cite this as limitation in thesis

## Expected Time Investment

- RQ1 benchmarks: 4-6 hours runtime + 2-3 hours analysis
- RQ2 precision: 2-3 hours execution + 4-5 hours manual review
- RQ3 case study: 8-10 hours analysis + documentation

**Total: ~20-30 hours of active work + waiting for benchmarks**

## Next Steps After Evaluation

1. Generate thesis tables and figures
2. Write results chapters (5 & 6)
3. Update introduction with concrete findings
4. Prepare defense presentation

## Questions?

Refer back to `Thesis_Plan.md` for detailed context on each RQ.
