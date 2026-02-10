#include <pluss/datalog/table/dataframe/dataframe.h>
#include <pluss/datalog/table/thread/thread.h>
#include <pluss/pluss.h>
#include <unordered_map>
#include <algorithm>

namespace pluss::table {

// GroupBy constructor
DataFrame::GroupBy::GroupBy(const DataFrame* parent, const std::vector<std::string>& by_cols)
    : parent_(parent), by_cols_(by_cols) {
    compute_groups();
}

// Aggregation operations
std::shared_ptr<DataFrame> DataFrame::GroupBy::sum() const {
    return aggregate([](const Thread& t) { return t.sum(); }, "sum");
}

std::shared_ptr<DataFrame> DataFrame::GroupBy::mean() const {
    return aggregate([](const Thread& t) { return t.mean(); }, "mean");
}

std::shared_ptr<DataFrame> DataFrame::GroupBy::median() const {
    return aggregate([](const Thread& t) { return t.median(); }, "median");
}

std::shared_ptr<DataFrame> DataFrame::GroupBy::min() const {
    return aggregate([](const Thread& t) { return t.min(); }, "min");
}

std::shared_ptr<DataFrame> DataFrame::GroupBy::max() const {
    return aggregate([](const Thread& t) { return t.max(); }, "max");
}

std::shared_ptr<DataFrame> DataFrame::GroupBy::std(int ddof) const {
    return aggregate([ddof](const Thread& t) { return t.std(ddof); }, "std");
}

std::shared_ptr<DataFrame> DataFrame::GroupBy::var(int ddof) const {
    return aggregate([ddof](const Thread& t) { return t.var(ddof); }, "var");
}

std::shared_ptr<DataFrame> DataFrame::GroupBy::count() const {
    return aggregate([](const Thread& t) { return static_cast<double>(t.count()); }, "count");
}

std::shared_ptr<DataFrame> DataFrame::GroupBy::nunique() const {
    return aggregate([](const Thread& t) { return static_cast<double>(t.nunique()); }, "nunique");
}

std::shared_ptr<DataFrame> DataFrame::GroupBy::first() const {
    return aggregate_thread([](const Thread& t) { return t.head(1); }, "first");
}

std::shared_ptr<DataFrame> DataFrame::GroupBy::last() const {
    return aggregate_thread([](const Thread& t) { return t.tail(1); }, "last");
}

// Apply custom aggregation
std::shared_ptr<DataFrame> DataFrame::GroupBy::agg(const std::vector<std::string>& funcs) const {
        auto result = std::make_shared<DataFrame>();
        
        for (const auto& func : funcs) {
            std::shared_ptr<DataFrame> agg_result;
            if (func == "sum") agg_result = sum();
            else if (func == "mean") agg_result = mean();
            else if (func == "median") agg_result = median();
            else if (func == "min") agg_result = min();
            else if (func == "max") agg_result = max();
            else if (func == "std") agg_result = std();
            else if (func == "var") agg_result = var();
            else if (func == "count") agg_result = count();
            else continue;
            
            // Add suffix to distinguish aggregation functions
            for (const auto& col : agg_result->columns()) {
                if (std::find(by_cols_.begin(), by_cols_.end(), col) == by_cols_.end()) {
                    result = result->set_col(col + "_" + func, agg_result->get_col(col));
                }
            }
        }
        
        // Add group keys
        if (!groups_.empty()) {
            for (const auto& by_col : by_cols_) {
                std::vector<double> keys;
                for (const auto& [key, _] : groups_) {
                    keys.push_back(std::stod(key)); // Simplified - assumes numeric keys
                }
                result = result->set_col(by_col, Thread(pluss::from_blob(keys.data(), {static_cast<int64_t>(keys.size())}), by_col));
            }
        }
        
        return result;
    }

// Private helper: compute groups
void DataFrame::GroupBy::compute_groups() {
    if (by_cols_.empty() || !parent_) return;

        // Get the grouping column(s)
        auto group_threads = std::vector<Thread>();
        for (const auto& col : by_cols_) {
            group_threads.push_back(parent_->get_col(col));
        }

        // Vectorized group computation
        const int64_t num_rows = group_threads[0].size();
        
        // Create group keys by combining values from all grouping columns
        for (int64_t i = 0; i < num_rows; ++i) {
            std::string group_key;
            for (size_t j = 0; j < group_threads.size(); ++j) {
                if (j > 0) group_key += "_";
                group_key += group_threads[j].cell_value(i);
            }
            groups_[group_key].push_back(i);
        }
}

// Aggregate using scalar function
std::shared_ptr<DataFrame> DataFrame::GroupBy::aggregate(const std::function<double(const Thread&)>& agg_func, const std::string& /*func_name*/) const {
    auto result = std::make_shared<DataFrame>();
    
    const auto all_cols = parent_->columns();
        // For each non-grouping column, compute aggregation
    for (const auto& col : all_cols) {
        if (std::find(by_cols_.begin(), by_cols_.end(), col) != by_cols_.end()) {
            continue; // Skip grouping columns
        }
        
        std::vector<double> agg_values;
        agg_values.reserve(groups_.size());
        
        // For each group, apply aggregation
        for (const auto& [group_key, indices] : groups_) {
            // Extract rows for this group using vectorized indexing
            auto col_thread = parent_->get_col(col);
            
            // Create mask for this group
            std::vector<uint8_t> mask(col_thread.size(), 0);
            for (int64_t idx : indices) {
                if (idx < col_thread.size()) mask[idx] = 1;
            }
            
            // Apply mask and aggregate
            auto mask_series = pluss::from_blob(mask.data(), {static_cast<int64_t>(mask.size())}, pluss::SeriesOptions().dtype(at::kByte)).to(at::kBool);
            auto filtered = col_thread.loc(mask_series);
            
            double agg_val = agg_func(filtered);
            agg_values.push_back(agg_val);
        }
        
        result = result->set_col(col, Thread(pluss::from_blob(agg_values.data(), {static_cast<int64_t>(agg_values.size())}), col));
    }
    
    // Add group keys as columns
    for (const auto& by_col : by_cols_) {
        std::vector<double> keys;
        keys.reserve(groups_.size());
        
        for (const auto& [group_key, _] : groups_) {
            // Extract the value for this grouping column from the key
            // Simplified: assumes single numeric grouping column
            try {
                keys.push_back(std::stod(group_key));
            } catch (...) {
                keys.push_back(0.0);
            }
        }
        
        result = result->set_col(by_col, Thread(pluss::from_blob(keys.data(), {static_cast<int64_t>(keys.size())}), by_col));
    }
    
    return result;
}

// Aggregate using Thread function (returns Thread, not scalar)
std::shared_ptr<DataFrame> DataFrame::GroupBy::aggregate_thread(const std::function<Thread(const Thread&)>& agg_func, const std::string& /*func_name*/) const {
    auto result = std::make_shared<DataFrame>();
    
    const auto all_cols = parent_->columns();
    
    for (const auto& col : all_cols) {
        if (std::find(by_cols_.begin(), by_cols_.end(), col) != by_cols_.end()) {
            continue;
        }
        
        std::vector<Thread> group_results;
        
        for (const auto& [group_key, indices] : groups_) {
            auto col_thread = parent_->get_col(col);
            
            std::vector<uint8_t> mask(col_thread.size(), 0);
            for (int64_t idx : indices) {
                if (idx < col_thread.size()) mask[idx] = 1;
            }
            
            auto mask_series = pluss::from_blob(mask.data(), {static_cast<int64_t>(mask.size())}, pluss::SeriesOptions().dtype(at::kByte)).to(at::kBool);
            auto filtered = col_thread.loc(mask_series);
            
            group_results.push_back(agg_func(filtered));
        }
        
        // Concatenate all group results
        if (!group_results.empty()) {
            // For simplicity, take first element of each group
            std::vector<double> values;
            for (const auto& t : group_results) {
                if (t.is_series() && t.size() > 0) {
                    values.push_back(t.as_series()[0].item<double>());
                }
            }
            result = result->set_col(col, Thread(pluss::from_blob(values.data(), {static_cast<int64_t>(values.size())}), col));
        }
    }
    
    return result;
}

// DataFrame methods
std::shared_ptr<DataFrame::GroupBy> DataFrame::groupby(const std::string& by) const {
    return std::make_shared<GroupBy>(this, std::vector<std::string>{by});
}

std::shared_ptr<DataFrame::GroupBy> DataFrame::groupby(const std::vector<std::string>& by) const {
    return std::make_shared<GroupBy>(this, by);
}

std::shared_ptr<DataFrame::GroupBy> DataFrame::groupby(std::initializer_list<std::string> by) const {
    return std::make_shared<GroupBy>(this, std::vector<std::string>(by));
}

} // namespace pluss::table
