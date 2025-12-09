#include <optiweave/prelude.hpp>
// CWE-190: Integer Overflow - Multiplication
#include <limits.h>
#include <stdio.h>

// Test 1: Clear overflow - large multiplication
int test_overflow_large_mult() {
    int x = INT_MAX / 2;
    return x * 3;  // OVERFLOW - Should detect
}

// Test 2: Potential overflow - unknown inputs
int test_potential_overflow_mult(int a, int b) {
    return a * b;  // POTENTIAL OVERFLOW - Should warn
}

// Test 3: Safe - small multiplication
int test_safe_mult() {
    int x = 10;
    int y = 20;
    return x * y;  // SAFE - Should NOT detect
}

// Test 4: Overflow in loop
int test_overflow_mult_loop(int n) {
    int result = 1;
    for (int i = 0; i < n; i++) {
        result *= 2;  // OVERFLOW if n >= 31 - Should detect
    }
    return result;
}

int main() {
    printf("CWE-190 Multiplication Overflow Tests\n");
    return 0;
}
