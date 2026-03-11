
╔══════════════════════════════════════════════════════════════╗
║        OptiWeave Code Complexity Analysis                   ║
╚══════════════════════════════════════════════════════════════╝

Project Statistics:
  Total Functions: 14
  Total Lines: 149
  Total SLOC: 149
  Complex Functions (CC > 20): 0
  Very Complex Functions (CC > 50): 0

Average Metrics:
  Cyclomatic Complexity: 2.29
  Cognitive Complexity: 1.86
  Maintainability Index: 99.23

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🔥 Most Complex Functions (by Cyclomatic Complexity)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

┌────┬─────────────────────────────┬─────────┬─────────┬──────────┬────────────┐
│ #  │ Function                    │ CC      │ CogC    │ MI       │ Risk       │
├────┼─────────────────────────────┼─────────┼─────────┼──────────┼────────────┤
│  1 │ matmul_naive                │       4 │       6 │    100.0 │ Low        │
│ 2  │ column_sum                  │       3 │       3 │    100.0 │ Low        │
│ 3  │ main                        │       3 │       3 │     89.2 │ Low        │
│ 4  │ pairwise_dist               │       3 │       3 │    100.0 │ Low        │
│ 5  │ rgba_to_gray                │       3 │       3 │    100.0 │ Low        │
│ 6  │ transpose                   │       3 │       3 │    100.0 │ Low        │
│ 7  │ accumulate                  │       2 │       1 │    100.0 │ Low        │
│ 8  │ gather_strided              │       2 │       1 │    100.0 │ Low        │
│ 9  │ normalize                   │       2 │       1 │    100.0 │ Low        │
│ 10 │ sparse_sum                  │       2 │       1 │    100.0 │ Low        │
└────┴─────────────────────────────┴─────────┴─────────┴──────────┴────────────┘

Legend:
  CC   = Cyclomatic Complexity (decision points)
  CogC = Cognitive Complexity (understandability)
  MI   = Maintainability Index (0-100, higher is better)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
⚠️  Least Maintainable Functions
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Function: main
  Location: /Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c:181
  Maintainability Index: 89.2 (Highly Maintainable)
  Cyclomatic Complexity: 3 (Simple)
  Lines of Code: 64

Function: accumulate
  Location: /Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c:69
  Maintainability Index: 100.0 (Highly Maintainable)
  Cyclomatic Complexity: 2 (Simple)
  Lines of Code: 7

Function: alloc_vector
  Location: /Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c:99
  Maintainability Index: 100.0 (Highly Maintainable)
  Cyclomatic Complexity: 1 (Simple)
  Lines of Code: 3

Function: column_sum
  Location: /Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c:86
  Maintainability Index: 100.0 (Highly Maintainable)
  Cyclomatic Complexity: 3 (Simple)
  Lines of Code: 9

Function: count_mismatches
  Location: /Users/shreesh/Dev/Github/OptiWeave/demo/src/matrix.c:77
  Maintainability Index: 100.0 (Highly Maintainable)
  Cyclomatic Complexity: 1 (Simple)
  Lines of Code: 3

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🧠 Hardest to Understand Functions (by Cognitive Complexity)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Function: matmul_naive
  Cognitive Complexity: 6 (Easy)
  Max Nesting Depth: 3
  Decision Points: 3

Function: column_sum
  Cognitive Complexity: 3 (Very Easy)
  Max Nesting Depth: 2
  Decision Points: 2

Function: main
  Cognitive Complexity: 3 (Very Easy)
  Max Nesting Depth: 2
  Decision Points: 2

Function: pairwise_dist
  Cognitive Complexity: 3 (Very Easy)
  Max Nesting Depth: 2
  Decision Points: 2

Function: rgba_to_gray
  Cognitive Complexity: 3 (Very Easy)
  Max Nesting Depth: 2
  Decision Points: 2

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📊 Summary & Recommendations
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

✅ Good code structure!
   → No circular dependencies detected
   → No extremely complex functions

