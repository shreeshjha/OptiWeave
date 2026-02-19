#ifndef MATH_UTILS_HPP
#define MATH_UTILS_HPP

// Floating-point precision issues demonstrated here
double calculateDistance(double x1, double y1, double x2, double y2);
double computeDelta(double a, double b);
float convertToFloat(double value);
double accumulateSum(double* values, int count);
bool isZero(double value);

// Integer overflow issues demonstrated here
int multiplyValues(int a, int b);
int countToMax();
int convertLongToInt(long long value);

#endif // MATH_UTILS_HPP
