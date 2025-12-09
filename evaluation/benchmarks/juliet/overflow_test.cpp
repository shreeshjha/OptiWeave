#include <optiweave/prelude.hpp>
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
