#pragma once

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <torch/torch.h>
#include <pluss/Series.h>
#include <pluss/datalog/table/thread/thread.h>

namespace pluss::table::io {
    enum class OutputFormat;
}

namespace pluss::table {

class DataFrame {
public:
    DataFrame();
    explicit DataFrame(const std::unordered_map<std::string, pluss::Series>& data);
    DataFrame(const std::unordered_map<std::string, pluss::Series>& data, const std::vector<std::string>& column_order);
    DataFrame(const DataFrame& other);
    DataFrame& operator=(const DataFrame& other);
    DataFrame(DataFrame&& other) noexcept;
    DataFrame& operator=(DataFrame&& other) noexcept;
    ~DataFrame();

    // Loader
    static std::shared_ptr<DataFrame> load(const std::string& format, const std::string& filename);

    // Get column names
    [[nodiscard]] std::vector<std::string> columns() const;

    // Conditional filtering (mask should be a boolean Series)
    [[nodiscard]] std::shared_ptr<DataFrame> loc(const pluss::Series& condition) const;

    // Index-based selection
    [[nodiscard]] std::shared_ptr<DataFrame> iloc(int64_t row_start, int64_t row_end) const;
    [[nodiscard]] std::unordered_map<std::string, pluss::table::Thread> iloc(int64_t row) const;

    // Move to device (CPU/GPU)
    [[nodiscard]] std::shared_ptr<DataFrame> to(const torch::Device& device) const;

    [[nodiscard]] std::vector<pluss::table::Thread> values() const;

    [[nodiscard]] std::shared_ptr<DataFrame> T() const;

    // Display methods (support text/html/latex output)
    // format: 0=TEXT, 1=HTML, 2=LATEX (defaults to TEXT for terminal)
    void head(int64_t n = 5, int format = 0) const;
    void tail(int64_t n = 5, int format = 0) const;
    void info(int format = 0) const;
    void describe(int format = 0) const;
    
    // Get formatted output as string (for Jupyter notebook rendering)
    [[nodiscard]] std::string to_html() const;
    [[nodiscard]] std::string to_latex() const;
    [[nodiscard]] std::string to_string() const;

    // Single column selection (returns Series)
    [[nodiscard]] pluss::table::Thread get_col(const std::string& col_name) const;

    // Multiple column selection (returns DataFrame)
    [[nodiscard]] std::shared_ptr<DataFrame> get_cols(const std::vector<std::string>& column_names) const;

    // single column creation
    [[nodiscard]] std::shared_ptr<DataFrame> set_col(const std::string& col_name, const pluss::table::Thread& values) const;

    // multiple column creation
    [[nodiscard]] std::shared_ptr<DataFrame> set_cols(const std::vector<std::string>& column_names, const std::vector<pluss::table::Thread>& values) const;

    // Save CSV
    void to_csv(const std::string& filename) const;

    // ========== PANDAS-STYLE METHODS (Thread-First Design) ==========
    // These delegate to Thread operations for efficient vectorized execution
    
    // Basic operations
    [[nodiscard]] std::shared_ptr<DataFrame> copy() const;
    [[nodiscard]] std::shared_ptr<DataFrame> head(int64_t n = 5) const;
    [[nodiscard]] std::shared_ptr<DataFrame> tail(int64_t n = 5) const;
    [[nodiscard]] std::shared_ptr<DataFrame> sample(int64_t n, bool replace = false) const;
    [[nodiscard]] std::shared_ptr<DataFrame> take(const std::vector<int64_t>& indices) const;
    [[nodiscard]] std::shared_ptr<DataFrame> truncate(int64_t before, int64_t after) const;
    
    // Sorting and indexing
    [[nodiscard]] std::shared_ptr<DataFrame> sort_values(const std::string& by, bool ascending = true) const;
    [[nodiscard]] std::shared_ptr<DataFrame> sort_values(const std::vector<std::string>& by, bool ascending = true) const;
    [[nodiscard]] std::shared_ptr<DataFrame> sort_index(bool ascending = true) const;
    [[nodiscard]] std::shared_ptr<DataFrame> nlargest(int64_t n, const std::string& col) const;
    [[nodiscard]] std::shared_ptr<DataFrame> nsmallest(int64_t n, const std::string& col) const;
    [[nodiscard]] std::shared_ptr<DataFrame> reset_index(bool drop = false) const;
    [[nodiscard]] std::shared_ptr<DataFrame> set_index(const std::string& col_name) const;
    [[nodiscard]] std::string idxmax(const std::string& col) const;
    [[nodiscard]] std::string idxmin(const std::string& col) const;
    
    // Filtering and selection
    [[nodiscard]] std::shared_ptr<DataFrame> drop(const std::vector<std::string>& columns) const;
    [[nodiscard]] std::shared_ptr<DataFrame> drop_duplicates(const std::vector<std::string>& subset = {}) const;
    [[nodiscard]] std::shared_ptr<DataFrame> duplicated(const std::vector<std::string>& subset = {}) const;
    [[nodiscard]] std::shared_ptr<DataFrame> filter(const std::vector<std::string>& items) const;
    [[nodiscard]] std::shared_ptr<DataFrame> select_dtypes(const std::vector<std::string>& include) const;
    [[nodiscard]] std::shared_ptr<DataFrame> where(const std::shared_ptr<DataFrame>& condition, double other) const;
    [[nodiscard]] std::shared_ptr<DataFrame> mask(const std::shared_ptr<DataFrame>& condition, double other) const;
    [[nodiscard]] std::shared_ptr<DataFrame> query(const std::string& expr) const;
    [[nodiscard]] std::shared_ptr<DataFrame> isin(const std::vector<double>& values) const;
    
    // Missing data handling
    [[nodiscard]] std::shared_ptr<DataFrame> dropna(const std::string& how = "any", int64_t thresh = -1) const;
    [[nodiscard]] std::shared_ptr<DataFrame> fillna(double value) const;
    [[nodiscard]] std::shared_ptr<DataFrame> fillna(const std::unordered_map<std::string, double>& values) const;
    [[nodiscard]] std::shared_ptr<DataFrame> ffill() const;  // Forward fill
    [[nodiscard]] std::shared_ptr<DataFrame> bfill() const;  // Backward fill
    [[nodiscard]] std::shared_ptr<DataFrame> interpolate(const std::string& method = "linear") const;
    [[nodiscard]] std::shared_ptr<DataFrame> isna() const;
    [[nodiscard]] std::shared_ptr<DataFrame> notna() const;
    
    // Renaming and replacement
    [[nodiscard]] std::shared_ptr<DataFrame> rename(const std::unordered_map<std::string, std::string>& columns) const;
    [[nodiscard]] std::shared_ptr<DataFrame> add_prefix(const std::string& prefix) const;
    [[nodiscard]] std::shared_ptr<DataFrame> add_suffix(const std::string& suffix) const;
    [[nodiscard]] std::shared_ptr<DataFrame> replace(double old_value, double new_value) const;
    [[nodiscard]] std::shared_ptr<DataFrame> replace(const std::unordered_map<double, double>& to_replace) const;
    
    // Aggregations (single column - return scalar)
    [[nodiscard]] double sum(const std::string& col_name) const;
    [[nodiscard]] double mean(const std::string& col_name) const;
    [[nodiscard]] double median(const std::string& col_name) const;
    [[nodiscard]] double std(const std::string& col_name, int ddof = 1) const;
    [[nodiscard]] double var(const std::string& col_name, int ddof = 1) const;
    [[nodiscard]] double min(const std::string& col_name) const;
    [[nodiscard]] double max(const std::string& col_name) const;
    [[nodiscard]] double prod(const std::string& col_name) const;
    [[nodiscard]] double sem(const std::string& col_name) const;  // Standard error of mean
    [[nodiscard]] double mad(const std::string& col_name) const;  // Mean absolute deviation
    [[nodiscard]] double skew(const std::string& col_name) const;
    [[nodiscard]] double kurt(const std::string& col_name) const;  // Kurtosis
    [[nodiscard]] double quantile(const std::string& col_name, double q) const;
    [[nodiscard]] int64_t count(const std::string& col_name) const;
    [[nodiscard]] int64_t nunique(const std::string& col_name) const;
    
    // Multi-column aggregations (return Series or DataFrame)
    [[nodiscard]] std::shared_ptr<DataFrame> sum() const;
    [[nodiscard]] std::shared_ptr<DataFrame> mean() const;
    [[nodiscard]] std::shared_ptr<DataFrame> median() const;
    [[nodiscard]] std::shared_ptr<DataFrame> std(int ddof = 1) const;
    [[nodiscard]] std::shared_ptr<DataFrame> var(int ddof = 1) const;
    [[nodiscard]] std::shared_ptr<DataFrame> min() const;
    [[nodiscard]] std::shared_ptr<DataFrame> max() const;
    [[nodiscard]] std::shared_ptr<DataFrame> prod() const;
    [[nodiscard]] std::shared_ptr<DataFrame> count() const;
    [[nodiscard]] std::shared_ptr<DataFrame> nunique() const;
    [[nodiscard]] std::shared_ptr<DataFrame> mode() const;
    [[nodiscard]] std::shared_ptr<DataFrame> quantile(double q) const;
    [[nodiscard]] std::shared_ptr<DataFrame> quantile(const std::vector<double>& q) const;
    
    // Statistical operations
    [[nodiscard]] std::shared_ptr<DataFrame> describe(const std::vector<double>& percentiles = {0.25, 0.5, 0.75}) const;
    [[nodiscard]] std::shared_ptr<DataFrame> corr(const std::string& method = "pearson") const;
    [[nodiscard]] std::shared_ptr<DataFrame> cov(int ddof = 1) const;
    [[nodiscard]] std::shared_ptr<DataFrame> corrwith(const std::shared_ptr<DataFrame>& other) const;
    [[nodiscard]] std::shared_ptr<DataFrame> rank(const std::string& method = "average") const;
    [[nodiscard]] std::shared_ptr<DataFrame> pct_change(int64_t periods = 1) const;
    
    // Transformations (delegate to Thread)
    [[nodiscard]] std::shared_ptr<DataFrame> abs() const;
    [[nodiscard]] std::shared_ptr<DataFrame> round(int decimals = 0) const;
    [[nodiscard]] std::shared_ptr<DataFrame> clip(double lower, double upper) const;
    [[nodiscard]] std::shared_ptr<DataFrame> apply(std::function<Thread(const Thread&)> func) const;
    [[nodiscard]] std::shared_ptr<DataFrame> transform(const std::string& func) const;
    [[nodiscard]] std::shared_ptr<DataFrame> diff(int64_t periods = 1) const;
    [[nodiscard]] std::shared_ptr<DataFrame> shift(int64_t periods = 1) const;
    
    // Cumulative operations (delegate to Thread)
    [[nodiscard]] std::shared_ptr<DataFrame> cumsum() const;
    [[nodiscard]] std::shared_ptr<DataFrame> cumprod() const;
    [[nodiscard]] std::shared_ptr<DataFrame> cummax() const;
    [[nodiscard]] std::shared_ptr<DataFrame> cummin() const;
    
    // Comparison operations
    [[nodiscard]] std::shared_ptr<DataFrame> eq(const std::shared_ptr<DataFrame>& other) const;
    [[nodiscard]] std::shared_ptr<DataFrame> ne(const std::shared_ptr<DataFrame>& other) const;
    [[nodiscard]] std::shared_ptr<DataFrame> lt(const std::shared_ptr<DataFrame>& other) const;
    [[nodiscard]] std::shared_ptr<DataFrame> le(const std::shared_ptr<DataFrame>& other) const;
    [[nodiscard]] std::shared_ptr<DataFrame> gt(const std::shared_ptr<DataFrame>& other) const;
    [[nodiscard]] std::shared_ptr<DataFrame> ge(const std::shared_ptr<DataFrame>& other) const;
    
    // Arithmetic operations (element-wise)
    [[nodiscard]] std::shared_ptr<DataFrame> add(const std::shared_ptr<DataFrame>& other, double fill_value = NAN) const;
    [[nodiscard]] std::shared_ptr<DataFrame> sub(const std::shared_ptr<DataFrame>& other, double fill_value = NAN) const;
    [[nodiscard]] std::shared_ptr<DataFrame> mul(const std::shared_ptr<DataFrame>& other, double fill_value = NAN) const;
    [[nodiscard]] std::shared_ptr<DataFrame> div(const std::shared_ptr<DataFrame>& other, double fill_value = NAN) const;
    [[nodiscard]] std::shared_ptr<DataFrame> mod(const std::shared_ptr<DataFrame>& other, double fill_value = NAN) const;
    [[nodiscard]] std::shared_ptr<DataFrame> pow(const std::shared_ptr<DataFrame>& other, double fill_value = NAN) const;
    [[nodiscard]] std::shared_ptr<DataFrame> dot(const std::shared_ptr<DataFrame>& other) const;
    
    // Mathematical functions (delegate to Thread)
    [[nodiscard]] std::shared_ptr<DataFrame> sqrt() const;
    [[nodiscard]] std::shared_ptr<DataFrame> log() const;
    [[nodiscard]] std::shared_ptr<DataFrame> log10() const;
    [[nodiscard]] std::shared_ptr<DataFrame> exp() const;
    [[nodiscard]] std::shared_ptr<DataFrame> sin() const;
    [[nodiscard]] std::shared_ptr<DataFrame> cos() const;
    [[nodiscard]] std::shared_ptr<DataFrame> tan() const;
    
    // Boolean reductions
    [[nodiscard]] bool all(const std::string& col) const;
    [[nodiscard]] bool any(const std::string& col) const;
    [[nodiscard]] std::shared_ptr<DataFrame> all() const;  // All columns
    [[nodiscard]] std::shared_ptr<DataFrame> any() const;  // All columns
    
    // Merging and joining
    [[nodiscard]] std::shared_ptr<DataFrame> merge(
        const std::shared_ptr<DataFrame>& other,
        const std::string& on,
        const std::string& how = "inner") const;
    [[nodiscard]] std::shared_ptr<DataFrame> merge(
        const std::shared_ptr<DataFrame>& other,
        const std::vector<std::string>& on,
        const std::string& how = "inner") const;
    [[nodiscard]] std::shared_ptr<DataFrame> join(
        const std::shared_ptr<DataFrame>& other,
        const std::string& how = "left",
        const std::string& lsuffix = "_left",
        const std::string& rsuffix = "_right") const;
    [[nodiscard]] std::shared_ptr<DataFrame> combine_first(const std::shared_ptr<DataFrame>& other) const;
    [[nodiscard]] std::shared_ptr<DataFrame> update(const std::shared_ptr<DataFrame>& other) const;
    [[nodiscard]] std::shared_ptr<DataFrame> append(const std::shared_ptr<DataFrame>& other, bool ignore_index = false) const;
    static std::shared_ptr<DataFrame> concat(
        const std::vector<std::shared_ptr<DataFrame>>& dfs,
        int axis = 0,
        bool ignore_index = false);
    
    // Reshaping
    [[nodiscard]] std::shared_ptr<DataFrame> pivot(
        const std::string& index,
        const std::string& columns,
        const std::string& values) const;
    [[nodiscard]] std::shared_ptr<DataFrame> pivot_table(
        const std::string& values,
        const std::string& index,
        const std::string& columns,
        const std::string& aggfunc = "mean") const;
    [[nodiscard]] std::shared_ptr<DataFrame> melt(
        const std::vector<std::string>& id_vars,
        const std::vector<std::string>& value_vars = {}) const;
    [[nodiscard]] std::shared_ptr<DataFrame> stack() const;
    [[nodiscard]] std::shared_ptr<DataFrame> unstack() const;
    [[nodiscard]] std::shared_ptr<DataFrame> explode(const std::string& column) const;
    [[nodiscard]] std::shared_ptr<DataFrame> squeeze() const;
    
    // GroupBy operations
    class GroupBy;
    [[nodiscard]] std::shared_ptr<GroupBy> groupby(const std::string& by) const;
    [[nodiscard]] std::shared_ptr<GroupBy> groupby(const std::vector<std::string>& by) const;
    
    // Window operations
    class Rolling;
    class Expanding;
    class EWM;
    [[nodiscard]] std::shared_ptr<Rolling> rolling(int64_t window, int64_t min_periods = -1) const;
    [[nodiscard]] std::shared_ptr<Expanding> expanding(int64_t min_periods = 1) const;
    [[nodiscard]] std::shared_ptr<EWM> ewm(double span) const;
    
    // Time series operations
    [[nodiscard]] std::shared_ptr<DataFrame> resample(const std::string& rule) const;
    [[nodiscard]] std::shared_ptr<DataFrame> asfreq(const std::string& freq) const;
    [[nodiscard]] int64_t first_valid_index() const;
    [[nodiscard]] int64_t last_valid_index() const;
    
    // Utility methods
    [[nodiscard]] bool equals(const std::shared_ptr<DataFrame>& other) const;
    [[nodiscard]] bool empty() const;
    [[nodiscard]] std::pair<int64_t, int64_t> shape() const;
    [[nodiscard]] int64_t size() const;
    [[nodiscard]] std::shared_ptr<DataFrame> assign(const std::unordered_map<std::string, Thread>& kwargs) const;
    [[nodiscard]] std::shared_ptr<DataFrame> insert(int64_t loc, const std::string& column, const Thread& value) const;
    [[nodiscard]] Thread pop(const std::string& item);
    
    // I/O operations
    [[nodiscard]] std::string to_json(const std::string& orient = "columns") const;
    [[nodiscard]] std::string to_markdown() const;
    [[nodiscard]] std::unordered_map<std::string, std::vector<double>> to_dict(const std::string& orient = "dict") const;
    void to_parquet(const std::string& path) const;
    void to_excel(const std::string& path, const std::string& sheet_name = "Sheet1") const;
    static std::shared_ptr<DataFrame> from_dict(const std::unordered_map<std::string, std::vector<double>>& data);
    static std::shared_ptr<DataFrame> from_records(const std::vector<std::unordered_map<std::string, double>>& records);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // Format-specific helpers (not part of the public API)
    static std::shared_ptr<DataFrame> read_csv(const std::string& filename);

    [[nodiscard]] std::shared_ptr<DataFrame> select_many(const std::vector<std::string>& column_names) const;
    [[nodiscard]] pluss::table::Thread select_one(const std::string& col_name) const;
    [[nodiscard]] std::shared_ptr<DataFrame> set_column(const std::string& col_name, const pluss::table::Thread& values) const;
    [[nodiscard]] std::shared_ptr<DataFrame> set_columns(const std::vector<std::string>& column_names, const std::vector<pluss::table::Thread>& values) const;
};

} // namespace pluss::table





