#include <array>
#include <iostream>
#include <vector>

// Basic array subscript example
int basic_array_example() {
  int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  // These will be transformed to __primop_subscript calls
  int first = arr[0];
  int middle = arr[5];
  int last = arr[9];

  return first + middle + last;
}

// Pointer access example
int pointer_example() {
  int data[] = {10, 20, 30, 40, 50};
  int *ptr = data;

  // This will also be transformed
  return ptr[2];
}

// Multi-dimensional array example
int multidimensional_example() {
  int matrix[3][4] = {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};

  // Both subscript operations will be transformed
  return matrix[1][2];
}

// Dynamic array example
int dynamic_array_example() {
  std::vector<int> vec = {1, 2, 3, 4, 5};

  // This uses overloaded operator[], may not be transformed
  // depending on configuration
  return vec[2];
}

// Template function example
template <typename T> T template_array_access(T *arr, int index) {
  // This will use __maybe_primop_subscript for template-dependent types
  return arr[index];
}

// Class with overloaded operator[]
class MyArray {
private:
  int data[10];

public:
  MyArray() {
    for (int i = 0; i < 10; ++i) {
      data[i] = i * i;
    }
  }

  int &operator[](int index) {
    // Built-in array access inside class
    return data[index]; // This will be transformed
  }

  const int &operator[](int index) const {
    return data[index]; // This will also be transformed
  }
};

// Function using custom array class
int custom_array_example() {
  MyArray arr;

  // This is an overloaded operator call, behavior depends on configuration
  return arr[5];
}

// Nested access example
int nested_access_example() {
  int *ptrs[3];
  int data1[] = {1, 2, 3};
  int data2[] = {4, 5, 6};
  int data3[] = {7, 8, 9};

  ptrs[0] = data1;
  ptrs[1] = data2;
  ptrs[2] = data3;

  // Multiple levels of array access
  return ptrs[1][2]; // Both will be transformed
}

// Complex expression as index
int complex_index_example() {
  int arr[20];
  for (int i = 0; i < 20; ++i) {
    arr[i] = i;
  }

  int x = 3, y = 4;

  // Complex index expression will be preserved
  return arr[x * y + 2];
}

// Array access in different contexts
int context_examples() {
  int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  // Normal access - will be transformed
  int normal = arr[3];

  // Address-of context - should NOT be transformed
  int *ptr = &arr[5];

  // In sizeof context - should NOT be transformed
  size_t element_size = sizeof(arr[0]);

  return normal + *ptr + static_cast<int>(element_size);
}

// Function demonstrating arithmetic operators (if enabled)
int arithmetic_example() {
  int a = 10, b = 20, c = 30;

  // These might be transformed if arithmetic operator transformation is enabled
  int sum = a + b;
  int product = a * c;
  int difference = c - a;
  int quotient = b / 2;

  return sum + product - difference + quotient;
}

// Main function
int main() {
  std::cout << "OptiWeave Basic Transformation Example\n";
  std::cout << "=====================================\n\n";

  std::cout << "Basic array: " << basic_array_example() << std::endl;
  std::cout << "Pointer access: " << pointer_example() << std::endl;
  std::cout << "Multi-dimensional: " << multidimensional_example() << std::endl;
  std::cout << "Dynamic array: " << dynamic_array_example() << std::endl;

  // Template instantiation
  int template_data[] = {100, 200, 300};
  std::cout << "Template access: " << template_array_access(template_data, 1)
            << std::endl;

  std::cout << "Custom array: " << custom_array_example() << std::endl;
  std::cout << "Nested access: " << nested_access_example() << std::endl;
  std::cout << "Complex index: " << complex_index_example() << std::endl;
  std::cout << "Context examples: " << context_examples() << std::endl;
  std::cout << "Arithmetic: " << arithmetic_example() << std::endl;

  return 0;
}
