#include <iostream>
#include <pluss/pluss.h>
#include <torch/torch.h>

using namespace pluss;
using namespace pluss::table;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

int main() {
    print_separator("Atlas 3D Data Structure Examples");
    
    std::cout << "Thread-first design hierarchy:\n";
    std::cout << "  Thread (1D) -> DataFrame (2D) -> Atlas (3D)\n\n";
    
    // ========== Example 1: Time-series of DataFrames ==========
    print_separator("Example 1: Time-Series of Daily Sales Data");
    
    // Create daily sales DataFrames
    auto day1 = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"product", from_torch(torch::tensor({1.0, 2.0, 3.0}))},
        {"sales", from_torch(torch::tensor({100.0, 150.0, 200.0}))},
        {"revenue", from_torch(torch::tensor({1000.0, 2250.0, 4000.0}))}
    });
    
    auto day2 = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"product", from_torch(torch::tensor({1.0, 2.0, 3.0}))},
        {"sales", from_torch(torch::tensor({110.0, 140.0, 210.0}))},
        {"revenue", from_torch(torch::tensor({1100.0, 2100.0, 4200.0}))}
    });
    
    auto day3 = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"product", from_torch(torch::tensor({1.0, 2.0, 3.0}))},
        {"sales", from_torch(torch::tensor({105.0, 160.0, 195.0}))},
        {"revenue", from_torch(torch::tensor({1050.0, 2400.0, 3900.0}))}
    });
    
    // Create Atlas with daily labels
    Atlas sales_atlas({day1, day2, day3}, {"2024-01-01", "2024-01-02", "2024-01-03"});
    
    std::cout << "Created Atlas with 3 days of sales data\n";
    std::cout << "Size: " << sales_atlas.size() << " DataFrames\n";
    std::cout << "Labels: ";
    for (const auto& label : sales_atlas.labels()) {
        std::cout << label << " ";
    }
    std::cout << "\n\n";
    
    // Access specific day
    std::cout << "Accessing day 2024-01-02:\n";
    auto day2_df = sales_atlas["2024-01-02"];
    std::cout << "Columns: ";
    for (const auto& col : day2_df->columns()) {
        std::cout << col << " ";
    }
    std::cout << "\n\n";
    
    // ========== Example 2: Multi-category data ==========
    print_separator("Example 2: Regional Sales Data (Multi-Category)");
    
    auto us_sales = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"store_id", from_torch(torch::tensor({1.0, 2.0, 3.0}))},
        {"revenue", from_torch(torch::tensor({50000.0, 75000.0, 60000.0}))}
    });
    
    auto eu_sales = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"store_id", from_torch(torch::tensor({4.0, 5.0, 6.0}))},
        {"revenue", from_torch(torch::tensor({45000.0, 55000.0, 48000.0}))}
    });
    
    auto asia_sales = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"store_id", from_torch(torch::tensor({7.0, 8.0, 9.0}))},
        {"revenue", from_torch(torch::tensor({40000.0, 62000.0, 58000.0}))}
    });
    
    Atlas regional_atlas(std::unordered_map<std::string, std::shared_ptr<DataFrame>>{
        {"US", us_sales},
        {"EU", eu_sales},
        {"Asia", asia_sales}
    });
    
    std::cout << regional_atlas.to_string();
    
    // ========== Example 3: Slicing and filtering ==========
    print_separator("Example 3: Slicing and Filtering");
    
    // Slice first 2 days
    auto first_two_days = sales_atlas.slice(0, 2);
    std::cout << "Sliced first 2 days: " << first_two_days->size() << " DataFrames\n";
    std::cout << "Labels: ";
    for (const auto& label : first_two_days->labels()) {
        std::cout << label << " ";
    }
    std::cout << "\n\n";
    
    // Select specific regions
    auto selected_regions = regional_atlas.select({"US", "Asia"});
    std::cout << "Selected US and Asia regions:\n";
    std::cout << selected_regions->to_string() << "\n";
    
    // Filter by condition
    auto large_regions = regional_atlas.filter(
        [](const std::string& label, const std::shared_ptr<DataFrame>& df) {
            return label == "US" || label == "Asia";
        }
    );
    std::cout << "Filtered large regions: " << large_regions->size() << " DataFrames\n\n";
    
    // ========== Example 4: Transformation operations ==========
    print_separator("Example 4: Applying Transformations");
    
    // Apply function to all DataFrames
    auto sorted_atlas = sales_atlas.apply([](std::shared_ptr<DataFrame> df) {
        return df->sort_values("sales");
    });
    std::cout << "Applied sorting to all DataFrames in Atlas\n";
    std::cout << "Sorted Atlas size: " << sorted_atlas->size() << "\n\n";
    
    // Apply with label context
    auto labeled_transform = regional_atlas.apply_with_label(
        [](const std::string& label, std::shared_ptr<DataFrame> df) {
            std::cout << "  Processing region: " << label << "\n";
            return df->copy();
        }
    );
    std::cout << "\n";
    
    // ========== Example 5: Stacking DataFrames ==========
    print_separator("Example 5: Stacking DataFrames Vertically");
    
    try {
        auto stacked = sales_atlas.stack(true);
        std::cout << "Stacked all daily sales into single DataFrame\n";
        std::cout << "Columns: ";
        for (const auto& col : stacked->columns()) {
            std::cout << col << " ";
        }
        std::cout << "\n";
        std::cout << "Total rows: 9 (3 days × 3 products)\n\n";
    } catch (const std::exception& e) {
        std::cout << "Stacking error: " << e.what() << "\n\n";
    }
    
    // ========== Example 6: Query operations ==========
    print_separator("Example 6: Query Operations");
    
    std::cout << "Sales Atlas properties:\n";
    std::cout << "  Is uniform: " << (sales_atlas.is_uniform() ? "Yes" : "No") << "\n";
    std::cout << "  Empty: " << (sales_atlas.empty() ? "Yes" : "No") << "\n";
    
    auto common_cols = sales_atlas.common_columns();
    std::cout << "  Common columns: ";
    for (const auto& col : common_cols) {
        std::cout << col << " ";
    }
    std::cout << "\n\n";
    
    std::cout << "Regional Atlas properties:\n";
    std::cout << "  All columns: ";
    auto all_cols = regional_atlas.all_columns();
    for (const auto& col : all_cols) {
        std::cout << col << " ";
    }
    std::cout << "\n\n";
    
    // ========== Example 7: Concatenation ==========
    print_separator("Example 7: Concatenating Atlases");
    
    auto q1_atlas = sales_atlas.slice(0, 2);  // Jan days
    auto q2_atlas = sales_atlas.slice(2, 3);  // Feb days
    
    auto combined = q1_atlas->concat(q2_atlas, false);
    std::cout << "Concatenated Q1 and Q2:\n";
    std::cout << "  Total DataFrames: " << combined->size() << "\n";
    std::cout << "  Labels: ";
    for (const auto& label : combined->labels()) {
        std::cout << label << " ";
    }
    std::cout << "\n\n";
    
    // ========== Example 8: Sorting and reversing ==========
    print_separator("Example 8: Sorting and Reversing");
    
    auto sorted_labels = regional_atlas.sort_by_label(true);
    std::cout << "Sorted by label (ascending): ";
    for (const auto& label : sorted_labels->labels()) {
        std::cout << label << " ";
    }
    std::cout << "\n";
    
    auto reversed = sales_atlas.reverse();
    std::cout << "Reversed order: ";
    for (const auto& label : reversed->labels()) {
        std::cout << label << " ";
    }
    std::cout << "\n\n";
    
    // ========== Example 9: First/Last access ==========
    print_separator("Example 9: First and Last DataFrames");
    
    auto first_day = sales_atlas.first();
    auto last_day = sales_atlas.last();
    
    std::cout << "First DataFrame columns: ";
    for (const auto& col : first_day->columns()) {
        std::cout << col << " ";
    }
    std::cout << "\n";
    
    std::cout << "Last DataFrame columns: ";
    for (const auto& col : last_day->columns()) {
        std::cout << col << " ";
    }
    std::cout << "\n\n";
    
    // ========== Example 10: Deep copy ==========
    print_separator("Example 10: Deep Copy");
    
    auto atlas_copy = sales_atlas.copy();
    std::cout << "Created deep copy of Atlas\n";
    std::cout << "Original size: " << sales_atlas.size() << "\n";
    std::cout << "Copy size: " << atlas_copy->size() << "\n";
    std::cout << "All DataFrames are independently copied\n\n";
    
    print_separator("Atlas Examples Complete");
    std::cout << "\nAtlas enables powerful 3D data operations in the thread-first design!\n";
    std::cout << "Thread (1D) -> DataFrame (2D) -> Atlas (3D)\n";
    
    return 0;
}
