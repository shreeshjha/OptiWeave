#include <optiweave/prelude.hpp>
#include <iostream>
#include <stdio.h>

int main() {
  int arr[5] = {0, 1, 2, 3, 4};
  std::cout << optiweave::ow_subscript(arr, 2) << std::endl;
  return 0;
}
