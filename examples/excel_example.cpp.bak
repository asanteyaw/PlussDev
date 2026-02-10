// Example demonstrating Excel loading in PlussMa
// Compile with: (linked automatically via CMake)
// Usage: ./excel_example_test <path_to_excel_file.xlsx>

#include <pluss/pluss.h>
#include <iostream>
#include <exception>

int main(int argc, char** argv) {
    try {
        std::string filename = argc > 1 ? argv[1] : "data/sample.xlsx";
        
        std::cout << "Loading Excel file: " << filename << "\n\n";
        
        // Example 1: Load default (first) sheet
        auto df = pluss::table::io::load_excel(filename);
        std::cout << "Successfully loaded Excel file!\n";
        std::cout << "Shape: " << df->shape().first << " rows × " 
                  << df->shape().second << " columns\n\n";
        
        // Example 2: Load specific sheet by name
        // auto df2 = pluss::table::io::load_excel(filename, "Sheet2");
        
        // Example 3: Load raw data (strings only) for custom processing
        // auto raw = pluss::table::io::load_excel_raw(filename);
        // std::cout << "Columns: ";
        // for (const auto& col : raw.columns) {
        //     std::cout << col << " ";
        // }
        // std::cout << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "\nNote: Make sure libzip is installed:\n";
        std::cerr << "  macOS: brew install libzip\n";
        std::cerr << "  Ubuntu/Debian: sudo apt-get install libzip-dev\n";
        std::cerr << "  Fedora/RHEL: sudo dnf install libzip-devel\n";
        return 1;
    }
}
