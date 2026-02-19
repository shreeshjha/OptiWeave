#include "math_utils.hpp"
#include <cmath>

// ISSUE: FP equality comparison (CRITICAL)
bool isZero(double value) {
    return value == 0.0;  // Should use epsilon comparison
}

// ISSUE: Catastrophic cancellation (WARNING)
double computeDelta(double a, double b) {
    // When a ≈ b, this loses precision
    return a - b;
}

// ISSUE: Mixed precision arithmetic (INFO)
double calculateDistance(double x1, double y1, double x2, double y2) {
    float dx = x2 - x1;  // Narrowing conversion
    float dy = y2 - y1;  // Narrowing conversion
    return sqrt(dx * dx + dy * dy);  // Mixed precision
}

// ISSUE: Double to float conversion (WARNING)
float convertToFloat(double value) {
    return value;  // Potential precision loss
}

// ISSUE: Accumulation without compensation (WARNING)
double accumulateSum(double* values, int count) {
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += values[i];  // Should use Kahan summation for better precision
    }
    return sum;
}

// ISSUE: Signed integer overflow potential (CRITICAL)
int multiplyValues(int a, int b) {
    return a * b;  // No overflow check
}

// ISSUE: Loop counter overflow (WARNING)
int countToMax() {
    int counter = 2147483640;  // Close to INT_MAX
    for (int i = 0; i < 20; i++) {
        counter++;  // Will overflow
    }
    return counter;
}

// ISSUE: Narrowing conversion (WARNING)
int convertLongToInt(long long value) {
    return value;  // Potential data loss
}
