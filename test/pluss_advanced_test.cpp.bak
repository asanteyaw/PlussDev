#include <iostream>
#include <pluss/pluss.h>

int main() {
    std::cout << "=== Pluss Series Advanced Test ===" << std::endl;
    
    // Create a 2D Series
    pluss::Series matrix = pluss::value({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
    std::cout << "Matrix: " << matrix.toString() << std::endl;
    
    // Shape information
    std::cout << "Matrix dimensions: " << matrix.dim() << std::endl;
    std::cout << "Matrix shape: [";
    for (int i = 0; i < matrix.dim(); ++i) {
        std::cout << matrix.sizes()[i];
        if (i < matrix.dim() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    // Indexing
    pluss::Series row = matrix[0];
    std::cout << "First row: " << row.toString() << std::endl;
    
    // Reshaping
    pluss::Series reshaped = matrix.reshape({3, 2});
    std::cout << "Reshaped matrix (3x2): " << reshaped.toString() << std::endl;
    
    // Transposing
    pluss::Series transposed = matrix.transpose(0, 1);
    std::cout << "Transposed matrix: " << transposed.toString() << std::endl;
    
    // Element-wise operations
    pluss::Series squared = matrix.pow(2);
    std::cout << "Matrix squared: " << squared.toString() << std::endl;
    
    // Aggregation operations
    std::cout << "Matrix sum: " << matrix.sum().toString() << std::endl;
    std::cout << "Matrix mean: " << matrix.mean().toString() << std::endl;
    
    // Arithmetic operations
    pluss::Series doubled = matrix * pluss::value(2.0);
    std::cout << "Matrix doubled: " << doubled.toString() << std::endl;
    
    pluss::Series added = matrix + pluss::value(1.0);
    std::cout << "Matrix + 1: " << added.toString() << std::endl;
    
    // In-place operations
    pluss::Series inplace = matrix.clone();
    inplace += pluss::value(5.0);
    std::cout << "Matrix + 5 (in-place): " << inplace.toString() << std::endl;
    
    // Create a random matrix
    pluss::Series random = pluss::rand({2, 2});
    std::cout << "Random matrix: " << random.toString() << std::endl;
    
    // Create a sequence
    pluss::Series sequence = pluss::arange(0, 10);
    std::cout << "Sequence: " << sequence.toString() << std::endl;
    
    // Create a linspace
    pluss::Series linspace = pluss::linspace(0, 1, 5);
    std::cout << "Linspace: " << linspace.toString() << std::endl;
    
    return 0;
}
