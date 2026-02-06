#include <pluss/pluss.h>
#include <iostream>

using namespace pluss::table;

int main() {
    auto df = DataFrame::load("csv", "../data/messy_data.csv");
    
    std::cout << "=== Default head() - TEXT format for terminal ===" << std::endl;
    df->head(3, 0);  // Explicit format parameter to use display version
    
    std::cout << "\n\n=== head() with TEXT format (0) ===" << std::endl;
    df->head(3, 0);
    
    std::cout << "\n\n=== head() with HTML format (1) - for Jupyter ===" << std::endl;
    df->head(3, 1);
    
    std::cout << "\n\n=== head() with LaTeX format (2) ===" << std::endl;
    df->head(3, 2);
    
    return 0;
}
