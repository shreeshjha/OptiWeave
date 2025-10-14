#include <optiweave/prelude.hpp>
#include <iostream>

int main() {
    // Unused variable
    int unused_var = 42;

    // Uninitialized variable use
    int uninitialized;
    std::cout << "Value: " << uninitialized << std::endl;  // Bug: using uninitialized variable

    // Variable that's written but never read
    int write_only = 10;
    write_only = 20;
    write_only = 30;

    // Normal usage
    int normal = 5;
    std::cout << "Normal: " << normal << std::endl;

    return 0;
}
