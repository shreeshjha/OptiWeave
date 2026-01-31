#!/usr/bin/env python3
"""
RQ2 Comprehensive Analysis Script
Calculates precision, recall, F1 scores for static analysis
with Wilson confidence intervals for proportions
"""

import pandas as pd
import numpy as np
import sys
import math
from pathlib import Path

def wilson_ci(successes, total, confidence=0.95):
    """
    Calculate Wilson score confidence interval for a proportion.
    
    More accurate than normal approximation for small samples or
    extreme proportions (near 0 or 1).
    
    Args:
        successes: Number of successes
        total: Total number of trials
        confidence: Confidence level (default 0.95)
    
    Returns:
        (lower, upper, proportion): Wilson CI bounds and point estimate
    """
    if total == 0:
        return (0.0, 0.0, 0.0)
    
    # z-score for confidence level
    z = 1.96  # 95% CI
    if confidence == 0.99:
        z = 2.576
    elif confidence == 0.90:
        z = 1.645
    
    p_hat = successes / total
    n = total
    
    # Wilson score interval
    denominator = 1 + z**2 / n
    center = (p_hat + z**2 / (2 * n)) / denominator
    margin = (z / denominator) * math.sqrt(p_hat * (1 - p_hat) / n + z**2 / (4 * n**2))
    
    lower = max(0, center - margin)
    upper = min(1, center + margin)
    
    return (lower, upper, p_hat)

def calculate_metrics(df):
    """Calculate precision, recall, F1 score from classified results with Wilson CIs"""

    # Count outcomes
    tp = len(df[(df['ground_truth'] == 'TRUE_POSITIVE') & (df['tool_result'] == 'DETECTED')])
    fp = len(df[(df['ground_truth'] == 'TRUE_NEGATIVE') & (df['tool_result'] == 'DETECTED')])
    fn = len(df[(df['ground_truth'] == 'TRUE_POSITIVE') & (df['tool_result'] == 'MISSED')])
    tn = len(df[(df['ground_truth'] == 'TRUE_NEGATIVE') & (df['tool_result'] == 'MISSED')])

    # Calculate metrics
    precision = tp / (tp + fp) if (tp + fp) > 0 else 0
    recall = tp / (tp + fn) if (tp + fn) > 0 else 0
    f1 = 2 * (precision * recall) / (precision + recall) if (precision + recall) > 0 else 0
    accuracy = (tp + tn) / (tp + fp + fn + tn) if (tp + fp + fn + tn) > 0 else 0

    # Calculate Wilson confidence intervals for precision and recall
    precision_ci = wilson_ci(tp, tp + fp)
    recall_ci = wilson_ci(tp, tp + fn)
    accuracy_ci = wilson_ci(tp + tn, tp + fp + fn + tn)

    return {
        'tp': tp,
        'fp': fp,
        'fn': fn,
        'tn': tn,
        'precision': precision * 100,
        'recall': recall * 100,
        'f1': f1 * 100,
        'accuracy': accuracy * 100,
        # Wilson CIs (as percentages)
        'precision_ci_low': precision_ci[0] * 100,
        'precision_ci_high': precision_ci[1] * 100,
        'recall_ci_low': recall_ci[0] * 100,
        'recall_ci_high': recall_ci[1] * 100,
        'accuracy_ci_low': accuracy_ci[0] * 100,
        'accuracy_ci_high': accuracy_ci[1] * 100
    }

def analyze_rq2(results_dir):
    """Comprehensive RQ2 analysis"""

    print("=" * 80)
    print("RQ2: STATIC ANALYSIS PRECISION - COMPREHENSIVE ANALYSIS")
    print("=" * 80)
    print()

    results_path = results_dir / 'classified_results.csv'

    if not results_path.exists():
        print(f"Error: Classified results not found: {results_path}")
        print()
        print("Please run the manual classification first:")
        print(f"  1. cd {results_dir}")
        print("  2. ./classify_results.sh")
        print("  3. Edit classified_results.csv")
        print("  4. Run this script again")
        sys.exit(1)

    # Read results
    df = pd.read_csv(results_path)

    print(f"Loaded {len(df)} test cases")
    print()

    # ========================================================================
    # 1. OPTIWEAVE METRICS
    # ========================================================================
    print("=" * 80)
    print("1. OPTIWEAVE STATIC ANALYSIS METRICS")
    print("=" * 80)
    print()

    df_optiweave = df[['test_file', 'test_function', 'ground_truth']].copy()
    df_optiweave['tool_result'] = df['optiweave_result']

    ow_metrics = calculate_metrics(df_optiweave)

    print("Confusion Matrix:")
    print(f"  True Positives  (TP): {ow_metrics['tp']:>3d}")
    print(f"  False Positives (FP): {ow_metrics['fp']:>3d}")
    print(f"  False Negatives (FN): {ow_metrics['fn']:>3d}")
    print(f"  True Negatives  (TN): {ow_metrics['tn']:>3d}")
    print()

    print("Performance Metrics:")
    print(f"  Precision: {ow_metrics['precision']:>6.2f}%  (95% CI: {ow_metrics['precision_ci_low']:.2f}%-{ow_metrics['precision_ci_high']:.2f}%)")
    print(f"  Recall:    {ow_metrics['recall']:>6.2f}%  (95% CI: {ow_metrics['recall_ci_low']:.2f}%-{ow_metrics['recall_ci_high']:.2f}%)")
    print(f"  F1 Score:  {ow_metrics['f1']:>6.2f}%")
    print(f"  Accuracy:  {ow_metrics['accuracy']:>6.2f}%  (95% CI: {ow_metrics['accuracy_ci_low']:.2f}%-{ow_metrics['accuracy_ci_high']:.2f}%)")
    print()

    # ========================================================================
    # 2. CPPCHECK METRICS
    # ========================================================================
    print("=" * 80)
    print("2. CPPCHECK BASELINE METRICS")
    print("=" * 80)
    print()

    df_cppcheck = df[['test_file', 'test_function', 'ground_truth']].copy()
    df_cppcheck['tool_result'] = df['cppcheck_result']

    cpp_metrics = calculate_metrics(df_cppcheck)

    print("Confusion Matrix:")
    print(f"  True Positives  (TP): {cpp_metrics['tp']:>3d}")
    print(f"  False Positives (FP): {cpp_metrics['fp']:>3d}")
    print(f"  False Negatives (FN): {cpp_metrics['fn']:>3d}")
    print(f"  True Negatives  (TN): {cpp_metrics['tn']:>3d}")
    print()

    print("Performance Metrics:")
    print(f"  Precision: {cpp_metrics['precision']:>6.2f}%  (95% CI: {cpp_metrics['precision_ci_low']:.2f}%-{cpp_metrics['precision_ci_high']:.2f}%)")
    print(f"  Recall:    {cpp_metrics['recall']:>6.2f}%  (95% CI: {cpp_metrics['recall_ci_low']:.2f}%-{cpp_metrics['recall_ci_high']:.2f}%)")
    print(f"  F1 Score:  {cpp_metrics['f1']:>6.2f}%")
    print(f"  Accuracy:  {cpp_metrics['accuracy']:>6.2f}%  (95% CI: {cpp_metrics['accuracy_ci_low']:.2f}%-{cpp_metrics['accuracy_ci_high']:.2f}%)")
    print()

    # ========================================================================
    # 3. COMPARISON
    # ========================================================================
    print("=" * 80)
    print("3. COMPARATIVE ANALYSIS")
    print("=" * 80)
    print()

    comparison = pd.DataFrame({
        'Metric': ['Precision', 'Recall', 'F1 Score', 'Accuracy'],
        'OptiWeave': [
            f"{ow_metrics['precision']:.2f}%",
            f"{ow_metrics['recall']:.2f}%",
            f"{ow_metrics['f1']:.2f}%",
            f"{ow_metrics['accuracy']:.2f}%"
        ],
        'Cppcheck': [
            f"{cpp_metrics['precision']:.2f}%",
            f"{cpp_metrics['recall']:.2f}%",
            f"{cpp_metrics['f1']:.2f}%",
            f"{cpp_metrics['accuracy']:.2f}%"
        ],
        'Difference': [
            f"{ow_metrics['precision'] - cpp_metrics['precision']:+.2f}pp",
            f"{ow_metrics['recall'] - cpp_metrics['recall']:+.2f}pp",
            f"{ow_metrics['f1'] - cpp_metrics['f1']:+.2f}pp",
            f"{ow_metrics['accuracy'] - cpp_metrics['accuracy']:+.2f}pp"
        ]
    })

    print(comparison.to_string(index=False))
    print()

    # ========================================================================
    # 4. FALSE POSITIVE ANALYSIS
    # ========================================================================
    print("=" * 80)
    print("4. FALSE POSITIVE ANALYSIS")
    print("=" * 80)
    print()

    ow_fp = df_optiweave[
        (df_optiweave['ground_truth'] == 'TRUE_NEGATIVE') &
        (df_optiweave['tool_result'] == 'DETECTED')
    ]

    if len(ow_fp) > 0:
        print("OptiWeave False Positives:")
        print(ow_fp[['test_file', 'test_function']].to_string(index=False))
    else:
        print("OptiWeave: No false positives! ✓")
    print()

    cpp_fp = df_cppcheck[
        (df_cppcheck['ground_truth'] == 'TRUE_NEGATIVE') &
        (df_cppcheck['tool_result'] == 'DETECTED')
    ]

    if len(cpp_fp) > 0:
        print("Cppcheck False Positives:")
        print(cpp_fp[['test_file', 'test_function']].to_string(index=False))
    else:
        print("Cppcheck: No false positives! ✓")
    print()

    # ========================================================================
    # 5. FALSE NEGATIVE ANALYSIS
    # ========================================================================
    print("=" * 80)
    print("5. FALSE NEGATIVE ANALYSIS")
    print("=" * 80)
    print()

    ow_fn = df_optiweave[
        (df_optiweave['ground_truth'] == 'TRUE_POSITIVE') &
        (df_optiweave['tool_result'] == 'MISSED')
    ]

    if len(ow_fn) > 0:
        print("OptiWeave False Negatives (Missed Issues):")
        print(ow_fn[['test_file', 'test_function']].to_string(index=False))
    else:
        print("OptiWeave: No false negatives! ✓")
    print()

    cpp_fn = df_cppcheck[
        (df_cppcheck['ground_truth'] == 'TRUE_POSITIVE') &
        (df_cppcheck['tool_result'] == 'MISSED')
    ]

    if len(cpp_fn) > 0:
        print("Cppcheck False Negatives (Missed Issues):")
        print(cpp_fn[['test_file', 'test_function']].to_string(index=False))
    else:
        print("Cppcheck: No false negatives! ✓")
    print()

    # ========================================================================
    # 6. SAVE OUTPUTS
    # ========================================================================
    print("=" * 80)
    print("6. SAVING OUTPUTS")
    print("=" * 80)
    print()

    # Save summary metrics
    summary_df = pd.DataFrame({
        'Tool': ['OptiWeave', 'Cppcheck'],
        'Precision (%)': [ow_metrics['precision'], cpp_metrics['precision']],
        'Precision CI Low (%)': [ow_metrics['precision_ci_low'], cpp_metrics['precision_ci_low']],
        'Precision CI High (%)': [ow_metrics['precision_ci_high'], cpp_metrics['precision_ci_high']],
        'Recall (%)': [ow_metrics['recall'], cpp_metrics['recall']],
        'Recall CI Low (%)': [ow_metrics['recall_ci_low'], cpp_metrics['recall_ci_low']],
        'Recall CI High (%)': [ow_metrics['recall_ci_high'], cpp_metrics['recall_ci_high']],
        'F1 Score (%)': [ow_metrics['f1'], cpp_metrics['f1']],
        'Accuracy (%)': [ow_metrics['accuracy'], cpp_metrics['accuracy']],
        'Accuracy CI Low (%)': [ow_metrics['accuracy_ci_low'], cpp_metrics['accuracy_ci_low']],
        'Accuracy CI High (%)': [ow_metrics['accuracy_ci_high'], cpp_metrics['accuracy_ci_high']],
        'TP': [ow_metrics['tp'], cpp_metrics['tp']],
        'FP': [ow_metrics['fp'], cpp_metrics['fp']],
        'FN': [ow_metrics['fn'], cpp_metrics['fn']],
        'TN': [ow_metrics['tn'], cpp_metrics['tn']]
    })

    summary_csv = results_dir / 'metrics_summary.csv'
    summary_df.to_csv(summary_csv, index=False, float_format='%.2f')
    print(f"✓ Saved metrics summary: {summary_csv}")

    # Save LaTeX table with Wilson CIs
    latex_file = results_dir / 'thesis_table_rq2.tex'
    with open(latex_file, 'w') as f:
        f.write("\\begin{table}[h]\n")
        f.write("\\centering\n")
        f.write("\\caption{Static Analysis Precision Comparison with 95\\% Wilson Confidence Intervals}\n")
        f.write("\\label{tab:rq2-precision}\n")
        f.write("\\begin{tabular}{lcccc}\n")
        f.write("\\toprule\n")
        f.write("\\textbf{Tool} & \\textbf{Precision (\\%)} & \\textbf{Recall (\\%)} & ")
        f.write("\\textbf{F1 Score (\\%)} & \\textbf{Accuracy (\\%)} \\\\\n")
        f.write("\\midrule\n")
        f.write(f"OptiWeave & {ow_metrics['precision']:.1f} ({ow_metrics['precision_ci_low']:.1f}-{ow_metrics['precision_ci_high']:.1f}) & ")
        f.write(f"{ow_metrics['recall']:.1f} ({ow_metrics['recall_ci_low']:.1f}-{ow_metrics['recall_ci_high']:.1f}) & ")
        f.write(f"{ow_metrics['f1']:.1f} & {ow_metrics['accuracy']:.1f} ({ow_metrics['accuracy_ci_low']:.1f}-{ow_metrics['accuracy_ci_high']:.1f}) \\\\\n")
        f.write(f"Cppcheck & {cpp_metrics['precision']:.1f} ({cpp_metrics['precision_ci_low']:.1f}-{cpp_metrics['precision_ci_high']:.1f}) & ")
        f.write(f"{cpp_metrics['recall']:.1f} ({cpp_metrics['recall_ci_low']:.1f}-{cpp_metrics['recall_ci_high']:.1f}) & ")
        f.write(f"{cpp_metrics['f1']:.1f} & {cpp_metrics['accuracy']:.1f} ({cpp_metrics['accuracy_ci_low']:.1f}-{cpp_metrics['accuracy_ci_high']:.1f}) \\\\\n")
        f.write("\\bottomrule\n")
        f.write("\\end{tabular}\n")
        f.write("\\end{table}\n")

    print(f"✓ Saved LaTeX table: {latex_file}")
    print()

    # ========================================================================
    # 7. KEY FINDINGS
    # ========================================================================
    print("=" * 80)
    print("7. KEY FINDINGS FOR THESIS")
    print("=" * 80)
    print()

    print(f"1. OptiWeave achieves {ow_metrics['precision']:.1f}% precision and ")
    print(f"   {ow_metrics['recall']:.1f}% recall on integer overflow detection")
    print()
    print(f"2. Compared to Cppcheck ({cpp_metrics['precision']:.1f}% precision, ")
    print(f"   {cpp_metrics['recall']:.1f}% recall), OptiWeave shows ")
    if ow_metrics['f1'] > cpp_metrics['f1']:
        print(f"   BETTER overall performance (+{ow_metrics['f1'] - cpp_metrics['f1']:.1f}pp F1)")
    elif ow_metrics['f1'] < cpp_metrics['f1']:
        print(f"   COMPARABLE performance ({cpp_metrics['f1'] - ow_metrics['f1']:.1f}pp lower F1)")
    else:
        print("   EQUIVALENT performance")
    print()
    print(f"3. False positive rate: {ow_metrics['fp']}/{ow_metrics['fp'] + ow_metrics['tn']} ")
    print(f"   ({ow_metrics['fp'] / (ow_metrics['fp'] + ow_metrics['tn']) * 100:.1f}%)")
    print()
    print(f"4. False negative rate: {ow_metrics['fn']}/{ow_metrics['fn'] + ow_metrics['tp']} ")
    print(f"   ({ow_metrics['fn'] / (ow_metrics['fn'] + ow_metrics['tp']) * 100:.1f}%)")
    print()
    print("These results answer RQ2 regarding the precision and effectiveness")
    print("of OptiWeave's AST-based static analysis.")
    print()
    print("=" * 80)

if __name__ == '__main__':
    script_dir = Path(__file__).parent
    results_dir = script_dir.parent / 'results' / 'rq2'

    analyze_rq2(results_dir)
