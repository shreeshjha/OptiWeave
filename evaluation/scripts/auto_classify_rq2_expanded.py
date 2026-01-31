#!/usr/bin/env python3
"""
Automatic classification of RQ2 EXPANDED results
Properly counts ALL Cppcheck error types: integerOverflow, zerodiv, shiftNegative, shiftTooManyBits
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

def get_function_at_line(source_file, target_line):
    """Find which function a given line number belongs to"""
    if not Path(source_file).exists():
        return None

    with open(source_file, 'r') as f:
        lines = f.readlines()

    current_function = None
    for i, line in enumerate(lines, 1):
        # Match function definitions: int function_name(...) {
        func_match = re.match(r'^\s*\w+\s+(\w+)\s*\([^)]*\)\s*\{', line)
        if func_match:
            current_function = func_match.group(1)

        if i == target_line:
            return current_function

    return current_function

def parse_cppcheck_output(file_path):
    """Parse Cppcheck output - COUNT ALL RELEVANT ERROR TYPES"""
    detected_functions = set()

    if not file_path.exists():
        return detected_functions

    with open(file_path, 'r') as f:
        content = f.read()

    lines = content.split('\n')
    for line in lines:
        # Match cppcheck error format with ANY of these tags:
        # [integerOverflow], [zerodiv], [shiftNegative], [shiftTooManyBits]
        if any(tag in line for tag in ['[integerOverflow]', '[zerodiv]', '[shiftNegative]', '[shiftTooManyBits]']):
            # Extract filename and line number
            # Format: /path/file.c:line:col: error: ... [tag]
            match = re.search(r'([^:]+\.c):(\d+):\d+:', line)
            if match:
                source_file = match.group(1)
                line_num = int(match.group(2))

                # Find the function at this line
                func_name = get_function_at_line(source_file, line_num)
                if func_name:
                    detected_functions.add(func_name)

    return detected_functions

def auto_classify_expanded():
    """Automatically classify expanded results"""

    script_dir = Path(__file__).parent
    results_dir = script_dir.parent / 'results' / 'rq2_expanded'

    print("=" * 80)
    print("RQ2 EXPANDED: AUTOMATIC CLASSIFICATION (UPDATED)")
    print("=" * 80)
    print()

    # Load ground truth
    gt_path = script_dir.parent / 'results' / 'rq2' / 'ground_truth_expanded.csv'
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

        # Check Cppcheck detection (NOW INCLUDES ALL ERROR TYPES!)
        cpp_detected_funcs = parse_cppcheck_output(cpp_file)
        cpp_result = 'DETECTED' if test_function in cpp_detected_funcs else 'MISSED'

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

        status = f"{ow_symbol}{cpp_symbol}"
        print(f"{status} {test_file:20s} {test_function:40s} | OW: {ow_result:8s} | CPP: {cpp_result:8s} | GT: {expected}")

    print()

    # Save classified results
    results_df = pd.DataFrame(results)
    output_path = results_dir / 'classified_results.csv'
    results_df.to_csv(output_path, index=False)

    print(f"✓ Saved classified results: {output_path}")
    print()

    # Calculate metrics
    print("=" * 80)
    print("EXPANDED METRICS (CORRECTED)")
    print("=" * 80)
    print()

    # OptiWeave metrics
    ow_tp = sum(1 for r in results if r['optiweave_result'] == 'DETECTED' and r['ground_truth'] == 'TRUE_POSITIVE')
    ow_fp = sum(1 for r in results if r['optiweave_result'] == 'DETECTED' and r['ground_truth'] == 'TRUE_NEGATIVE')
    ow_fn = sum(1 for r in results if r['optiweave_result'] == 'MISSED' and r['ground_truth'] == 'TRUE_POSITIVE')
    ow_tn = sum(1 for r in results if r['optiweave_result'] == 'MISSED' and r['ground_truth'] == 'TRUE_NEGATIVE')

    ow_precision = 100 * ow_tp / (ow_tp + ow_fp) if (ow_tp + ow_fp) > 0 else 0
    ow_recall = 100 * ow_tp / (ow_tp + ow_fn) if (ow_tp + ow_fn) > 0 else 0
    ow_f1 = 2 * (ow_precision * ow_recall) / (ow_precision + ow_recall) if (ow_precision + ow_recall) > 0 else 0
    ow_accuracy = 100 * (ow_tp + ow_tn) / len(results)

    # Cppcheck metrics
    cpp_tp = sum(1 for r in results if r['cppcheck_result'] == 'DETECTED' and r['ground_truth'] == 'TRUE_POSITIVE')
    cpp_fp = sum(1 for r in results if r['cppcheck_result'] == 'DETECTED' and r['ground_truth'] == 'TRUE_NEGATIVE')
    cpp_fn = sum(1 for r in results if r['cppcheck_result'] == 'MISSED' and r['ground_truth'] == 'TRUE_POSITIVE')
    cpp_tn = sum(1 for r in results if r['cppcheck_result'] == 'MISSED' and r['ground_truth'] == 'TRUE_NEGATIVE')

    cpp_precision = 100 * cpp_tp / (cpp_tp + cpp_fp) if (cpp_tp + cpp_fp) > 0 else 0
    cpp_recall = 100 * cpp_tp / (cpp_tp + cpp_fn) if (cpp_tp + cpp_fn) > 0 else 0
    cpp_f1 = 2 * (cpp_precision * cpp_recall) / (cpp_precision + cpp_recall) if (cpp_precision + cpp_recall) > 0 else 0
    cpp_accuracy = 100 * (cpp_tp + cpp_tn) / len(results)

    print("OptiWeave:")
    print(f"  TP={ow_tp}, FP={ow_fp}, FN={ow_fn}, TN={ow_tn}")
    print(f"  Precision: {ow_precision:.2f}%")
    print(f"  Recall:    {ow_recall:.2f}%")
    print(f"  F1 Score:  {ow_f1:.2f}%")
    print(f"  Accuracy:  {ow_accuracy:.2f}%")
    print()

    print("Cppcheck:")
    print(f"  TP={cpp_tp}, FP={cpp_fp}, FN={cpp_fn}, TN={cpp_tn}")
    print(f"  Precision: {cpp_precision:.2f}%")
    print(f"  Recall:    {cpp_recall:.2f}%")
    print(f"  F1 Score:  {cpp_f1:.2f}%")
    print(f"  Accuracy:  {cpp_accuracy:.2f}%")
    print()

    # Save metrics
    metrics = {
        'Tool': ['OptiWeave', 'Cppcheck'],
        'Precision (%)': [ow_precision, cpp_precision],
        'Recall (%)': [ow_recall, cpp_recall],
        'F1 Score (%)': [ow_f1, cpp_f1],
        'Accuracy (%)': [ow_accuracy, cpp_accuracy],
        'TP': [ow_tp, cpp_tp],
        'FP': [ow_fp, cpp_fp],
        'FN': [ow_fn, cpp_fn],
        'TN': [ow_tn, cpp_tn]
    }

    metrics_df = pd.DataFrame(metrics)
    metrics_path = results_dir / 'metrics_summary.csv'
    metrics_df.to_csv(metrics_path, index=False)

    print(f"✓ Saved metrics: {metrics_path}")
    print()

    print("=" * 80)
    print(f"COMPARISON: Original (13 tests) vs Expanded ({len(results)} tests)")
    print("=" * 80)
    print()
    print("Original OptiWeave:  60.00% precision, 75.00% recall, 66.67% F1")
    print(f"Expanded OptiWeave:  {ow_precision:.2f}% precision, {ow_recall:.2f}% recall, {ow_f1:.2f}% F1")
    print()
    print("Original Cppcheck:  100.00% precision, 50.00% recall, 66.67% F1")
    print(f"Expanded Cppcheck:  {cpp_precision:.2f}% precision, {cpp_recall:.2f}% recall, {cpp_f1:.2f}% F1")
    print()

if __name__ == '__main__':
    auto_classify_expanded()
