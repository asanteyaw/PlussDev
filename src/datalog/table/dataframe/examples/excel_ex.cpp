// Excel Loader Example - Testing those sweet test_grades.xlsx 📊
// Because sometimes your data comes in Excel format and that's just life!

#include <iostream>
#include <filesystem>
#include <pluss/pluss.h>

using namespace pluss;
using namespace pluss::table;

int main(int argc, char** argv) {
    // When running from the `build/` directory the data files are one level up
    std::string excel_file = "../data/test_grades.xlsx";

    if (!std::filesystem::exists(excel_file)) {
        std::cerr << "❌ Excel file not found: " << excel_file << "\n";
        std::cerr << "Make sure test_grades.xlsx exists in the data/ directory!\n";
        return 1;
    }

    std::cout << "🎓 Loading test grades from Excel...\n";
    std::cout << "================================================\n\n";

    try {
        // Load Excel file using DataFrame::load - just like CSV but with Excel magic! ✨
        auto df = DataFrame::load("excel", excel_file);
        
        if (!df) {
            std::cerr << "❌ Failed to load " << excel_file << "\n";
            return 1;
        }

        std::cout << "✅ Successfully loaded Excel file!\n\n";

        // Show what columns we got
        std::cout << "📋 Columns found (" << df->columns().size() << "):\n";
        for (const auto& col : df->columns()) {
            std::cout << "   • " << col << "\n";
        }
        std::cout << "\n";

        // Get some basic info
        std::cout << "📊 DataFrame Info:\n";
        std::cout << "------------------------------------------------\n";
        df->info();
        std::cout << "\n";

        // Show the data shape
        auto [rows, cols] = df->shape();
        std::cout << "📐 Shape: " << rows << " rows × " << cols << " columns\n\n";

        // Display first few rows
        std::cout << "👀 First few rows:\n";
        std::cout << "================================================\n";
        df->head(10, 0);  // format=0 for text output
        std::cout << "\n";

        // Try to calculate some statistics if we have numeric columns
        std::cout << "📈 Quick Statistics:\n";
        std::cout << "------------------------------------------------\n";
        
        for (const auto& col : df->columns()) {
            try {
                auto series = df->get_col(col);
                // Try to get some stats
                std::cout << "\n" << col << ":\n";
                std::cout << "  Count: " << series.size() << "\n";
                
                // If it's numeric, we could do more...
                try {
                    double mean_val = series.mean();
                    std::cout << "  Mean: " << mean_val << "\n";
                } catch (...) {
                    std::cout << "  (Non-numeric column)\n";
                }
            } catch (const std::exception& e) {
                std::cout << "  Could not analyze: " << e.what() << "\n";
            }
        }

        std::cout << "\n🎉 Excel loading completed successfully!\n";
        std::cout << "================================================\n";
        std::cout << "\n💡 Tips:\n";
        std::cout << "  • Default loads first sheet: DataFrame::load(\"excel\", file)\n";
        std::cout << "  • List sheets: pluss::table::io::list_excel_sheets(file)\n";
        std::cout << "  • Load specific sheet: pluss::table::io::load_excel(file, \"Sheet2\")\n";
        std::cout << "  • Works with .xlsx files (Excel 2007+ format)\n";
        std::cout << "  • Powered by libzip + custom XML parser\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "💥 Error loading Excel file: " << e.what() << "\n";
        std::cerr << "\nPossible causes:\n";
        std::cerr << "  • File is not a valid .xlsx format\n";
        std::cerr << "  • File is corrupted\n";
        std::cerr << "  • libzip is not properly installed\n";
        return 1;
    }
}
