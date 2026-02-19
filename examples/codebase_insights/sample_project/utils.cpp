#include "utils.hpp"

void utilityFunction() {
    // Utility implementation
}

// ISSUE: Recursive function (will show in call graph)
void recursiveHelper(int depth) {
    if (depth > 0) {
        recursiveHelper(depth - 1);
    }
}
