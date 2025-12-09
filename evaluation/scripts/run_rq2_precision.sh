#!/bin/bash
# RQ2: Static Analysis Precision/Recall Evaluation
# Evaluates OptiWeave's bug detection capabilities

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
OPTIWEAVE_BIN="${SCRIPT_DIR}/../../build/optiweave"
RESULTS_DIR="${SCRIPT_DIR}/../results/rq2"
JULIET_DIR="${SCRIPT_DIR}/../benchmarks/juliet"

mkdir -p "$RESULTS_DIR"

echo "======================================================================="
echo "RQ2: Static Analysis Precision/Recall Evaluation"
echo "======================================================================="
echo "NOTE: This evaluation requires MANUAL ANALYSIS of results"
echo "      You must compare OptiWeave detections against ground truth"
echo "======================================================================="
echo ""

# CSV header
echo "test_case,ground_truth,optiweave_detected,cppcheck_detected,category" > "$RESULTS_DIR/precision_results.csv"

# Test overflow detection on synthetic test
if [ -f "$JULIET_DIR/overflow_test.cpp" ]; then
    echo "Testing integer overflow detection..."

    # Run OptiWeave
    "$OPTIWEAVE_BIN" "$JULIET_DIR/overflow_test.cpp" \
        --detect-overflow --verbose -- > "$RESULTS_DIR/optiweave_overflow.txt" 2>&1 || true

    # Run Cppcheck (baseline comparison)
    cppcheck --enable=all "$JULIET_DIR/overflow_test.cpp" 2> "$RESULTS_DIR/cppcheck_overflow.txt" || true

    echo "✓ Overflow detection complete"
fi

# Manual analysis required - create template
cat > "$RESULTS_DIR/manual_analysis_template.csv" << 'EOF'
# Manual Analysis Template
# Fill this in after reviewing OptiWeave and Cppcheck outputs
#
# ground_truth: 1=bug exists, 0=no bug
# detected: 1=tool detected, 0=tool missed
#
test_case,ground_truth,optiweave_detected,cppcheck_detected,category,notes
test_overflow_tp1,1,?,?,overflow,MAX_INT + 1
test_overflow_tp2,1,?,?,overflow,Loop accumulation overflow
test_overflow_fp1,0,?,?,overflow,Safe small addition
test_overflow_tn1,0,?,?,overflow,Obviously safe
# Add more test cases...
EOF

echo ""
echo "Results saved to: $RESULTS_DIR"
echo ""
echo "NEXT STEPS:"
echo "  1. Review OptiWeave output: $RESULTS_DIR/optiweave_overflow.txt"
echo "  2. Review Cppcheck output: $RESULTS_DIR/cppcheck_overflow.txt"
echo "  3. Fill in: $RESULTS_DIR/manual_analysis_template.csv"
echo "  4. Run: python3 evaluation/scripts/analyze_rq2.py"
echo ""
echo "For comprehensive evaluation, download Juliet Test Suite:"
echo "  https://samate.nist.gov/SARD/test-suites/112"
