#include <optiweave/prelude.hpp>
// Test file for call graph generation
#include <iostream>
#include <vector>

// Helper function
int add(int a, int b) {
    return a + b;
}

// Helper function
int multiply(int a, int b) {
    return a * b;
}

// Calls add and multiply
int calculate(int x, int y) {
    int sum = add(x, y);
    int product = multiply(x, y);
    return sum + product;
}

// Calls calculate
int process(int n) {
    int result = 0;
    for (int i = 0; i < n; ++i) {
        result += calculate(i, i + 1);
    }
    return result;
}

// Entry point
int main() {
    int result = process(10);
    std::cout << "Result: " << result << std::endl;
    return 0;
}
