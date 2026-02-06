#include <pluss/datalog/table/dataframe/dataframe.h>
#include <pluss/datalog/table/thread/thread.h>
#include <pluss/pluss.h>
#include <unordered_map>
#include <set>
#include <algorithm>

namespace pluss::table {

// Pivot implementation - reshapes data from long to wide format
std::shared_ptr<DataFrame> DataFrame::pivot(
    const std::string& index,
    const std::string& columns,
    const std::string& values) const {
    
    // Get the columns as threads
    auto index_thread = get_col(index);
    auto columns_thread = get_col(columns);
    auto values_thread = get_col(values);
    
    const int64_t num_rows = index_thread.size();
    
    // Extract unique values for index and columns (vectorized)
    auto unique_indices = index_thread.unique();
    auto unique_columns = columns_thread.unique();
    
    // Create result DataFrame structure
    auto result = std::make_shared<DataFrame>();
    
    // Get unique index values as vector
    std::vector<double> index_vals;
    if (unique_indices.is_series()) {
        auto tensor = at::Tensor(unique_indices.as_series().data());
        auto flat = tensor.view({-1});
        index_vals.resize(flat.numel());
        
        for (int64_t i = 0; i < flat.numel(); ++i) {
            index_vals[i] = flat[i].item<double>();
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
    // Create a map: (index_val, col_val) -> value
    std::unordered_map<std::string, double> pivot_map;
    
    for (int64_t i = 0; i < num_rows; ++i) {
        std::string idx_key = index_thread.cell_value(i);
        std::string col_key = columns_thread.cell_value(i);
        std::string map_key = idx_key + "_" + col_key;
        
        if (values_thread.is_series()) {
            pivot_map[map_key] = values_thread.as_series()[i].item<double>();
        }
    }
    
    // Build result columns
    for (const auto& col_val : col_vals) {
        std::vector<double> col_data;
        col_data.reserve(index_vals.size());
        
        for (const auto& idx_val : index_vals) {
            std::string map_key = std::to_string(idx_val) + "_" + col_val;
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
    
    // Add index column
    result = result->set_col(index, Thread(
        pluss::from_blob(index_vals.data(), {static_cast<int64_t>(index_vals.size())}), 
        index));
    
    return result;
}

// Pivot table with aggregation - more flexible than pivot
std::shared_ptr<DataFrame> DataFrame::pivot_table(
    const std::string& values,
    const std::string& index,
    const std::string& columns,
    const std::string& aggfunc) const {
    
    // Get threads
    auto index_thread = get_col(index);
    auto columns_thread = get_col(columns);
    auto values_thread = get_col(values);
    
    const int64_t num_rows = index_thread.size();
    
    // Get unique values (vectorized)
    auto unique_indices = index_thread.unique();
    auto unique_columns = columns_thread.unique();
    
    // Extract unique values as vectors
    std::vector<double> index_vals;
    if (unique_indices.is_series()) {
        auto tensor = at::Tensor(unique_indices.as_series().data());
        auto flat = tensor.view({-1});
        index_vals.resize(flat.numel());
        for (int64_t i = 0; i < flat.numel(); ++i) {
            index_vals[i] = flat[i].item<double>();
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
    
    // Build groups: (index_val, col_val) -> vector of values
    std::unordered_map<std::string, std::vector<double>> groups;
    
    for (int64_t i = 0; i < num_rows; ++i) {
        std::string idx_key = index_thread.cell_value(i);
        std::string col_key = columns_thread.cell_value(i);
        std::string map_key = idx_key + "_" + col_key;
        
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
        col_data.reserve(index_vals.size());
        
        for (const auto& idx_val : index_vals) {
            std::string map_key = std::to_string(idx_val) + "_" + col_val;
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
    
    // Add index column
    result = result->set_col(index, Thread(
        pluss::from_blob(index_vals.data(), {static_cast<int64_t>(index_vals.size())}), 
        index));
    
    return result;
}

} // namespace pluss::table
