// Multi-Sheet Excel Loader Example 📚
// Demonstrates how to work with Excel files that have multiple worksheets

#include <iostream>
#include <filesystem>
#include <pluss/pluss.h>

using namespace pluss;
using namespace pluss::table;
using namespace pluss::table::io;

int main(int argc, char** argv) {
    std::string excel_file = argc > 1 ? argv[1] : "../data/test_grades.xlsx";

    if (!std::filesystem::exists(excel_file)) {
        std::cerr << "❌ Excel file not found: " << excel_file << "\n";
        return 1;
    }

    std::cout << "📂 Excel Multi-Sheet Example\n";
    std::cout << "================================================\n";
    std::cout << "File: " << excel_file << "\n\n";

    try {
        // 1️⃣ List all sheets in the workbook
        std::cout << "🔍 Discovering sheets...\n";
        auto sheet_names = list_excel_sheets(excel_file);
        
        std::cout << "Found " << sheet_names.size() << " sheet(s):\n";
        for (size_t i = 0; i < sheet_names.size(); ++i) {
            std::cout << "  " << (i+1) << ". \"" << sheet_names[i] << "\"\n";
        }
        std::cout << "\n";

        // 2️⃣ Load default sheet (first sheet) using DataFrame::load
        if (!sheet_names.empty()) {
            std::cout << "📊 Loading first sheet using DataFrame::load(): \"" << sheet_names[0] << "\"\n";
            std::cout << "------------------------------------------------\n";
            
            auto df = DataFrame::load("excel", excel_file);
            auto [rows, cols] = df->shape();
            
            std::cout << "✅ Loaded: " << rows << " rows × " << cols << " columns\n";
            std::cout << "Columns: ";
            for (const auto& col : df->columns()) {
                std::cout << col << " ";
            }
            std::cout << "\n\n";
            
            // Show preview
            std::cout << "Preview:\n";
            df->head(5, 0);
            std::cout << "\n";
        }

        // 3️⃣ Load all sheets at once (if you have multiple)
        if (sheet_names.size() > 1) {
            std::cout << "🗂️  Loading ALL sheets...\n";
            std::cout << "------------------------------------------------\n";
            
            auto all_sheets = load_excel_all_sheets(excel_file);
            
            for (const auto& [name, df] : all_sheets) {
                auto [rows, cols] = df->shape();
                std::cout << "  📄 \"" << name << "\": " 
                         << rows << " rows × " << cols << " cols\n";
            }
            std::cout << "\n";
        }

        // 4️⃣ Load second sheet if it exists
        if (sheet_names.size() > 1) {
            std::cout << "📊 Loading second sheet: \"" << sheet_names[1] << "\"\n";
            std::cout << "------------------------------------------------\n";
            
            auto df2 = load_excel(excel_file, sheet_names[1]);
            auto [rows, cols] = df2->shape();
            
            std::cout << "✅ Loaded: " << rows << " rows × " << cols << " columns\n";
            df2->head(5, 0);
            std::cout << "\n";
        }

        // 5️⃣ Load Sheet2 by name using the advanced API
        
        std::cout << "📊 Loading \"Sheet2\" using load_excel()...\n";
        auto df_sheet2 = pluss::table::io::load_excel(excel_file, "Sheet2");
        auto [rows, cols] = df_sheet2->shape();
        std::cout << "✅ Sheet2: " << rows << " rows × " << cols << " columns\n";
        df_sheet2->head(5, 0);
        std::cout << "\n";
        

        // 🎯 Usage summary
        std::cout << "🎯 How to use with multiple sheets:\n";
        std::cout << "================================================\n";
        std::cout << "1. List sheets:\n";
        std::cout << "   auto sheets = pluss::table::io::list_excel_sheets(file);\n\n";
        
        std::cout << "2. Load specific sheet (advanced):\n";
        std::cout << "   auto df = pluss::table::io::load_excel(file, \"Sheet2\");\n\n";
        
        std::cout << "3. Load all sheets:\n";
        std::cout << "   auto all = pluss::table::io::load_excel_all_sheets(file);\n";
        std::cout << "   auto sheet1 = all[\"Sheet1\"];\n\n";

        std::cout << "4. Recommended: DataFrame::load (first sheet):\n";
        std::cout << "   auto df = DataFrame::load(\"excel\", file);\n";
        std::cout << "   // This is the official API for end users\n\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "💥 Error: " << e.what() << "\n";
        return 1;
    }
}
