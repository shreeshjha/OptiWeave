#include "data_processor.hpp"

// ISSUE: Uninitialized variable (WARNING)
void DataProcessor::processData(int* data, int size) {
    int result;  // Declared but not initialized
    
    if (size > 0) {
        result = data[0];
    }
    
    // ISSUE: Potentially using uninitialized variable
    int output = result * 2;  // result might be uninitialized if size <= 0
}

// ISSUE: Unused variable (INFO)
int DataProcessor::computeResult(int input) {
    int temp = input * 2;  // Computed but never used
    int unused_var = 42;   // Declared and initialized but never used
    
    return input + 10;  // temp is not used
}

// ISSUE: Dead code (WARNING)
void DataProcessor::helperFunction(int value) {
    if (value < 0) {
        return;
    }
    
    int processing_result = value * 3;
    return;
    
    // ISSUE: Dead code - unreachable
    processing_result = processing_result + 1;
    int dead_variable = 100;
}

// ISSUE: Variable never read (INFO)
void processArray(int* arr, int size) {
    int counter = 0;  // Written but never read
    for (int i = 0; i < size; i++) {
        counter++;
        arr[i] = i * 2;
    }
    // counter value is never used
}
