#include <optiweave/prelude.hpp>
// CWE-190: Integer Overflow - Subtraction (Underflow)
#include <limits.h>
#include <stdio.h>

// Test 1: Clear underflow - MIN - positive
int test_underflow_min_minus_one() {
    int x = INT_MIN;
    return x - 1;  // UNDERFLOW - Should detect
}

// Test 2: Potential underflow - unknown inputs
int test_potential_underflow(int a, int b) {
    return a - b;  // POTENTIAL UNDERFLOW - Should warn
}

// Test 3: Safe - subtraction
int test_safe_subtraction() {
    int x = 100;
    int y = 50;
    return x - y;  // SAFE - Should NOT detect
}

int main() {
    printf("CWE-190 Underflow Tests\n");
    return 0;
}
