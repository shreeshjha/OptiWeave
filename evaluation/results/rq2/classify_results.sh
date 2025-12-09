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
