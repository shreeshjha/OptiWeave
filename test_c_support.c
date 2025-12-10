/* Simple C test file */
#include <stdio.h>

int test_addition(int a, int b) {
    return a + b;
}

int main() {
    int x = 5;
    int y = 10;
    int result = test_addition(x, y);

    printf("Result: %d\n", result);

    int arr[10];
    arr[0] = 42;
    printf("Array value: %d\n", arr[0]);

    return 0;
}
