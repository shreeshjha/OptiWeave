#include "math_utils.hpp"
#include "data_processor.hpp"
#include "config_parser.hpp"

// Main entry point demonstrating call graph

void initialize() {
    ConfigParser parser;
    parser.loadConfig("config.txt");
}

void runSimulation() {
    double distance = calculateDistance(0.0, 0.0, 10.0, 10.0);
    bool zero = isZero(distance);
    
    int result = multiplyValues(1000, 2000);
}

void processInput() {
    DataProcessor processor;
    int data[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    processor.processData(data, 10);
}

int main() {
    // Call graph: main -> initialize -> ConfigParser::loadConfig -> utilityFunction
    //                  -> runSimulation -> calculateDistance, isZero, multiplyValues
    //                  -> processInput -> DataProcessor::processData
    
    initialize();
    runSimulation();
    processInput();
    
    return 0;
}
