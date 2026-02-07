#include <pluss/datalog/table/dataframe/dataframe.h>
#include <pluss/datalog/table/thread/thread.h>
#include <pluss/pluss.h>
#include <unordered_map>
#include <set>
#include <algorithm>

namespace pluss::table {

// Pivot implementation - reshapes data from long to wide format
std::shared_ptr<DataFrame> DataFrame::pivot(
    const std::string& case_col,
    const std::string& columns,
    const std::string& values) const {
    
    // Get the columns as threads
    auto case_thread = get_col(case_col);
    auto columns_thread = get_col(columns);
    auto values_thread = get_col(values);
    
    const int64_t num_rows = case_thread.size();
    
    // Extract unique values for case and columns (vectorized)
    auto unique_cases = case_thread.unique();
    auto unique_columns = columns_thread.unique();
    
    // Create result DataFrame structure
    auto result = std::make_shared<DataFrame>();
    
    // Get unique case values as vector
    std::vector<double> case_vals;
    if (unique_cases.is_series()) {
        auto tensor = at::Tensor(unique_cases.as_series().data());
        auto flat = tensor.view({-1});
        case_vals.resize(flat.numel());
        
        for (int64_t i = 0; i < flat.numel(); ++i) {
            case_vals[i] = flat[i].item<double>();
        }
    }
    
    // Get unique column values as vector
    std::vector<std::string> col_vals;
    if (unique_columns.is_series()) {
        auto tensor = at::Tensor(unique_columns.as_series().data());
        auto flat = tensor.view({-1});
        
        for (int64_t i = 0; i < flat.numel(); ++i) {
            col_vals.push_back(std::to_string(flat[i].item<double>()));
        }
    }
    
    // Build pivot table using vectorized operations
    // Create a map: (case_val, col_val) -> value
    std::unordered_map<std::string, double> pivot_map;
    
    for (int64_t i = 0; i < num_rows; ++i) {
        std::string case_key = case_thread.cell_value(i);
        std::string col_key = columns_thread.cell_value(i);
        std::string map_key = case_key + "_" + col_key;
        
        if (values_thread.is_series()) {
            pivot_map[map_key] = values_thread.as_series()[i].item<double>();
        }
    }
    
    // Build result columns
    for (const auto& col_val : col_vals) {
        std::vector<double> col_data;
        col_data.reserve(case_vals.size());
        
        for (const auto& case_val : case_vals) {
            std::string map_key = std::to_string(case_val) + "_" + col_val;
            auto it = pivot_map.find(map_key);
            if (it != pivot_map.end()) {
                col_data.push_back(it->second);
            } else {
                col_data.push_back(NAN); // Missing value
            }
        }
        
        result = result->set_col(col_val, Thread(
            pluss::from_blob(col_data.data(), {static_cast<int64_t>(col_data.size())}), 
            col_val));
    }
    
    // Add case column
    result = result->set_col(case_col, Thread(
        pluss::from_blob(case_vals.data(), {static_cast<int64_t>(case_vals.size())}), 
        case_col));
    
    return result;
}

// Pivot table with aggregation - more flexible than pivot
std::shared_ptr<DataFrame> DataFrame::pivot_table(
    const std::string& values,
    const std::string& case_col,
    const std::string& columns,
    const std::string& aggfunc) const {
    
    // Get threads
    auto case_thread = get_col(case_col);
    auto columns_thread = get_col(columns);
    auto values_thread = get_col(values);
    
    const int64_t num_rows = case_thread.size();
    
    // Get unique values (vectorized)
    auto unique_cases = case_thread.unique();
    auto unique_columns = columns_thread.unique();
    
    // Extract unique values as vectors
    std::vector<double> case_vals;
    if (unique_cases.is_series()) {
        auto tensor = at::Tensor(unique_cases.as_series().data());
        auto flat = tensor.view({-1});
        case_vals.resize(flat.numel());
        for (int64_t i = 0; i < flat.numel(); ++i) {
            case_vals[i] = flat[i].item<double>();
        }
    }
    
    std::vector<std::string> col_vals;
    if (unique_columns.is_series()) {
        auto tensor = at::Tensor(unique_columns.as_series().data());
        auto flat = tensor.view({-1});
        for (int64_t i = 0; i < flat.numel(); ++i) {
            col_vals.push_back(std::to_string(flat[i].item<double>()));
        }
    }
    
    // Build groups: (case_val, col_val) -> vector of values
    std::unordered_map<std::string, std::vector<double>> groups;
    
    for (int64_t i = 0; i < num_rows; ++i) {
        std::string case_key = case_thread.cell_value(i);
        std::string col_key = columns_thread.cell_value(i);
        std::string map_key = case_key + "_" + col_key;
        
        if (values_thread.is_series()) {
            groups[map_key].push_back(values_thread.as_series()[i].item<double>());
        }
    }
    
    // Apply aggregation function
    auto agg_func = [&aggfunc](const std::vector<double>& vals) -> double {
        if (vals.empty()) return NAN;
        
        if (aggfunc == "mean") {
            double sum = 0.0;
            for (double v : vals) sum += v;
            return sum / vals.size();
        } else if (aggfunc == "sum") {
            double sum = 0.0;
            for (double v : vals) sum += v;
            return sum;
        } else if (aggfunc == "min") {
            return *std::min_element(vals.begin(), vals.end());
        } else if (aggfunc == "max") {
            return *std::max_element(vals.begin(), vals.end());
        } else if (aggfunc == "count") {
            return static_cast<double>(vals.size());
        } else if (aggfunc == "median") {
            auto sorted = vals;
            std::sort(sorted.begin(), sorted.end());
            size_t mid = sorted.size() / 2;
            if (sorted.size() % 2 == 0) {
                return (sorted[mid - 1] + sorted[mid]) / 2.0;
            } else {
                return sorted[mid];
            }
        }
        
        // Default to mean
        double sum = 0.0;
        for (double v : vals) sum += v;
        return sum / vals.size();
    };
    
    // Build result DataFrame
    auto result = std::make_shared<DataFrame>();
    
    for (const auto& col_val : col_vals) {
        std::vector<double> col_data;
        col_data.reserve(case_vals.size());
        
        for (const auto& case_val : case_vals) {
            std::string map_key = std::to_string(case_val) + "_" + col_val;
            auto it = groups.find(map_key);
            if (it != groups.end()) {
                col_data.push_back(agg_func(it->second));
            } else {
                col_data.push_back(NAN);
            }
        }
        
        result = result->set_col(col_val, Thread(
            pluss::from_blob(col_data.data(), {static_cast<int64_t>(col_data.size())}), 
            col_val));
    }
    
    // Add case column
    result = result->set_col(case_col, Thread(
        pluss::from_blob(case_vals.data(), {static_cast<int64_t>(case_vals.size())}), 
        case_col));
    
    return result;
}

} // namespace pluss::table
