
// High-level, pandas-like presentation helpers.
// Core storage/selection logic lives in dataframe_impl.cpp.

#include <pluss/datalog/table/dataframe/dataframe.h>
#include <pluss/datalog/table/thread/thread.h>

#include <pluss/datalog/table/io/display.h>

#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>

namespace pluss::table {

namespace {

// Helper to calculate non-null count for a Thread
// Thread-First: Use Thread.count() which handles all types properly
int64_t non_null_count(const pluss::table::Thread& t) {
    return t.count();
}

} // namespace

void DataFrame::head(int64_t n, int format) const {
    pluss::table::io::DisplayOptions opt;
    opt.format = static_cast<pluss::table::io::OutputFormat>(format);
    
    // For head(), show exactly n rows from the top
    auto cols = this->columns();
    int64_t total = cols.empty() ? 0 : this->get_col(cols[0]).size();
    
    if (total <= n) {
        // Show all rows without truncation
        opt.max_rows = 0;
    } else {
        // Show first n rows, with ellipsis if needed, no tail
        opt.max_rows = n;
        opt.head_rows = n;
        opt.tail_rows = 0;  // No tail rows
    }
    opt.show_summary = false;
    
    std::string result;
    if (opt.format == pluss::table::io::OutputFormat::HTML) {
        result = pluss::table::io::to_html(*this, opt);
    } else if (opt.format == pluss::table::io::OutputFormat::LATEX) {
        result = pluss::table::io::to_latex(*this, opt);
    } else {
        result = pluss::table::io::to_string(*this, opt);
    }
    std::cout << result;
}

void DataFrame::tail(int64_t n, int format) const {
    auto col_names = this->columns();
    if (col_names.empty()) {
        std::cout << "Empty DataFrame\n";
        return;
    }
    const int64_t rows = this->get_col(col_names.front()).size();
    const int64_t start = std::max<int64_t>(0, rows - n);
    auto df_tail = this->iloc(start, rows);
    
    pluss::table::io::DisplayOptions opt;
    opt.format = static_cast<pluss::table::io::OutputFormat>(format);
    opt.max_rows = 0;  // Show all rows in the tail slice
    opt.show_summary = false;  // tail() is for peeking, no summary
    
    std::string result;
    if (opt.format == pluss::table::io::OutputFormat::HTML) {
        result = pluss::table::io::to_html(*df_tail, opt);
    } else if (opt.format == pluss::table::io::OutputFormat::LATEX) {
        result = pluss::table::io::to_latex(*df_tail, opt);
    } else {
        result = pluss::table::io::to_string(*df_tail, opt);
    }
    std::cout << result;
}

void DataFrame::info(int format) const {
    const auto col_names = this->columns();
    if (col_names.empty()) {
        std::cout << "Empty DataFrame\n";
        return;
    }

    size_t num_rows = 0;
    if (!col_names.empty()) num_rows = static_cast<size_t>(this->get_col(col_names.front()).size());

    std::cout << "<class 'pluss::table::DataFrame'>\n";
    std::cout << "Case: 0 to " << (num_rows == 0 ? 0 : num_rows - 1) << "\n";
    std::cout << "Data columns (total " << col_names.size() << "):\n";

    // Build DataFrame with column info (Column, Non-Null Count, Dtype)
    std::vector<std::string> col_data;
    std::vector<int64_t> count_data;
    std::vector<std::string> dtype_data;
    
    for (const auto& name : col_names) {
        const auto t = this->get_col(name);
        col_data.push_back(name);
        count_data.push_back(non_null_count(t));
        dtype_data.push_back(t.is_series() ? "Series" : (t.is_timeline() ? "Timeline" : "Note"));
    }
    
    // Create DataFrame for info display
    std::shared_ptr<DataFrame> info_df = std::make_shared<DataFrame>();
    
    // Convert column names to Note (strings)
    std::vector<pluss::Chord> col_chords;
    for (const auto& name : col_data) {
        col_chords.push_back(pluss::Chord(name));
    }
    info_df = info_df->set_col("Column", pluss::make_note(col_chords));
    
    // Add non-null counts as Series
    info_df = info_df->set_col("Non-Null Count", pluss::value(count_data));
    
    // Add dtypes as Note
    std::vector<pluss::Chord> dtype_chords;
    for (const auto& dtype : dtype_data) {
        dtype_chords.push_back(pluss::Chord(dtype));
    }
    info_df = info_df->set_col("Dtype", pluss::make_note(dtype_chords));
    
    // Use the same display engine as head/tail/describe
    pluss::table::io::DisplayOptions opt;
    opt.format = static_cast<pluss::table::io::OutputFormat>(format);
    opt.max_rows = 0;  // Show all rows
    opt.show_summary = false;  // info() is metadata, no summary
    
    std::string result;
    if (opt.format == pluss::table::io::OutputFormat::HTML) {
        result = pluss::table::io::to_html(*info_df, opt);
    } else if (opt.format == pluss::table::io::OutputFormat::LATEX) {
        result = pluss::table::io::to_latex(*info_df, opt);
    } else {
        result = pluss::table::io::to_string(*info_df, opt);
    }
    std::cout << result;
}

void DataFrame::describe(int format) const {
    // Thread-First: Use Thread aggregation methods instead of raw series operations
    const auto col_names = this->columns();
    if (col_names.empty()) {
        std::cout << "Empty DataFrame\n";
        return;
    }

    // Compute statistics for numeric columns using Thread API
    std::vector<std::string> numeric_cols;
    std::vector<std::vector<double>> stats;
    
    for (const auto& name : col_names) {
        const auto thread = this->get_col(name);
        if (!thread.is_series()) continue;
        
        try {
            // Use Thread aggregation methods
            const double count = static_cast<double>(thread.count());
            if (count == 0) continue;
            
            const double mean_val = thread.mean();
            const double std_val = thread.std(1);  // ddof=1
            const double min_val = thread.min();
            const double q25 = thread.quantile(0.25);
            const double median_val = thread.median();
            const double q75 = thread.quantile(0.75);
            const double max_val = thread.max();
            
            numeric_cols.push_back(name);
            stats.push_back({count, mean_val, std_val, min_val, q25, median_val, q75, max_val});
        } catch (const std::exception& e) {
            continue;
        }
    }

    if (numeric_cols.empty()) {
        std::cout << "No numeric columns to describe.\n";
        return;
    }

    // Create a DataFrame with stats and use to_string() - THE SAME ENGINE AS head()/tail()
    std::shared_ptr<DataFrame> stats_df = std::make_shared<DataFrame>();
    for (size_t i = 0; i < numeric_cols.size(); ++i) {
        stats_df = stats_df->set_col(numeric_cols[i], pluss::value(stats[i]));
    }
    
    pluss::table::io::DisplayOptions opt;
    opt.format = static_cast<pluss::table::io::OutputFormat>(format);
    opt.max_rows = 0;  // Show all 8 stat rows
    opt.show_summary = true;  // describe() shows statistical summary with dimensions
    
    std::string result;
    if (opt.format == pluss::table::io::OutputFormat::HTML) {
        result = pluss::table::io::to_html(*stats_df, opt);
    } else if (opt.format == pluss::table::io::OutputFormat::LATEX) {
        result = pluss::table::io::to_latex(*stats_df, opt);
    } else {
        result = pluss::table::io::to_string(*stats_df, opt);
    }
    std::cout << result;
}

std::string DataFrame::to_html() const {
    return pluss::table::io::to_html(*this);
}

std::string DataFrame::to_latex() const {
    return pluss::table::io::to_latex(*this);
}

std::string DataFrame::to_string() const {
    pluss::table::io::DisplayOptions opt;
    opt.format = pluss::table::io::OutputFormat::TEXT;
    return pluss::table::io::to_string(*this, opt);
}

} // namespace pluss::table

