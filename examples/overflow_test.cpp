#include <optiweave/prelude.hpp>
// Test file for integer overflow detection
#include <iostream>
#include <cstdint>

// Test 1: Signed integer overflow (undefined behavior)
int signed_add_overflow() {
    int x = 2147483647;  // INT_MAX
    int y = 1;
    return x + y;  // Potential signed overflow
}

// Test 2: Signed multiplication overflow
int signed_mul_overflow(int a, int b) {
    return a * b;  // Potential signed overflow
}

// Test 3: Mixed signedness
int mixed_signedness(int a, unsigned int b) {
    return a + b;  // Mixed signed/unsigned operation
}

// Test 4: Narrowing conversion
void narrowing_conversion() {
    int64_t large = 1000000000000;
    int small = large;  // Narrowing conversion
}

// Test 5: Loop counter overflow
void loop_counter_overflow() {
    for (int i = 0; i < 2147483647; ++i) {
        // Increment may overflow
    }
}

// Test 6: Unsigned wraparound
unsigned int unsigned_overflow(unsigned int a, unsigned int b) {
    return a + b;  // Unsigned wraparound (defined but potentially unintended)
}

// Test 7: Left shift overflow
int shift_overflow(int x) {
    return x << 31;  // May cause signed overflow
}

// Test 8: Division by zero (not overflow, but related)
int division_test(int a, int b) {
    return a / b;  // Potential division by zero
}

// Test 9: Negation overflow
int negation_overflow() {
    int x = -2147483648;  // INT_MIN
    return -x;  // Negating INT_MIN causes overflow
}

// Test 10: Safe operations with constants
int safe_constant_expr() {
    return 5 + 10;  // Constant expression, safe
}

int main() {
    std::cout << "Testing overflow detection" << std::endl;

    signed_add_overflow();
    signed_mul_overflow(1000000, 1000000);
    mixed_signedness(-1, 2U);
    narrowing_conversion();
    loop_counter_overflow();
    unsigned_overflow(4000000000U, 500000000U);
    shift_overflow(1);
    division_test(10, 2);
    negation_overflow();
    safe_constant_expr();

    return 0;
}
