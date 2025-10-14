#include <optiweave/prelude.hpp>
// Test file for floating-point precision warnings
#include <iostream>
#include <cmath>

// Test 1: Direct equality comparison of floating-point values
bool test_fp_equality(double a, double b) {
    return a == b;  // WARNING: Direct FP equality comparison
}

// Test 2: Comparison with zero
bool is_zero(double x) {
    return x == 0.0;  // WARNING: Exact zero comparison
}

// Test 3: Subtraction of nearly equal values (catastrophic cancellation)
double catastrophic_cancellation(double x) {
    double y = x + 1e-10;
    return x - y;  // INFO: Potential catastrophic cancellation
}

// Test 4: Double to float conversion (precision loss)
float convert_double_to_float(double x) {
    return x;  // WARNING: Precision loss in conversion
}

// Test 5: Mixed precision arithmetic
double mixed_precision(float a, double b) {
    return a + b;  // INFO: Mixed precision arithmetic
}

// Test 6: Accumulation without compensation
double accumulate_sum(double* values, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        sum += optiweave::__ow_subscript_impl(values, i, "/Users/shreeshjha/Dev/Github/OptiWeave/examples/fp_precision_test.cpp", 35, __FUNCTION__);  // INFO: Accumulation without compensation
    }
    return sum;
}

// Test 7: Division by potentially small value
double divide_by_small(double x, double y) {
    return x / y;  // INFO: Division by potentially small value
}

// Test 8: Multiplication that may overflow
double large_multiplication(double a, double b) {
    return a * b;  // INFO: May overflow to infinity
}

// Test 9: Safe operations with epsilon comparison
bool safe_equality(double a, double b, double epsilon = 1e-9) {
    return std::abs(a - b) < epsilon;  // SAFE: Uses epsilon comparison
}

// Test 10: Safe zero check
bool safe_zero_check(double x, double epsilon = 1e-9) {
    return std::abs(x) < epsilon;  // SAFE: Uses epsilon for zero check
}

int main() {
    std::cout << "Testing FP precision warnings" << std::endl;

    double a = 0.1 + 0.2;
    double b = 0.3;

    // Trigger various warnings
    test_fp_equality(a, b);
    is_zero(a - b);
    catastrophic_cancellation(1.0e15);
    convert_double_to_float(3.14159265358979323846);

    float f = 1.0f;
    double d = 2.0;
    mixed_precision(f, d);

    double values[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    accumulate_sum(values, 5);

    divide_by_small(100.0, 1e-10);
    large_multiplication(1e200, 1e200);

    // Safe operations
    safe_equality(a, b);
    safe_zero_check(a - b);

    return 0;
}
