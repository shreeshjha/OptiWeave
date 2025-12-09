#!/usr/bin/env python3
"""
Automatic classification of RQ2 results based on ground truth and tool outputs
"""

import re
from pathlib import Path
import pandas as pd

def parse_optiweave_output(file_path):
    """Parse OptiWeave overflow detection output"""
    detected_functions = set()

    if not file_path.exists():
        return detected_functions

    with open(file_path, 'r') as f:
        content = f.read()
        # Extract function names from location lines
        # Format: Location: ...file:line:col (in function_name)
        matches = re.findall(r'\(in (\w+)\)', content)
        detected_functions.update(matches)

    return detected_functions

def parse_cppcheck_output(file_path):
    """Parse Cppcheck output"""
    detected_functions = set()

    if not file_path.exists():
        return detected_functions

    with open(file_path, 'r') as f:
        content = f.read()
        # Cppcheck format varies, try to extract function references
        # This is a simplified parser
        lines = content.split('\n')
        for line in lines:
            if 'error' in line.lower() or 'warning' in line.lower():
                # Try to extract function context
                match = re.search(r'(\w+)\s*\(', line)
                if match:
                    detected_functions.add(match.group(1))

    return detected_functions

def auto_classify(results_dir):
    """Automatically classify results"""

    print("=" * 80)
    print("RQ2: AUTOMATIC CLASSIFICATION")
    print("=" * 80)
    print()

    # Load ground truth
    gt_path = results_dir / 'ground_truth.csv'
    if not gt_path.exists():
        print(f"Error: Ground truth not found: {gt_path}")
        return

    gt_df = pd.read_csv(gt_path)

    print(f"Loaded {len(gt_df)} test cases from ground truth")
    print()

    # Classify each test case
    results = []

    for _, row in gt_df.iterrows():
        test_file = row['test_file']
        test_function = row['test_function']
        expected = row['expected_result']

        # Find corresponding output files
        ow_file = results_dir / f"optiweave_{test_file}.txt"
        cpp_file = results_dir / f"cppcheck_{test_file}.txt"

        # Check OptiWeave detection
        ow_detected_funcs = parse_optiweave_output(ow_file)
        ow_result = 'DETECTED' if test_function in ow_detected_funcs else 'MISSED'

        # Check Cppcheck detection
        cpp_detected_funcs = parse_cppcheck_output(cpp_file)
        cpp_result = 'DETECTED' if test_function in cpp_detected_funcs else 'MISSED'

        # Keep DETECTED/MISSED - metrics calculator will determine if it's FP/FN

        results.append({
            'test_file': test_file,
            'test_function': test_function,
            'optiweave_result': ow_result,
            'cppcheck_result': cpp_result,
            'ground_truth': expected
        })

        # Print classification
        ow_symbol = "✓" if (ow_result == 'DETECTED' and expected == 'TRUE_POSITIVE') or \
                          (ow_result == 'MISSED' and expected == 'TRUE_NEGATIVE') else "✗"
        cpp_symbol = "✓" if (cpp_result == 'DETECTED' and expected == 'TRUE_POSITIVE') or \
                           (cpp_result == 'MISSED' and expected == 'TRUE_NEGATIVE') else "✗"

        print(f"{ow_symbol} {test_function:40s} | OW: {ow_result:15s} | CPP: {cpp_result:15s} | GT: {expected}")

    print()

    # Save classified results
    results_df = pd.DataFrame(results)
    output_path = results_dir / 'classified_results.csv'
    results_df.to_csv(output_path, index=False)

    print(f"✓ Saved classified results: {output_path}")
    print()

    # Quick summary
    print("=" * 80)
    print("CLASSIFICATION SUMMARY")
    print("=" * 80)
    print()

    ow_correct = sum(1 for r in results if
                     (r['optiweave_result'] == 'DETECTED' and r['ground_truth'] == 'TRUE_POSITIVE') or
                     (r['optiweave_result'] == 'MISSED' and r['ground_truth'] == 'TRUE_NEGATIVE'))

    cpp_correct = sum(1 for r in results if
                      (r['cppcheck_result'] == 'DETECTED' and r['ground_truth'] == 'TRUE_POSITIVE') or
                      (r['cppcheck_result'] == 'MISSED' and r['ground_truth'] == 'TRUE_NEGATIVE'))

    print(f"OptiWeave: {ow_correct}/{len(results)} correct classifications ({ow_correct/len(results)*100:.1f}%)")
    print(f"Cppcheck:  {cpp_correct}/{len(results)} correct classifications ({cpp_correct/len(results)*100:.1f}%)")
    print()

    print("Ready for full analysis! Run:")
    print("  python3 analyze_rq2_comprehensive.py")
    print()

if __name__ == '__main__':
    script_dir = Path(__file__).parent
    results_dir = script_dir.parent / 'results' / 'rq2'

    auto_classify(results_dir)
