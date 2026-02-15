#include <optiweave/prelude.hpp>
#include <array>
#include <iostream>
#include <vector>

// Basic array subscript example
int basic_array_example() {
  int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  // Reverted to standard array access
  int first = optiweave::__ow_subscript_impl(arr, 0, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 10, __FUNCTION__);
  int middle = optiweave::__ow_subscript_impl(arr, 5, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 11, __FUNCTION__);
  int last = optiweave::__ow_subscript_impl(arr, 9, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 12, __FUNCTION__);

  return first + middle + last;
}

// Pointer access example
int pointer_example() {
  int data[] = {10, 20, 30, 40, 50};
  int *ptr = data;

  // Reverted pointer subscripting
  return optiweave::__ow_subscript_impl(ptr, 2, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 23, __FUNCTION__);
}

// Multi-dimensional array example
int multidimensional_example() {
  int matrix[3][4] = {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};

  // Reverted nested subscripting
  return optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(matrix, 1, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 31, __FUNCTION__), 2, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 31, __FUNCTION__);
}

// Dynamic array example
int dynamic_array_example() {
  std::vector<int> vec = {1, 2, 3, 4, 5};

  // This uses overloaded operator[]
  return vec[2];
}

// Template function example
template <typename T> T template_array_access(T *arr, int index) {
  // Reverted template access
  return optiweave::__ow_subscript_impl(arr, index, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 45, __FUNCTION__);
}

// Class with overloaded operator[]
class MyArray {
private:
  int data[10];

public:
  MyArray() {
    for (int i = 0; i < 10; ++i) {
      ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 56, __FUNCTION__); ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 57, __FUNCTION__); ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 57, __FUNCTION__); data[i] = i * i; }); }); });
    }
  }

  int &operator[](int index) {
    // Built-in array access inside class
    return optiweave::__ow_subscript_impl(data, index, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 62, __FUNCTION__);
  }

  const int &operator[](int index) const { return optiweave::__ow_subscript_impl(data, index, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 65, __FUNCTION__); }
};

// Function using custom array class
int custom_array_example() {
  MyArray arr;

  // This is an overloaded operator call
  return arr[5];
}

// Nested access example
int nested_access_example() {
  int *ptrs[3];
  int data1[] = {1, 2, 3};
  int data2[] = {4, 5, 6};
  int data3[] = {7, 8, 9};

  ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 83, __FUNCTION__); ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 84, __FUNCTION__); ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 84, __FUNCTION__); ptrs[0] = data1; }); }); });
  ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 84, __FUNCTION__); ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 85, __FUNCTION__); ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 85, __FUNCTION__); ptrs[1] = data2; }); }); });
  ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 85, __FUNCTION__); ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 86, __FUNCTION__); ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 86, __FUNCTION__); ptrs[2] = data3; }); }); });

  // Multiple levels of array access
  return optiweave::__ow_subscript_impl(optiweave::__ow_subscript_impl(ptrs, 1, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 88, __FUNCTION__), 2, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 88, __FUNCTION__);
}

// Complex expression as index
int complex_index_example() {
  int arr[20];
  for (int i = 0; i < 20; ++i) {
    ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 95, __FUNCTION__); ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 96, __FUNCTION__); ({ __optiweave_record_subscript("/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 96, __FUNCTION__); arr[i] = i; }); }); });
  }

  int x = 3, y = 4;

  // Complex index expression preserved
  return optiweave::__ow_subscript_impl(arr, x * y + 2, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 101, __FUNCTION__);
}

// Array access in different contexts
int context_examples() {
  int arr[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  // Normal access
  int normal = optiweave::__ow_subscript_impl(arr, 3, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 109, __FUNCTION__);

  // Address-of context
  int *ptr = &arr[5];

  // In sizeof context
  size_t element_size = sizeof(optiweave::__ow_subscript_impl(arr, 0, "/Users/shreesh/Dev/Github/OptiWeave/examples/basic_transformation/example1.cpp", 115, __FUNCTION__));

  return normal + *ptr + static_cast<int>(element_size);
}

// Function demonstrating arithmetic operators
int arithmetic_example() {
  int a = 10, b = 20, c = 30;

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
