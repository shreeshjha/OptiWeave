# Juliet Test Suite Subset for OptiWeave Evaluation

This directory contains focused test cases for evaluating OptiWeave's static analysis.

## Test Coverage

### CWE-190: Integer Overflow (15+ test cases)
- Addition overflow (8 cases)
- Multiplication overflow (4 cases)  
- Subtraction underflow (3 cases)

## Ground Truth Labels

Each test case is labeled with expected behavior:
- **OVERFLOW**: True positive (should detect)
- **SAFE**: True negative (should not detect)
- **POTENTIAL OVERFLOW**: Conservative warning acceptable

## Running Evaluation

```bash
# Analyze all test files
../../build/optiweave juliet_subset/CWE190_Integer_Overflow/*.c --detect-overflow --

# Compare with Cppcheck
cppcheck --enable=all juliet_subset/CWE190_Integer_Overflow/*.c
```

## Expected Results

OptiWeave is designed for high recall (catch all bugs) at the cost of precision (some false positives on safe code). This is appropriate for security-critical analysis.
