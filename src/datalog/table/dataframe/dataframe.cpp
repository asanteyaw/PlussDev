
// High-level, pandas-like presentation helpers.
// Core storage/selection logic lives in dataframe_impl.cpp.

#include <pluss/datalog/table/dataframe/dataframe.h>
#include <pluss/datalog/table/thread/thread.h>

#include <pluss/datalog/table/io/display.h>

#include <pluss/pluss.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace pluss::table {

namespace {

// Helper to calculate non-null count for a Thread
int64_t non_null_count(const pluss::table::Thread& t) {
    try {
        if (t.is_series()) {
            auto tt = at::Tensor(static_cast<const at::TensorBase&>(t.as_series())).view({-1});
            if (tt.numel() == 0) return 0;
            if (tt.is_floating_point()) return (tt == tt).sum().item<int64_t>(); // NaN != NaN
            return tt.numel();
        }
        if (t.is_timeline()) {
            const auto& tl = t.as_timeline();
            auto ds = at::Tensor(static_cast<const at::TensorBase&>(tl.date_to_Series())).view({-1});
            auto ts = at::Tensor(static_cast<const at::TensorBase&>(tl.time_to_Series())).view({-1});
            const int64_t n = std::max<int64_t>(ds.numel(), ts.numel());
            if (n == 0) return 0;
            if (ds.numel() == 0) return (ts != 0).sum().item<int64_t>();
            if (ts.numel() == 0) return (ds != 0).sum().item<int64_t>();
            return ((ds != 0) | (ts != 0)).sum().item<int64_t>();
        }
        if (t.is_note()) {
            auto v = t.as_note().vector();
            int64_t c = 0;
            for (const auto& chord : v) if (!chord.str().empty()) ++c;
            return c;
        }
    } catch (...) {
    }
    return t.size();
}

} // namespace

void DataFrame::head(int64_t n) const {
    pluss::table::io::DisplayOptions opt;
    opt.max_rows = n;  // Limit to n rows
    opt.head_rows = n;
    opt.tail_rows = 0;
    opt.show_summary = false;
    std::cout << pluss::table::io::to_string(*this, opt);
}

void DataFrame::tail(int64_t n) const {
    auto col_names = this->columns();
    if (col_names.empty()) {
        std::cout << "Empty DataFrame\n";
        return;
    }
    const int64_t rows = this->get_col(col_names.front()).size();
    const int64_t start = std::max<int64_t>(0, rows - n);
    auto df_tail = this->iloc(start, rows);
    
    pluss::table::io::DisplayOptions opt;
    opt.max_rows = 0; // show all rows in the slice
    opt.show_summary = false;
    std::cout << pluss::table::io::to_string(*df_tail, opt);
}

void DataFrame::info() const {
    const auto col_names = this->columns();
    const size_t num_cols = col_names.size();

    size_t num_rows = 0;
    if (!col_names.empty()) num_rows = static_cast<size_t>(this->get_col(col_names.front()).size());

    std::cout << "<class 'pluss::table::DataFrame'>\n";
    std::cout << "Index: 0 to " << (num_rows == 0 ? 0 : num_rows - 1) << "\n";
    std::cout << "Data columns (total " << num_cols << "):\n";

    // Calculate widths dynamically
    int idx_width = 1;
    if (!col_names.empty()) {
        idx_width = std::max(1, static_cast<int>(std::to_string(col_names.size() - 1).size()));
    }
    
    int name_width = 6; // "Column"
    for (const auto& name : col_names) {
        name_width = std::max(name_width, static_cast<int>(name.size()));
    }
    name_width = std::min(name_width, 32);

    const int count_width = 14;
    const int dtype_width = 8;

    // Header
    std::cout << std::left << std::setw(idx_width + 2) << "#" 
              << std::setw(name_width + 2) << "Column" 
              << std::setw(count_width + 2) << "Non-Null Count" 
              << std::setw(dtype_width) << "Dtype" << "\n";
    std::cout << std::string(static_cast<size_t>(idx_width + name_width + count_width + dtype_width + 6), '-') << "\n";

    // Rows
    for (size_t i = 0; i < col_names.size(); ++i) {
        const auto t = this->get_col(col_names[i]);
        const std::string dtype = t.is_series() ? "Series" : (t.is_timeline() ? "Timeline" : "Note");
        
        const int64_t nn_count = non_null_count(t);
        std::ostringstream count_str;
        count_str << nn_count << " non-null";

        const std::string col_display = pluss::table::io::detail::ellipsize(col_names[i], name_width, pluss::table::io::DisplayOptions{});
        
        std::cout << std::left << std::setw(idx_width + 2) << static_cast<int>(i)
                  << std::setw(name_width + 2) << col_display
                  << std::setw(count_width + 2) << count_str.str()
                  << std::setw(dtype_width) << dtype << "\n";
    }
}

void DataFrame::describe() const {
    const auto col_names = this->columns();
    if (col_names.empty()) {
        std::cout << "Empty DataFrame\n";
        return;
    }

    // Filter to only numeric (Series) columns and compute stats
    std::vector<std::string> numeric_cols;
    std::vector<std::vector<double>> stats; // count, mean, std, min, 25%, 50%, 75%, max
    
    for (const auto& name : col_names) {
        const auto t = this->get_col(name);
        if (!t.is_series()) continue;
        
        try {
            auto s = t.as_series();
            // Filter out NaN values for statistics
            auto mask = ~torch::isnan(s.data());
            auto valid = torch::masked_select(s.data(), mask);
            
            if (valid.numel() == 0) continue; // Skip if all NaN
            
            auto valid_float = valid.to(torch::kFloat64);
            
            double count = static_cast<double>(valid.numel());
            double mean = valid_float.mean().item<double>();
            double stddev = valid_float.std().item<double>();
            double min_val = valid_float.min().item<double>();
            double q25 = valid_float.quantile(0.25).item<double>();
            double median = valid_float.median().item<double>();
            double q75 = valid_float.quantile(0.75).item<double>();
            double max_val = valid_float.max().item<double>();
            
            numeric_cols.push_back(name);
            stats.push_back({count, mean, stddev, min_val, q25, median, q75, max_val});
        } catch (const std::exception& e) {
            // Skip columns that can't be converted to numeric
            continue;
        }
    }

    if (numeric_cols.empty()) {
        std::cout << "No numeric columns to describe.\n";
        return;
    }

    const std::vector<std::string> stat_names = {"count", "mean", "std", "min", "25%", "50%", "75%", "max"};
    
    // Use fixed column width like pandas - reduced for better terminal fit
    const int col_width = 20;
    const int stat_width = 5; // "count"
    
    // Determine if we need to truncate columns (like pandas does)
    const int max_display_cols = 6; // Limit for readability
    bool needs_truncation = numeric_cols.size() > max_display_cols;
    std::vector<std::string> display_cols;
    
    if (needs_truncation) {
        // Show first 3 and last 3
        for (size_t i = 0; i < 3 && i < numeric_cols.size(); ++i) {
            display_cols.push_back(numeric_cols[i]);
        }
        display_cols.push_back("...");
        for (size_t i = numeric_cols.size() - 3; i < numeric_cols.size(); ++i) {
            display_cols.push_back(numeric_cols[i]);
        }
    } else {
        display_cols = numeric_cols;
    }
    
    // Header row with column names
    std::cout << std::setw(stat_width) << " ";
    for (const auto& col_name : display_cols) {
        std::string col_display = col_name;
        if (col_display.length() > col_width - 1) {
            col_display = col_display.substr(0, col_width - 1);
        }
        std::cout << std::setw(col_width) << std::right << col_display;
    }
    std::cout << "\n";
    
    // Stats rows
    for (size_t stat_idx = 0; stat_idx < stat_names.size(); ++stat_idx) {
        std::cout << std::setw(stat_width) << std::left << stat_names[stat_idx];
        
        if (needs_truncation) {
            // First 3 columns
            for (size_t i = 0; i < 3 && i < numeric_cols.size(); ++i) {
                double val = stats[i][stat_idx];
                std::ostringstream tmp;
                tmp << std::fixed << std::setprecision(6) << val;
                std::string formatted = tmp.str();
                if (formatted.length() > col_width - 1) {
                    formatted = formatted.substr(0, col_width - 1);
                }
                std::cout << std::setw(col_width) << std::right << formatted;
            }
            // ... column
            std::cout << std::setw(col_width) << std::right << "...";
            // Last 3 columns
            for (size_t i = numeric_cols.size() - 3; i < numeric_cols.size(); ++i) {
                double val = stats[i][stat_idx];
                std::ostringstream tmp;
                tmp << std::fixed << std::setprecision(6) << val;
                std::string formatted = tmp.str();
                if (formatted.length() > col_width - 1) {
                    formatted = formatted.substr(0, col_width - 1);
                }
                std::cout << std::setw(col_width) << std::right << formatted;
            }
        } else {
            // Show all columns
            for (size_t col_idx = 0; col_idx < numeric_cols.size(); ++col_idx) {
                double val = stats[col_idx][stat_idx];
                std::ostringstream tmp;
                tmp << std::fixed << std::setprecision(6) << val;
                std::string formatted = tmp.str();
                if (formatted.length() > col_width - 1) {
                    formatted = formatted.substr(0, col_width - 1);
                }
                std::cout << std::setw(col_width) << std::right << formatted;
            }
        }
        std::cout << "\n";
    }
    
    // Summary line like pandas
    if (needs_truncation) {
        std::cout << "\n[8 rows x " << numeric_cols.size() << " columns]\n";
    }
}

} // namespace pluss::table

