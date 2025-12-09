#!/bin/bash
# RQ2: Static Analysis Precision - Comprehensive Evaluation
# Tests OptiWeave static analysis against known test cases

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
OPTIWEAVE_BIN="${SCRIPT_DIR}/../../build/optiweave"
RESULTS_DIR="${SCRIPT_DIR}/../results/rq2"
JULIET_DIR="${SCRIPT_DIR}/../benchmarks/juliet/juliet_subset"

mkdir -p "$RESULTS_DIR"

echo "================================================================="
echo "RQ2: Static Analysis Precision Evaluation"
echo "================================================================="
echo ""
echo "Testing OptiWeave's static analysis capabilities:"
echo "  - Integer overflow detection (CWE-190)"
echo "  - Floating-point precision warnings"
echo "  - Data flow analysis"
echo ""

# Test results file
RESULTS_CSV="$RESULTS_DIR/detailed_results.csv"
echo "test_file,category,test_name,expected,optiweave_detected,cppcheck_detected,notes" > "$RESULTS_CSV"

# =========================================================================
# TEST 1: Integer Overflow Detection (CWE-190)
# =========================================================================
echo "================================================================="
echo "TEST 1: Integer Overflow Detection"
echo "================================================================="
echo ""

OVERFLOW_DIR="$JULIET_DIR/CWE190_Integer_Overflow"

if [ -d "$OVERFLOW_DIR" ]; then
    echo "Testing overflow detection on Juliet CWE-190 samples..."

    for test_file in "$OVERFLOW_DIR"/*.c; do
        if [ -f "$test_file" ]; then
            filename=$(basename "$test_file")
            echo "  Testing: $filename"

            # Run OptiWeave overflow detection
            "$OPTIWEAVE_BIN" "$test_file" --detect-overflow \
                --overflow-output="$RESULTS_DIR/optiweave_${filename}.txt" \
                --overflow-format=text -- 2>/dev/null || true

            # Run Cppcheck for comparison
            cppcheck --enable=all --inconclusive "$test_file" \
                2>"$RESULTS_DIR/cppcheck_${filename}.txt" >/dev/null || true

            echo "    ✓ Analysis complete"
        fi
    done

    # Also test C++ versions
    for test_file in "$OVERFLOW_DIR"/*.cpp; do
        if [ -f "$test_file" ]; then
            filename=$(basename "$test_file")
            echo "  Testing: $filename"

            # Run OptiWeave overflow detection
            "$OPTIWEAVE_BIN" "$test_file" --detect-overflow \
                --overflow-output="$RESULTS_DIR/optiweave_${filename}.txt" \
                --overflow-format=text -- 2>/dev/null || true

            # Run Cppcheck for comparison
            cppcheck --enable=all --inconclusive --language=c++ "$test_file" \
                2>"$RESULTS_DIR/cppcheck_${filename}.txt" >/dev/null || true

            echo "    ✓ Analysis complete"
        fi
    done
else
    echo "  ⚠️  Overflow test directory not found: $OVERFLOW_DIR"
fi

echo ""

# =========================================================================
# TEST 2: Create Ground Truth Classification
# =========================================================================
echo "================================================================="
echo "Creating Ground Truth Classification Template"
echo "================================================================="
echo ""

cat > "$RESULTS_DIR/ground_truth.csv" << 'GTEOF'
test_file,test_function,category,expected_result,severity,notes
test_addition.c,test_clear_overflow_max_plus_one,overflow,TRUE_POSITIVE,CRITICAL,INT_MAX + 1 must overflow
test_addition.c,test_clear_overflow_max_plus_large,overflow,TRUE_POSITIVE,CRITICAL,INT_MAX + 1000 must overflow
test_addition.c,test_potential_overflow_unknown,overflow,TRUE_POSITIVE,WARNING,Unknown inputs may overflow
test_addition.c,test_loop_overflow,overflow,TRUE_POSITIVE,WARNING,Loop accumulation may overflow
test_addition.c,test_safe_small_constants,overflow,TRUE_NEGATIVE,INFO,10 + 20 is safe
test_addition.c,test_safe_negative,overflow,TRUE_NEGATIVE,INFO,-100 + 50 is safe
test_addition.c,test_overflow_near_boundary,overflow,TRUE_POSITIVE,CRITICAL,(INT_MAX-10) + 20 overflows
test_addition.c,test_safe_near_boundary,overflow,TRUE_NEGATIVE,INFO,(INT_MAX-100) + 50 is safe
test_multiplication.c,test_clear_overflow_max_times_two,overflow,TRUE_POSITIVE,CRITICAL,INT_MAX * 2 overflows
test_multiplication.c,test_overflow_large_numbers,overflow,TRUE_POSITIVE,CRITICAL,100000 * 100000 overflows
test_multiplication.c,test_safe_small_multiply,overflow,TRUE_NEGATIVE,INFO,10 * 20 is safe
test_subtraction.c,test_underflow_min_minus_one,overflow,TRUE_POSITIVE,CRITICAL,INT_MIN - 1 underflows
test_subtraction.c,test_safe_subtraction,overflow,TRUE_NEGATIVE,INFO,100 - 50 is safe
GTEOF

echo "✓ Ground truth template created: $RESULTS_DIR/ground_truth.csv"
echo ""

# =========================================================================
# TEST 3: Manual Classification Helper
# =========================================================================
echo "================================================================="
echo "Creating Manual Classification Helper"
echo "================================================================="
echo ""

cat > "$RESULTS_DIR/classify_results.sh" << 'CLASSEOF'
#!/bin/bash
# Helper script to classify OptiWeave and Cppcheck outputs

RESULTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_CSV="$RESULTS_DIR/classified_results.csv"

echo "test_file,test_function,optiweave_result,cppcheck_result,ground_truth" > "$OUTPUT_CSV"

echo "================================================================="
echo "Manual Classification Helper for RQ2"
echo "================================================================="
echo ""
echo "Instructions:"
echo "1. Review each test case output file in $RESULTS_DIR"
echo "2. For each test function, classify as:"
echo "   - DETECTED: Tool found the issue"
echo "   - MISSED: Tool did not find the issue"
echo "   - FALSE_POSITIVE: Tool reported issue where none exists"
echo ""
echo "3. Edit $OUTPUT_CSV with your classifications"
echo ""
echo "Expected format:"
echo "test_file,test_function,optiweave_result,cppcheck_result,ground_truth"
echo "test_addition.c,test_clear_overflow_max_plus_one,DETECTED,DETECTED,TRUE_POSITIVE"
echo ""
echo "After classification, run: python3 ../scripts/analyze_rq2_comprehensive.py"
echo ""
CLASSEOF

chmod +x "$RESULTS_DIR/classify_results.sh"

echo "✓ Classification helper created: $RESULTS_DIR/classify_results.sh"
echo ""

# =========================================================================
# SUMMARY
# =========================================================================
echo "================================================================="
echo "RQ2 Evaluation Complete!"
echo "================================================================="
echo ""
echo "Results saved to: $RESULTS_DIR"
echo ""
echo "Generated files:"
echo "  • optiweave_*.txt       - OptiWeave analysis outputs"
echo "  • cppcheck_*.txt        - Cppcheck analysis outputs"
echo "  • ground_truth.csv      - Expected results for each test"
echo "  • classify_results.sh   - Helper script for manual classification"
echo ""
echo "NEXT STEPS:"
echo ""
echo "1. Review the analysis outputs:"
echo "   cd $RESULTS_DIR"
echo "   ls -lh optiweave_*.txt cppcheck_*.txt"
echo ""
echo "2. Classify the results manually:"
echo "   ./classify_results.sh"
echo "   vim classified_results.csv"
echo ""
echo "3. Calculate precision/recall metrics:"
echo "   cd $SCRIPT_DIR"
echo "   python3 analyze_rq2_comprehensive.py"
echo ""
echo "================================================================="
