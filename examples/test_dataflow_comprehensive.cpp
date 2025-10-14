#include <optiweave/prelude.hpp>
#include <iostream>

// Test 1: Unused variable
void test_unused() {
    int unused = 42;  // Should be detected as unused
    std::cout << "test_unused called" << std::endl;
}

// Test 2: Uninitialized variable
void test_uninitialized() {
    int x;
    std::cout << "Value: " << x << std::endl;  // Using uninitialized variable
}

// Test 3: Write-only variable
void test_write_only() {
    int counter = 0;
    counter = 1;
    counter = 2;
    // Never read counter
}

// Test 4: Normal usage (should not be flagged)
void test_normal() {
    int value = 10;
    std::cout << "Value: " << value << std::endl;
}

// Test 5: Multiple issues in one function
void test_multiple_issues() {
    int unused1 = 1;     // Unused
    int unused2 = 2;     // Unused
    int uninitialized;   // Uninitialized use below
    int normal = 5;      // Normal usage

    std::cout << "Uninit: " << uninitialized << std::endl;
    std::cout << "Normal: " << normal << std::endl;
}

int main() {
    test_unused();
    test_uninitialized();
    test_write_only();
    test_normal();
    test_multiple_issues();
    return 0;
}
