#include <optiweave/prelude.hpp>
// CWE-190: Integer Overflow - Addition
#include <limits.h>
#include <stdio.h>

// Test 1: Clear overflow - MAX + positive
int test_clear_overflow_max_plus_one() {
    int x = INT_MAX;
    return x + 1;  // OVERFLOW - Should detect
}

// Test 2: Clear overflow - MAX + large positive  
int test_clear_overflow_max_plus_large() {
    int x = INT_MAX;
    return x + 1000;  // OVERFLOW - Should detect
}

// Test 3: Potential overflow - unknown input
int test_potential_overflow_unknown(int a, int b) {
    return a + b;  // POTENTIAL OVERFLOW - Should warn
}

// Test 4: Loop accumulation overflow
int test_loop_overflow(int iterations) {
    int sum = 0;
    for (int i = 0; i < iterations; i++) {
        sum += 1000000;  // OVERFLOW if iterations large - Should detect
    }
    return sum;
}

// Test 5: Safe - small constants
int test_safe_small_constants() {
    int x = 10;
    int y = 20;
    return x + y;  // SAFE - Should NOT detect
}

// Test 6: Safe - negative numbers
int test_safe_negative() {
    int x = -100;
    int y = 50;
    return x + y;  // SAFE - Should NOT detect
}

// Test 7: Overflow - Near boundary
int test_overflow_near_boundary() {
    int x = INT_MAX - 10;
    return x + 20;  // OVERFLOW - Should detect
}

// Test 8: Safe - Near boundary but safe
int test_safe_near_boundary() {
    int x = INT_MAX - 100;
    return x + 50;  // SAFE - Should NOT detect (but may warn conservatively)
}

int main() {
    printf("CWE-190 Integer Overflow Test Cases\n");
    return 0;
}
