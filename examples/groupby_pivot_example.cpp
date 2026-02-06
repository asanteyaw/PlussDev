#include <iostream>
#include <pluss/pluss.h>

using namespace pluss;
using namespace pluss::table;

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n\n";
}

int main() {
    print_separator("GroupBy and Pivot Examples");
    
    // ============================================================
    // Example 1: Basic GroupBy with Single Column
    // ============================================================
    print_separator("1. Basic GroupBy - Sales by Region");
    
    // Create sample sales data
    auto sales_df = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"region", from_torch(torch::tensor({1.0, 1.0, 2.0, 2.0, 3.0, 3.0}))},
        {"product", from_torch(torch::tensor({101.0, 102.0, 101.0, 102.0, 101.0, 102.0}))},
        {"sales", from_torch(torch::tensor({100.0, 150.0, 120.0, 180.0, 90.0, 200.0}))},
        {"revenue", from_torch(torch::tensor({1000.0, 2250.0, 1200.0, 2700.0, 900.0, 3000.0}))}
    });
    
    std::cout << "Original DataFrame:\n";
    sales_df->head(10, 0);
    
    // Group by region and compute aggregations
    auto grouped = sales_df->groupby("region");
    
    std::cout << "\nGroupBy region - Sum:\n";
    auto sum_result = grouped->sum();
    sum_result->head(10, 0);
    
    std::cout << "\nGroupBy region - Mean:\n";
    auto mean_result = grouped->mean();
    mean_result->head(10, 0);
    
    std::cout << "\nGroupBy region - Count:\n";
    auto count_result = grouped->count();
    count_result->head(10, 0);
    
    // ============================================================
    // Example 2: Multiple Aggregations
    // ============================================================
    print_separator("2. Multiple Aggregations with agg()");
    
    std::cout << "Computing mean, std, and count together:\n";
    auto multi_agg = grouped->agg({"mean", "std", "count"});
    multi_agg->head(10, 0);
    
    // ============================================================
    // Example 3: Multi-Column GroupBy
    // ============================================================
    print_separator("3. Multi-Column GroupBy");
    
    // Create data with multiple grouping dimensions
    auto multi_df = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"region", from_torch(torch::tensor({1.0, 1.0, 1.0, 2.0, 2.0, 2.0}))},
        {"quarter", from_torch(torch::tensor({1.0, 2.0, 3.0, 1.0, 2.0, 3.0}))},
        {"revenue", from_torch(torch::tensor({1000.0, 1200.0, 1100.0, 1500.0, 1600.0, 1400.0}))}
    });
    
    std::cout << "Original DataFrame:\n";
    multi_df->head(10, 0);
    
    std::cout << "\nGroupBy [region, quarter] - Mean revenue:\n";
    auto multi_grouped = multi_df->groupby({"region", "quarter"});
    auto multi_result = multi_grouped->mean();
    multi_result->head(10, 0);
    
    // ============================================================
    // Example 4: Basic Pivot
    // ============================================================
    print_separator("4. Pivot - Reshape from Long to Wide Format");
    
    // Create long-format data
    auto long_df = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"date", from_torch(torch::tensor({1.0, 1.0, 2.0, 2.0, 3.0, 3.0}))},
        {"product", from_torch(torch::tensor({1.0, 2.0, 1.0, 2.0, 1.0, 2.0}))},
        {"sales", from_torch(torch::tensor({100.0, 150.0, 120.0, 180.0, 90.0, 200.0}))}
    });
    
    std::cout << "Long-format data:\n";
    long_df->head(10, 0);
    
    std::cout << "\nPivot: dates as rows, products as columns, sales as values\n";
    auto pivoted = long_df->pivot("date", "product", "sales");
    pivoted->head(10, 0);
    
    // ============================================================
    // Example 5: Pivot Table with Aggregation
    // ============================================================
    print_separator("5. Pivot Table - With Aggregation");
    
    // Create data with duplicate combinations (needs aggregation)
    auto pivot_data = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"region", from_torch(torch::tensor({1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 1.0, 1.0}))},
        {"quarter", from_torch(torch::tensor({1.0, 1.0, 2.0, 1.0, 1.0, 2.0, 1.0, 2.0}))},
        {"revenue", from_torch(torch::tensor({1000.0, 1100.0, 1200.0, 1500.0, 1600.0, 1400.0, 950.0, 1250.0}))}
    });
    
    std::cout << "Data with duplicates (multiple observations per region-quarter):\n";
    pivot_data->head(10, 0);
    
    std::cout << "\nPivot Table with mean aggregation:\n";
    auto pivot_table_mean = pivot_data->pivot_table("revenue", "region", "quarter", "mean");
    pivot_table_mean->head(10, 0);
    
    std::cout << "\nPivot Table with sum aggregation:\n";
    auto pivot_table_sum = pivot_data->pivot_table("revenue", "region", "quarter", "sum");
    pivot_table_sum->head(10, 0);
    
    std::cout << "\nPivot Table with count aggregation:\n";
    auto pivot_table_count = pivot_data->pivot_table("revenue", "region", "quarter", "count");
    pivot_table_count->head(10, 0);
    
    // ============================================================
    // Example 6: Practical Use Case - Monthly Report
    // ============================================================
    print_separator("6. Practical Use Case - Monthly Sales Report");
    
    // Create monthly sales data
    auto monthly = std::make_shared<DataFrame>(std::unordered_map<std::string, Series>{
        {"month", from_torch(torch::tensor({1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 3.0, 3.0, 3.0}))},
        {"category", from_torch(torch::tensor({1.0, 2.0, 3.0, 1.0, 2.0, 3.0, 1.0, 2.0, 3.0}))},
        {"units_sold", from_torch(torch::tensor({100.0, 85.0, 120.0, 110.0, 90.0, 130.0, 95.0, 88.0, 125.0}))},
        {"price", from_torch(torch::tensor({10.5, 15.0, 8.0, 10.5, 15.0, 8.0, 10.5, 15.0, 8.0}))}
    });
    
    std::cout << "Monthly sales data:\n";
    monthly->head(10, 0);
    
    // Create pivot table: months as rows, categories as columns
    std::cout << "\nPivot: Units sold by month and category\n";
    auto units_pivot = monthly->pivot_table("units_sold", "month", "category", "sum");
    units_pivot->head(10, 0);
    
    // Group by month to get total statistics
    std::cout << "\nMonthly totals (grouped by month):\n";
    auto monthly_totals = monthly->groupby("month");
    auto totals = monthly_totals->sum();
    totals->head(10, 0);
    
    // ============================================================
    // Example 7: Statistical Analysis with GroupBy
    // ============================================================
    print_separator("7. Statistical Analysis - std, min, max");
    
    std::cout << "Standard deviation by region:\n";
    auto std_result = grouped->std();
    std_result->head(10, 0);
    
    std::cout << "\nMinimum values by region:\n";
    auto min_result = grouped->min();
    min_result->head(10, 0);
    
    std::cout << "\nMaximum values by region:\n";
    auto max_result = grouped->max();
    max_result->head(10, 0);
    
    std::cout << "\nFirst observation in each group:\n";
    auto first_result = grouped->first();
    first_result->head(10, 0);
    
    std::cout << "\nLast observation in each group:\n";
    auto last_result = grouped->last();
    last_result->head(10, 0);
    
    print_separator("Examples Complete!");
    
    std::cout << "\n📊 Summary of Operations:\n";
    std::cout << "  ✅ GroupBy: groupby(col) or groupby({col1, col2})\n";
    std::cout << "  ✅ Aggregations: sum(), mean(), median(), std(), var(), count(), min(), max()\n";
    std::cout << "  ✅ Multi-agg: agg({\"mean\", \"std\", \"count\"})\n";
    std::cout << "  ✅ Pivot: pivot(index, columns, values)\n";
    std::cout << "  ✅ Pivot Table: pivot_table(values, index, columns, aggfunc)\n";
    std::cout << "  ✅ Aggfuncs: \"mean\", \"sum\", \"min\", \"max\", \"count\", \"median\"\n\n";
    
    return 0;
}
