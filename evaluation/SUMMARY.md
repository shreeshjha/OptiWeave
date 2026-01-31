# Evaluation Infrastructure Summary

## What's Been Created

I've set up a complete evaluation framework for your MSc thesis with automated scripts for all research questions.

### Directory Structure

```
evaluation/
├── README.md                  # Overview and RQ mapping
├── QUICKSTART.md             # 4-week aggressive timeline guide
├── benchmarks/               # Test suites
│   ├── polybench/           # For RQ1 overhead analysis
│   ├── juliet/              # For RQ2 precision/recall
│   └── case-studies/        # For RQ3 hybrid analysis (json-c)
├── scripts/                  # Automation scripts
│   ├── setup_polybench.sh
│   ├── setup_juliet.sh
│   ├── setup_case_study.sh
│   ├── run_rq1_overhead.sh
│   ├── run_rq2_precision.sh
│   ├── run_rq3_case_study.sh
│   ├── analyze_rq1.py
│   └── analyze_rq2.py
├── results/                  # Raw experimental data
│   ├── rq1/
│   ├── rq2/
│   └── rq3/
└── data/                     # Processed datasets
```

## Scripts Overview

### Setup Scripts (Week 1)

1. **`setup_polybench.sh`**
   - Downloads Polybench/C benchmark suite
   - Selects 10 representative kernels
   - Ready for RQ1 overhead experiments

2. **`setup_juliet.sh`**
   - Instructions for Juliet Test Suite
   - Creates synthetic overflow tests
   - Ready for RQ2 precision evaluation

3. **`setup_case_study.sh`**
   - Clones json-c (~15K LOC)
   - Builds the project
   - Ready for RQ3 hybrid analysis

### Evaluation Scripts (Week 2-3)

4. **`run_rq1_overhead.sh`**
   - **Purpose:** Measure instrumentation overhead
   - **Runs:** 10 Polybench kernels × 10 iterations
   - **Modes:** Baseline, gprof, OptiWeave (array), OptiWeave (arithmetic)
   - **Output:** `results/rq1/overhead_results.csv`
   - **Runtime:** 2-4 hours

5. **`analyze_rq1.py`**
   - Calculates mean/std/min/max overhead
   - Generates thesis-ready tables
   - Produces key findings summary

6. **`run_rq2_precision.sh`**
   - **Purpose:** Evaluate static analysis precision
   - **Runs:** OptiWeave + Cppcheck on test cases
   - **Analyses:** Overflow, FP precision, data flow
   - **Output:** Requires manual classification
   - **Runtime:** 1-2 hours + manual review

7. **`analyze_rq2.py`**
   - Calculates Precision, Recall, F1 scores
   - Confusion matrix analysis
   - Comparison with Cppcheck

8. **`run_rq3_case_study.sh`**
   - **Purpose:** Demonstrate hybrid analysis
   - **Runs:** Full OptiWeave analysis on json-c
   - **Phases:** Static → Dynamic → Synthesis
   - **Output:** Findings template to fill in
   - **Runtime:** 2-3 hours + documentation

## Usage Flow

### Quick Start (Copy-Paste Commands)

```bash
# Week 1: Setup (Day 1-5)
cd /Users/shreesh/Dev/Github/OptiWeave
./scripts/build.sh

cd evaluation/scripts
./setup_polybench.sh
./setup_juliet.sh
./setup_case_study.sh

# Week 2: RQ1 (Day 8-10)
./run_rq1_overhead.sh          # Wait 2-4 hours
python3 analyze_rq1.py         # Get thesis tables

# Week 2: RQ2 (Day 11-12)
./run_rq2_precision.sh
# Fill in: ../results/rq2/manual_analysis_template.csv
python3 analyze_rq2.py

# Week 3: RQ3 (Day 15-17)
./run_rq3_case_study.sh
# Fill in: ../results/rq3/case_study_findings.md
```

## Expected Thesis Contributions

### RQ1 Results (Chapter 5.1)
**Tables generated:**
- Overhead by benchmark (10 kernels)
- Overhead by instrumentation mode
- Statistical summary (mean, std, min, max)

**Key finding example:**
> "OptiWeave's array instrumentation adds X% overhead on average,
> compared to Y% for gprof, demonstrating favorable performance
> characteristics for AST-level instrumentation."

### RQ2 Results (Chapter 5.2)
**Tables generated:**
- Precision/Recall/F1 comparison table
- Confusion matrix for each tool
- False positive analysis

**Key finding example:**
> "OptiWeave achieves X% precision and Y% recall for integer
> overflow detection, comparable to Cppcheck (X2%, Y2%) while
> providing integrated runtime validation."

### RQ3 Results (Chapters 5.3 & 6)
**Content generated:**
- Complete case study narrative
- Bugs found through hybrid approach
- Optimization opportunities identified
- Workflow efficiency analysis

**Key finding example:**
> "In the json-c case study, combining static warnings with
> runtime profiling identified N optimization opportunities
> that neither approach found independently."

## Realistic 4-Week Timeline

| Week | Tasks | Hours | Deliverables |
|------|-------|-------|--------------|
| 1 | Setup all benchmarks | 10-15h | All benchmarks ready |
| 2 | Run RQ1 + RQ2 | 15-20h | CSV data files |
| 3 | Run RQ3 + documentation | 15-20h | Case study findings |
| 4 | Analysis + thesis writing | 20-30h | Results chapters drafted |

**Total: 60-85 hours over 4 weeks** (15-20 hours/week)

## Next Steps

1. ✅ Read `QUICKSTART.md` for detailed 4-week plan
2. ⏳ Run setup scripts (Week 1)
3. ⏳ Execute evaluation scripts (Week 2-3)
4. ⏳ Analyze and document (Week 4)
