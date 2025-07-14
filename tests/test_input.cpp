// Simple test file for OptiWeave
#include <iostream>

int main() {
    // Basic array subscript operations that should be transformed
    int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    
    // These should be transformed to __primop_subscript calls
    int first = arr[0];
    int middle = arr[5];
    int last = arr[9];
    
    // Pointer access
    int* ptr = arr;
    int value = ptr[3];
    
    // Arithmetic operations (if enabled)
    int sum = first + middle;
    int product = middle * 2;
    
    std::cout << "First: " << first << std::endl;
    std::cout << "Middle: " << middle << std::endl;
    std::cout << "Last: " << last << std::endl;
    std::cout << "Pointer value: " << value << std::endl;
    std::cout << "Sum: " << sum << std::endl;
    std::cout << "Product: " << product << std::endl;
    
    return 0;
}