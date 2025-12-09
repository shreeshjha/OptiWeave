#!/bin/bash
# Setup script for Juliet Test Suite

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BENCH_DIR="${SCRIPT_DIR}/../benchmarks/juliet"

echo "Setting up Juliet Test Suite for OptiWeave evaluation..."

cd "$BENCH_DIR"

echo "Download Juliet Test Suite from:"
echo "https://samate.nist.gov/SARD/test-suites/112"
echo ""
echo "For this evaluation, focus on:"
echo "  - CWE-190: Integer Overflow"
echo "  - CWE-457: Use of Uninitialized Variable"
echo ""
echo "Extract relevant test cases to: $BENCH_DIR"
echo ""
echo "Alternative: Create synthetic test cases based on known patterns"

# Create a simple synthetic test for demonstration
cat > overflow_test.cpp << 'EOF'
// Synthetic integer overflow test cases
#include <iostream>
#include <limits>

// TRUE POSITIVE: Definite overflow
int test_overflow_tp1() {
    int a = std::numeric_limits<int>::max();
    int b = 1;
    return a + b;  // OptiWeave should detect this
}

// TRUE POSITIVE: Potential overflow in loop
int test_overflow_tp2(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += 1000000;  // OptiWeave should warn
    }
    return sum;
}

// FALSE POSITIVE CANDIDATE: Safe operation
int test_overflow_fp1() {
    int a = 100;
    int b = 200;
    return a + b;  // Should NOT detect
}

// TRUE NEGATIVE: Obviously safe
int test_overflow_tn1() {
    int a = 1;
    int b = 1;
    return a + b;
}

int main() {
    std::cout << "Overflow test cases" << std::endl;
    return 0;
}
EOF

echo "Created synthetic test: overflow_test.cpp"
echo "Juliet setup complete!"
