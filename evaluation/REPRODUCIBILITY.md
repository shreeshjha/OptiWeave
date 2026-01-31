# Reproducibility Guide for OptiWeave Evaluation

This document provides instructions for reproducing all experimental results in the OptiWeave MSc thesis evaluation.

---

## System Requirements

### Hardware
- CPU: x86_64 architecture (Intel/AMD)
- RAM: 8 GB minimum (16 GB recommended for full Polybench suite)
- Disk: 2 GB free space

### Software
| Component | Version | Purpose |
|-----------|---------|---------|
| Ubuntu | 22.04+ | Operating system |
| GCC | 11.0+ | C/C++ compiler |
| Clang/LLVM | 14.0+ | OptiWeave foundation |
| CMake | 3.16+ | Build system |
| Python | 3.8+ | Analysis scripts |
| pandas | 1.3+ | Data analysis |
| scipy | 1.7+ | Statistical tests |
| matplotlib | 3.5+ | Visualization |

### Installation

```bash
# Install system dependencies
sudo apt update
sudo apt install -y build-essential cmake clang llvm libclang-dev \
    python3 python3-pip git

# Install Python packages
pip3 install pandas numpy scipy matplotlib

# Clone and build OptiWeave
git clone https://github.com/your-repo/OptiWeave.git
cd OptiWeave
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## RQ1: Instrumentation Overhead

### Step 1: Build OptiWeave

```bash
cd /path/to/OptiWeave
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
cd ..
```

### Step 2: Download Polybench/C

```bash
cd evaluation/benchmarks
wget http://web.cse.ohio-state.edu/~pouchet.2/software/polybench/polybench-c-4.2.1.tar.gz
tar xzf polybench-c-4.2.1.tar.gz
cd ../..
```

### Step 3: Run Overhead Benchmark

```bash
cd evaluation/scripts
./run_rq1_benchmarks.sh

# Results saved to: evaluation/results/rq1/overhead_results.csv
```

### Step 4: Analyze Results

```bash
python3 evaluation/scripts/comprehensive_analyze_rq1.py

# Outputs:
# - evaluation/results/rq1/analysis/comprehensive_summary.csv
# - evaluation/results/rq1/analysis/thesis_table_comprehensive.tex
# - evaluation/results/rq1/analysis/thesis_table_comprehensive.md
```

### Expected Results

| Metric | Expected Value | Acceptable Range |
|--------|----------------|------------------|
| Mean overhead (array-only) | ~4.80% | 2-10% |
| Median overhead | ~0.00% | -5% to 5% |
| Benchmarks <10% overhead | ~77% | 65-90% |

---

## RQ2: Bug Detection Effectiveness

### Step 1: Run Static Analysis

```bash
# Run OptiWeave analysis on test cases
cd evaluation/test_cases/rq2
for file in *.c; do
    ../../build/optiweave "$file" --detect-overflow -- -x c > "results/${file%.c}_optiweave.txt" 2>&1
done

# Run Cppcheck for comparison
for file in *.c; do
    cppcheck --enable=all "$file" 2> "results/${file%.c}_cppcheck.txt"
done
```

### Step 2: Classify Results

```bash
cd evaluation/results/rq2
# Edit classified_results.csv to mark each finding as:
# - ground_truth: TRUE_POSITIVE or TRUE_NEGATIVE
# - optiweave_result: DETECTED or MISSED
# - cppcheck_result: DETECTED or MISSED
```

### Step 3: Analyze Results

```bash
python3 evaluation/scripts/analyze_rq2_comprehensive.py

# Outputs:
# - evaluation/results/rq2/metrics_summary.csv
# - evaluation/results/rq2/thesis_table_rq2.tex
```

### Expected Results

| Metric | OptiWeave | Cppcheck |
|--------|-----------|----------|
| Recall | ~75.86% | ~41.38% |
| Precision | ~70.97% | ~70.59% |
| F1 Score | ~0.733 | ~0.522 |

---

## RQ3: Hybrid Analysis (json-c Case Study)

### Step 1: Clone json-c

```bash
cd evaluation/case_studies
git clone https://github.com/json-c/json-c.git
cd json-c
git checkout json-c-0.15
```

### Step 2: Run Static Analysis

```bash
../../build/optiweave *.c --detect-overflow -- -I. -x c > ../rq3_static.txt 2>&1
```

### Step 3: Build with Instrumentation

```bash
cd ..
mkdir json-c-instrumented
../../build/optiweave json-c/*.c \
    --array-subscripts \
    --arithmetic-ops \
    --output-dir=json-c-instrumented/ \
    -- -I json-c -x c

cd json-c-instrumented
gcc -c -O2 -DOPTIWEAVE_ENABLE_STATS -I../../templates -I../../include *.c
ar rcs libjson-c-instrumented.a *.o
```

### Step 4: Run Workload

```bash
# Run representative workload and collect statistics
OPTIWEAVE_STATS=1 ./test_json_parsing < test_inputs.json
```

### Step 5: Combine Analysis

```bash
python3 evaluation/scripts/analyze_rq3_hybrid.py \
    --static evaluation/case_studies/rq3_static.txt \
    --runtime evaluation/case_studies/rq3_runtime.txt \
    --output evaluation/results/rq3/
```

---

## RQ4: Optimization Guidance

### Step 1: Run Use Case Demonstrations

```bash
cd evaluation/instrumentation_use_cases

# Compile and run the demonstration program
g++ -std=c++20 -O2 -o use_case_demo use_case_demo.cpp
./use_case_demo > results_baseline.txt

# Transform with OptiWeave
../../build/optiweave use_case_demo.cpp \
    --array-subscripts \
    --arithmetic-ops \
    --output-dir=transformed/

# Compile instrumented version
g++ -std=c++20 -O2 \
    -DOPTIWEAVE_ENABLE_STATS \
    -I../../templates -I../../include \
    transformed/use_case_demo.cpp \
    ../../build/liboptiweave_runtime.a \
    -o use_case_demo_instrumented

# Run with statistics
OPTIWEAVE_STATS=1 ./use_case_demo_instrumented > results_instrumented.txt
```

### Step 2: Measure Optimization Speedups

The demo automatically measures:
- Loop interchange speedup (expected: ~8x)
- Cache optimization speedup (expected: ~49x)
- Algorithm optimization speedup (expected: ~3800x)

---

## Validation Checklist

Use this checklist to verify your reproduction:

### RQ1 Validation
- [ ] All 27 Polybench benchmarks complete
- [ ] Mean overhead within 2-10%
- [ ] CSV and LaTeX outputs generated
- [ ] Statistical tests produce p-values

### RQ2 Validation
- [ ] All test cases classified
- [ ] OptiWeave recall > 70%
- [ ] Wilson CIs calculated
- [ ] Comparison table generated

### RQ3 Validation
- [ ] json-c static analysis complete
- [ ] Instrumented build succeeds
- [ ] Hotspot prioritization output generated

### RQ4 Validation
- [ ] All 6 use cases run
- [ ] Speedups measured and within expected ranges
- [ ] Instrumented code compiles and runs

---

## Troubleshooting

### OptiWeave Build Fails

```bash
# Ensure LLVM is properly installed
llvm-config --version
clang --version

# Try specifying LLVM paths
cmake .. -DLLVM_DIR=$(llvm-config --cmakedir)
```

### Python Script Errors

```bash
# Install missing packages
pip3 install --upgrade pandas scipy numpy matplotlib

# Check Python version
python3 --version  # Should be 3.8+
```

### Polybench Benchmark Fails

```bash
# Ensure benchmarks are extracted
ls evaluation/benchmarks/polybench-c-4.2.1/

# Check permissions
chmod +x evaluation/scripts/*.sh
```

### Instrumented Code Won't Compile

```bash
# Check include paths
ls templates/optiweave_runtime.hpp  # Should exist
ls include/optiweave/               # Should contain headers

# Verify runtime library
ls build/liboptiweave_runtime.a     # Should exist
```

---

## Data Availability

All evaluation data is available in the repository:

| Data | Location |
|------|----------|
| Raw benchmark results | `evaluation/results/rq1/` |
| Bug detection results | `evaluation/results/rq2/` |
| Case study outputs | `evaluation/case_studies/` |
| Analysis scripts | `evaluation/scripts/` |
| Generated tables | `evaluation/results/*/analysis/` |

---

## Citation

If you use these evaluation results, please cite:

```bibtex
@mastersthesis{optiweave2026,
  title={OptiWeave: A Source-to-Source Instrumentation Framework for Selective Operator Analysis},
  author={[Author Name]},
  school={[University]},
  year={2026}
}
```

---

## Contact

For questions about reproduction:
- Repository: https://github.com/your-repo/OptiWeave
- Issues: https://github.com/your-repo/OptiWeave/issues
