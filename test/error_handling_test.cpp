#include <iostream>
#include <pluss/pluss.h> 

using namespace pluss;
 // Include the full Series implementation

/**
 * This test demonstrates how error handling works in pluss.
 * 
 * Users write normal code without any try/catch blocks.
 * If an error occurs, they get a Pythonic-style error message
 * showing the exact location of the error.
 * 
 * The error handling is completely built into the Series operations.
 */

int main() {
    std::cout << "Pluss Error Handling Demonstration\n";
    std::cout << "==================================\n\n";
    
    std::cout << "Example 1: Shape mismatch error\n";
    std::cout << "-------------------------------\n";
    
    // Create tensors with incompatible shapes
    Series t1 = pluss::value({2.0, 1.34, 0.04});
    Series t2 = pluss::ones({2, 4});

    Series a = pluss::value({1.0, 2.0, 3.0});
    Series b = pluss::value({{1.0, 2.0}, {3.0, 4.0}});
    
    // This will cause a shape mismatch error
    // The error will be automatically converted to a PlussError with traceback
    Series result1 = t1.div(t2);
    
    // The following code won't be executed due to the error above
    std::cout << "This line won't be reached\n\n";
    
    std::cout << "Example 2: Index out of bounds error\n";
    std::cout << "----------------------------------\n";
    // Create a tensor
    Series t3 = pluss::arange(0, 5);
    
    // This will cause an index out of bounds error
    // The error will be automatically converted to a PlussError with traceback
    Series result2 = t3[10];
    
    // The following code won't be executed due to the error above
    std::cout << "This line won't be reached\n\n";
    
    std::cout << "Example 3: Type error\n";
    std::cout << "-------------------\n";
    // Create a boolean tensor
    Series t4 = pluss::value({true, false, true});
    
    // This will cause a type error (can't compute mean of boolean tensor)
    // The error will be automatically converted to a PlussError with traceback
    Series result3 = t4.mean();
    
    // The following code won't be executed due to the error above
    std::cout << "This line won't be reached\n\n";
    
    return 0;
}
