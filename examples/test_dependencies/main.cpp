#include <optiweave/prelude.hpp>
#include "utils.hpp"
#include "math_helpers.hpp"
#include <iostream>

int main() {
    int result = add_numbers(5, 10);
    print_message("Hello from main!");
    std::cout << "Result: " << result << std::endl;
    return 0;
}
