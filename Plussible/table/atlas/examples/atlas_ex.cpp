#include <iostream>
#include <torch/torch.h>

#include <pluss/pluss.h>

using namespace pluss;
using namespace pluss::table;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

int main() {
    print_separator("Atlas 3D Data Structure - Basic Examples");
    
    std::cout << "Thread-first design hierarchy:\n";
    std::cout << "  Thread (1D) -> DataFrame (2D) -> Atlas (3D)\n\n";
    
    // ========== Example 1: Time-series of DataFrames ==========
    print_separator("Example 1: Time-Series of Daily Sales Data");
    
    // Create daily sales DataFrames
    auto day1 = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"product", from_torch(torch::series({1.0, 2.0, 3.0}))},
        {"sales", from_torch(torch::series({100.0, 150.0, 200.0}))},
        {"revenue", from_torch(torch::series({1000.0, 2250.0, 4000.0}))}
    });
    
    auto day2 = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"product", from_torch(torch::series({1.0, 2.0, 3.0}))},
        {"sales", from_torch(torch::series({110.0, 140.0, 210.0}))},
        {"revenue", from_torch(torch::series({1100.0, 2100.0, 4200.0}))}
    });
    
    auto day3 = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"product", from_torch(torch::series({1.0, 2.0, 3.0}))},
        {"sales", from_torch(torch::series({105.0, 160.0, 195.0}))},
        {"revenue", from_torch(torch::series({1050.0, 2400.0, 3900.0}))}
    });
    
    // Create Atlas with daily labels
    Atlas sales_atlas({day1, day2, day3}, {"2024-01-01", "2024-01-02", "2024-01-03"});
    
    std::cout << "Created Atlas with 3 days of sales data\n";
    std::cout << "Number of DataFrames (depth): " << sales_atlas.ndepth() << "\n";
    std::cout << "Shape (depth, rows, cols): (" << std::get<0>(sales_atlas.shape()) 
              << ", " << std::get<1>(sales_atlas.shape()) 
              << ", " << std::get<2>(sales_atlas.shape()) << ")\n";
    std::cout << "Depth names: ";
    for (const auto& name : sales_atlas.depth_names()) {
        std::cout << name << " ";
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
    
    // ========== Example 2: Multi-region data ==========
    print_separator("Example 2: Regional Sales Data");
    
    auto us_sales = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"store_id", from_torch(torch::series({1.0, 2.0, 3.0}))},
        {"revenue", from_torch(torch::series({50000.0, 75000.0, 60000.0}))}
    });
    
    auto eu_sales = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"store_id", from_torch(torch::series({4.0, 5.0, 6.0}))},
        {"revenue", from_torch(torch::series({45000.0, 55000.0, 48000.0}))}
    });
    
    auto asia_sales = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"store_id", from_torch(torch::series({7.0, 8.0, 9.0}))},
        {"revenue", from_torch(torch::series({40000.0, 62000.0, 58000.0}))}
    });
    
    Atlas regional_atlas(std::unordered_map<std::string, std::shared_ptr<DataFrame>>{
        {"US", us_sales},
        {"EU", eu_sales},
        {"Asia", asia_sales}
    });
    
    std::cout << "Regional Atlas:\n";
    std::cout << "  Regions: " << regional_atlas.ndepth() << "\n";
    std::cout << "  Depth names: ";
    for (const auto& name : regional_atlas.depth_names()) {
        std::cout << name << " ";
    }
    std::cout << "\n\n";
    
    // ========== Example 3: Indexing Operations ==========
    print_separator("Example 3: Indexing and Slicing");
    
    // iloc - slice by position
    auto first_two = sales_atlas.iloc(0, 2);
    std::cout << "iloc(0,2) - first 2 days: " << first_two->ndepth() << " DataFrames\n";
    
    // head/tail
    auto first = sales_atlas.head(1);
    std::cout << "head(1): " << first->ndepth() << " DataFrame\n";
    
    auto last = sales_atlas.tail(1);
    std::cout << "tail(1): " << last->ndepth() << " DataFrame\n\n";
    
    // ========== Example 4: Column Selection ==========
    print_separator("Example 4: Selecting Columns Across All Depths");
    
    // Select specific columns from all DataFrames
    auto sales_only = sales_atlas.select_columns({"sales"});
    std::cout << "Selected 'sales' column from all days\n";
    std::cout << "  Result has " << sales_only->ncols() << " column(s)\n";
    std::cout << "  Across " << sales_only->ndepth() << " depths\n\n";
    
    // ========== Example 5: Depth Selection ==========
    print_separator("Example 5: Selecting Specific Depths");
    
    // Select by depth names
    std::vector<std::string> regions = {"US", "Asia"};
    auto selected = regional_atlas.loc(regions);
    std::cout << "Selected US and Asia regions: " << selected->ndepth() << " DataFrames\n";
    std::cout << "Depth names: ";
    for (const auto& name : selected->depth_names()) {
        std::cout << name << " ";
    }
    std::cout << "\n\n";
    
    // ========== Example 6: Apply Operations ==========
    print_separator("Example 6: Applying Functions Across Depths");
    
    // Apply a function to all DataFrames
    auto transformed = sales_atlas.apply([](std::shared_ptr<DataFrame> df) {
        // Sort each DataFrame by sales
        return df->sort_values("sales");
    });
    std::cout << "Applied sort_values('sales') to all DataFrames\n";
    std::cout << "  Result has " << transformed->ndepth() << " DataFrames\n\n";
    
    // ========== Example 7: Aggregation ==========
    print_separator("Example 7: Aggregations Across Depths");
    
    // Mean across all depths
    auto mean_atlas = sales_atlas.mean();
    std::cout << "Computed mean across all 3 days\n";
    std::cout << "  Result is an Atlas with averaged values\n";
    std::cout << "  Depth: " << mean_atlas->ndepth() << "\n\n";
    
    // Sum
    auto sum_atlas = sales_atlas.sum();
    std::cout << "Computed sum across all 3 days\n";
    std::cout << "  Total aggregated values\n";
    std::cout << "  Depth: " << sum_atlas->ndepth() << "\n\n";
    
    print_separator("Atlas Basic Examples Complete");
    std::cout << "\nFor comprehensive Atlas examples demonstrating all 50+ methods,\n";
    std::cout << "see: atlas_comprehensive_ex.cpp\n\n";
    std::cout << "Atlas enables powerful 3D data operations!\n";
    std::cout << "Thread (1D) -> DataFrame (2D) -> Atlas (3D)\n";
    
    return 0;
}
