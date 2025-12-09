#!/usr/bin/env python3
"""
RQ2 Analysis Script - Static Analysis Precision/Recall
Calculates precision, recall, and F1 scores for bug detection
"""

import pandas as pd
import sys
from pathlib import Path

def calculate_metrics(df, tool_column):
    """Calculate precision, recall, F1 for a given tool"""

    # Confusion matrix
    tp = len(df[(df['ground_truth'] == 1) & (df[tool_column] == 1)])  # True Positives
    fp = len(df[(df['ground_truth'] == 0) & (df[tool_column] == 1)])  # False Positives
    tn = len(df[(df['ground_truth'] == 0) & (df[tool_column] == 0)])  # True Negatives
    fn = len(df[(df['ground_truth'] == 1) & (df[tool_column] == 0)])  # False Negatives

    # Metrics
    precision = tp / (tp + fp) if (tp + fp) > 0 else 0
    recall = tp / (tp + fn) if (tp + fn) > 0 else 0
    f1 = 2 * (precision * recall) / (precision + recall) if (precision + recall) > 0 else 0

    return {
        'TP': tp,
        'FP': fp,
        'TN': tn,
        'FN': fn,
        'Precision': precision,
        'Recall': recall,
        'F1': f1
    }

def analyze_precision_recall(csv_path):
    """Analyze precision/recall results"""

    print("=" * 70)
    print("RQ2: Static Analysis Precision/Recall Analysis")
    print("=" * 70)
    print()

    # Read results
    df = pd.read_csv(csv_path, comment='#')

    # Remove rows with missing detections (marked with ?)
    df = df[df['optiweave_detected'] != '?']
    df = df[df['cppcheck_detected'] != '?']

    # Convert to int
    df['ground_truth'] = df['ground_truth'].astype(int)
    df['optiweave_detected'] = df['optiweave_detected'].astype(int)
    df['cppcheck_detected'] = df['cppcheck_detected'].astype(int)

    if len(df) == 0:
        print("Error: No completed test cases found.")
        print("Please fill in the manual_analysis_template.csv")
        return

    print(f"Total test cases: {len(df)}")
    print(f"Bugs (ground truth): {sum(df['ground_truth'])}")
    print(f"No bugs (ground truth): {len(df) - sum(df['ground_truth'])}")
    print()

    # Calculate metrics for OptiWeave
    print("=" * 70)
    print("OptiWeave Results:")
    print("=" * 70)
    ow_metrics = calculate_metrics(df, 'optiweave_detected')
    for key, value in ow_metrics.items():
        if isinstance(value, int):
            print(f"{key:12s}: {value}")
        else:
            print(f"{key:12s}: {value:.3f}")

    print()

    # Calculate metrics for Cppcheck
    print("=" * 70)
    print("Cppcheck Results:")
    print("=" * 70)
    cpp_metrics = calculate_metrics(df, 'cppcheck_detected')
    for key, value in cpp_metrics.items():
        if isinstance(value, int):
            print(f"{key:12s}: {value}")
        else:
            print(f"{key:12s}: {value:.3f}")

    print()

    # Comparison table
    print("=" * 70)
    print("Thesis Table: Precision/Recall Comparison")
    print("=" * 70)

    comparison = pd.DataFrame({
        'OptiWeave': [ow_metrics['Precision'], ow_metrics['Recall'], ow_metrics['F1']],
        'Cppcheck': [cpp_metrics['Precision'], cpp_metrics['Recall'], cpp_metrics['F1']]
    }, index=['Precision', 'Recall', 'F1 Score'])

    print(comparison.to_string())
    print()

    # Save results
    output_dir = Path(csv_path).parent
    comparison.to_csv(output_dir / 'precision_recall_summary.csv')

    print(f"✓ Saved summary to: {output_dir / 'precision_recall_summary.csv'}")
    print()

    # Key findings
    print("=" * 70)
    print("KEY FINDINGS for Thesis:")
    print("=" * 70)
    print(f"1. OptiWeave achieves {ow_metrics['Precision']:.1%} precision (FP rate: {ow_metrics['FP']}/{ow_metrics['FP']+ow_metrics['TP']})")
    print(f"2. OptiWeave achieves {ow_metrics['Recall']:.1%} recall (missed {ow_metrics['FN']} bugs)")
    print(f"3. F1 score: {ow_metrics['F1']:.3f} (vs Cppcheck: {cpp_metrics['F1']:.3f})")
    print()
    print("These results answer RQ2 regarding AST-level static analysis")
    print("effectiveness compared to established tools.")
    print("=" * 70)

if __name__ == '__main__':
    results_path = Path(__file__).parent.parent / 'results' / 'rq2' / 'manual_analysis_template.csv'

    if not results_path.exists():
        print(f"Error: Results file not found: {results_path}")
        print("Run evaluation/scripts/run_rq2_precision.sh first")
        sys.exit(1)

    analyze_precision_recall(results_path)
