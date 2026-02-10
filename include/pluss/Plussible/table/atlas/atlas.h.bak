#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <pluss/datalog/table/dataframe/dataframe.h>

namespace pluss::table {

/**
 * @brief Atlas - 3D data structure (collection of DataFrames)
 * 
 * Atlas extends the Thread-First design pattern:
 * - Thread (1D): Single column operations
 * - DataFrame (2D): Multiple columns with shared index
 * - Atlas (3D): Multiple DataFrames with shared structure
 * 
 * An Atlas is a collection of DataFrames along a depth dimension.
 * All operations are vectorized for 3D efficiency and delegate to DataFrame operations.
 */
class Atlas {
public:
    // =============== Constructors ===============
    Atlas() = default;
    explicit Atlas(const std::vector<std::shared_ptr<DataFrame>>& frames,
                   const std::vector<std::string>& depth_names = {},
                   bool validate_consistency = false);
    explicit Atlas(const std::unordered_map<std::string, std::shared_ptr<DataFrame>>& frames,
                   bool validate_consistency = false);
    
    // Copy and move semantics
    Atlas(const Atlas& other);
    Atlas(Atlas&& other) noexcept = default;
    Atlas& operator=(const Atlas& other);
    Atlas& operator=(Atlas&& other) noexcept = default;
    ~Atlas() = default;
    
    // =============== Access and Inspection ===============
    
    // Get DataFrame at specific depth
    [[nodiscard]] std::shared_ptr<DataFrame> at(int64_t depth) const;
    [[nodiscard]] std::shared_ptr<DataFrame> at(const std::string& depth_name) const;
    [[nodiscard]] std::shared_ptr<DataFrame> operator[](int64_t depth) const;
    [[nodiscard]] std::shared_ptr<DataFrame> operator[](const std::string& depth_name) const;
    
    // Shape and size information
    [[nodiscard]] int64_t ndepth() const noexcept;  // Number of DataFrames (depth dimension)
    
    // Legacy shape methods - return first DataFrame's dimensions for backward compatibility
    [[nodiscard]] int64_t nrows() const noexcept;    // Rows of first DataFrame (for homogeneous Atlas)
    [[nodiscard]] int64_t ncols() const noexcept;    // Columns of first DataFrame (for homogeneous Atlas)
    [[nodiscard]] std::tuple<int64_t, int64_t, int64_t> shape() const noexcept;  // (depth, rows[0], cols[0])
    
    // Per-DataFrame shape queries - for heterogeneous Atlas
    [[nodiscard]] int64_t nrows(int64_t depth) const;  // Rows at specific depth
    [[nodiscard]] int64_t ncols(int64_t depth) const;  // Columns at specific depth
    [[nodiscard]] std::pair<int64_t, int64_t> shape(int64_t depth) const;  // (rows, cols) at depth
    [[nodiscard]] std::vector<std::pair<int64_t, int64_t>> all_shapes() const;  // All DataFrame shapes
    
    // Aggregate dimensions - for heterogeneous Atlas
    [[nodiscard]] int64_t max_rows() const noexcept;  // Maximum rows across all DataFrames
    [[nodiscard]] int64_t min_rows() const noexcept;  // Minimum rows across all DataFrames
    [[nodiscard]] int64_t max_cols() const noexcept;  // Maximum columns across all DataFrames
    [[nodiscard]] int64_t min_cols() const noexcept;  // Minimum columns across all DataFrames
    [[nodiscard]] int64_t total_elements() const noexcept;  // Sum of all elements across all DataFrames
    
    [[nodiscard]] int64_t size() const noexcept;     // Total elements (sum for heterogeneous)
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] bool is_homogeneous() const noexcept;  // Check if all DataFrames have same shape
    
    // Get depth names
    [[nodiscard]] const std::vector<std::string>& depth_names() const noexcept;
    
    // Column name queries - for heterogeneous Atlas
    [[nodiscard]] std::vector<std::string> column_names() const;  // From first DataFrame
    [[nodiscard]] std::vector<std::string> column_names(int64_t depth) const;  // At specific depth
    [[nodiscard]] std::vector<std::string> all_column_names() const;  // Union of all columns across depths
    [[nodiscard]] std::vector<std::string> common_column_names() const;  // Intersection of columns across depths
    
    // =============== Indexing and Selection ===============
    
    // Select specific depths
    [[nodiscard]] std::shared_ptr<Atlas> select_depths(const std::vector<int64_t>& depths) const;
    [[nodiscard]] std::shared_ptr<Atlas> select_depths(const std::vector<std::string>& depth_names) const;
    [[nodiscard]] std::shared_ptr<Atlas> iloc(int64_t start, int64_t end) const;  // Depth slice
    [[nodiscard]] std::shared_ptr<Atlas> loc(const std::vector<std::string>& depth_names) const;
    
    // Select columns across all depths
    // For heterogeneous Atlas: missing columns are handled according to fill_strategy
    enum class MissingColumnStrategy {
        Skip,      // Skip DataFrames that don't have the column
        FillNaN,   // Add missing columns filled with NaN
        Error      // Throw error if any DataFrame is missing the column
    };
    [[nodiscard]] std::shared_ptr<Atlas> select_columns(
        const std::vector<std::string>& columns,
        MissingColumnStrategy strategy = MissingColumnStrategy::FillNaN) const;
    
    // Head/tail operations (along depth dimension)
    [[nodiscard]] std::shared_ptr<Atlas> head(int64_t n = 5) const;
    [[nodiscard]] std::shared_ptr<Atlas> tail(int64_t n = 5) const;
    [[nodiscard]] std::shared_ptr<Atlas> sample(int64_t n, bool replace = false, int64_t random_state = -1) const;
    [[nodiscard]] std::shared_ptr<Atlas> take(const std::vector<int64_t>& indices) const;
    [[nodiscard]] std::shared_ptr<Atlas> truncate(int64_t before = -1, int64_t after = -1) const;
    
    // =============== Sorting and Ordering ===============
    
    // Sort along depth dimension by aggregated values
    [[nodiscard]] std::shared_ptr<Atlas> sort_values(
        const std::string& by,
        bool ascending = true,
        const std::string& agg = "mean") const;
    [[nodiscard]] std::shared_ptr<Atlas> sort_values(
        const std::vector<std::string>& by,
        bool ascending = true,
        const std::string& agg = "mean") const;
    [[nodiscard]] std::shared_ptr<Atlas> sort_index(bool ascending = true) const;
    [[nodiscard]] std::shared_ptr<Atlas> reindex(const std::vector<std::string>& new_depth_names) const;
    [[nodiscard]] std::shared_ptr<Atlas> reindex_like(const std::shared_ptr<Atlas>& other) const;
    [[nodiscard]] std::shared_ptr<Atlas> set_depth_names(const std::vector<std::string>& names) const;
    [[nodiscard]] std::shared_ptr<Atlas> reset_depth_names() const;
    
    // =============== Filtering and Selection ===============
    
    [[nodiscard]] std::shared_ptr<Atlas> drop(const std::vector<std::string>& columns) const;
    [[nodiscard]] std::shared_ptr<Atlas> drop_depths(const std::vector<int64_t>& depths) const;
    [[nodiscard]] std::shared_ptr<Atlas> drop_depths(const std::vector<std::string>& depth_names) const;
    [[nodiscard]] std::shared_ptr<Atlas> drop_duplicates(const std::vector<std::string>& subset = {}) const;
    [[nodiscard]] std::shared_ptr<Atlas> filter(const std::vector<std::string>& items) const;
    
    // =============== Missing Data Handling ===============
    
    // Apply to all DataFrames in parallel
    [[nodiscard]] std::shared_ptr<Atlas> dropna(const std::string& how = "any", int64_t thresh = -1) const;
    [[nodiscard]] std::shared_ptr<Atlas> fillna(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> fillna(const std::unordered_map<std::string, double>& values) const;
    [[nodiscard]] std::shared_ptr<Atlas> ffill() const;
    [[nodiscard]] std::shared_ptr<Atlas> bfill() const;
    [[nodiscard]] std::shared_ptr<Atlas> interpolate(const std::string& method = "linear") const;
    [[nodiscard]] std::shared_ptr<Atlas> isna() const;
    [[nodiscard]] std::shared_ptr<Atlas> notna() const;
    
    // =============== Transformations ===============
    
    // Apply operation to each DataFrame
    [[nodiscard]] std::shared_ptr<Atlas> apply(
        std::function<std::shared_ptr<DataFrame>(const std::shared_ptr<DataFrame>&)> func) const;
    [[nodiscard]] std::shared_ptr<Atlas> transform(const std::string& func) const;
    
    // Element-wise transformations (delegate to DataFrame)
    [[nodiscard]] std::shared_ptr<Atlas> abs() const;
    [[nodiscard]] std::shared_ptr<Atlas> round(int decimals = 0) const;
    [[nodiscard]] std::shared_ptr<Atlas> clip(double lower, double upper) const;
    [[nodiscard]] std::shared_ptr<Atlas> diff(int64_t periods = 1) const;
    [[nodiscard]] std::shared_ptr<Atlas> shift(int64_t periods = 1) const;
    [[nodiscard]] std::shared_ptr<Atlas> pct_change(int64_t periods = 1) const;
    
    // Mathematical functions (vectorized across all depths)
    [[nodiscard]] std::shared_ptr<Atlas> sqrt() const;
    [[nodiscard]] std::shared_ptr<Atlas> log() const;
    [[nodiscard]] std::shared_ptr<Atlas> log10() const;
    [[nodiscard]] std::shared_ptr<Atlas> exp() const;
    [[nodiscard]] std::shared_ptr<Atlas> sin() const;
    [[nodiscard]] std::shared_ptr<Atlas> cos() const;
    [[nodiscard]] std::shared_ptr<Atlas> tan() const;
    
    // =============== Cumulative Operations ===============
    
    // Along depth dimension
    [[nodiscard]] std::shared_ptr<Atlas> cumsum_depth() const;
    [[nodiscard]] std::shared_ptr<Atlas> cumprod_depth() const;
    [[nodiscard]] std::shared_ptr<Atlas> cummax_depth() const;
    [[nodiscard]] std::shared_ptr<Atlas> cummin_depth() const;
    
    // Within each DataFrame
    [[nodiscard]] std::shared_ptr<Atlas> cumsum() const;
    [[nodiscard]] std::shared_ptr<Atlas> cumprod() const;
    [[nodiscard]] std::shared_ptr<Atlas> cummax() const;
    [[nodiscard]] std::shared_ptr<Atlas> cummin() const;
    
    // =============== Aggregations ===============
    
    // Aggregate across depth dimension (returns DataFrame)
    [[nodiscard]] std::shared_ptr<DataFrame> mean_depth() const;
    [[nodiscard]] std::shared_ptr<DataFrame> sum_depth() const;
    [[nodiscard]] std::shared_ptr<DataFrame> median_depth() const;
    [[nodiscard]] std::shared_ptr<DataFrame> std_depth(int ddof = 1) const;
    [[nodiscard]] std::shared_ptr<DataFrame> var_depth(int ddof = 1) const;
    [[nodiscard]] std::shared_ptr<DataFrame> min_depth() const;
    [[nodiscard]] std::shared_ptr<DataFrame> max_depth() const;
    [[nodiscard]] std::shared_ptr<DataFrame> prod_depth() const;
    [[nodiscard]] std::shared_ptr<DataFrame> quantile_depth(double q) const;
    
    // Within each DataFrame (returns Atlas of aggregated DataFrames)
    [[nodiscard]] std::shared_ptr<Atlas> mean() const;
    [[nodiscard]] std::shared_ptr<Atlas> sum() const;
    [[nodiscard]] std::shared_ptr<Atlas> median() const;
    [[nodiscard]] std::shared_ptr<Atlas> std(int ddof = 1) const;
    [[nodiscard]] std::shared_ptr<Atlas> var(int ddof = 1) const;
    [[nodiscard]] std::shared_ptr<Atlas> min() const;
    [[nodiscard]] std::shared_ptr<Atlas> max() const;
    [[nodiscard]] std::shared_ptr<Atlas> prod() const;
    [[nodiscard]] std::shared_ptr<Atlas> count() const;
    [[nodiscard]] std::shared_ptr<Atlas> nunique() const;
    [[nodiscard]] std::shared_ptr<Atlas> quantile(double q) const;
    
    // =============== Statistical Operations ===============
    
    // Across depth dimension
    [[nodiscard]] std::shared_ptr<DataFrame> describe_depth(
        const std::vector<double>& percentiles = {0.25, 0.5, 0.75}) const;
    
    // Within each DataFrame
    [[nodiscard]] std::shared_ptr<Atlas> describe(
        const std::vector<double>& percentiles = {0.25, 0.5, 0.75}) const;
    [[nodiscard]] std::shared_ptr<Atlas> corr(const std::string& method = "pearson") const;
    [[nodiscard]] std::shared_ptr<Atlas> cov(int ddof = 1) const;
    [[nodiscard]] std::shared_ptr<Atlas> rank(const std::string& method = "average") const;
    
    // =============== Comparison Operations ===============
    
    // Element-wise comparison between two Atlases
    [[nodiscard]] std::shared_ptr<Atlas> eq(const std::shared_ptr<Atlas>& other) const;
    [[nodiscard]] std::shared_ptr<Atlas> ne(const std::shared_ptr<Atlas>& other) const;
    [[nodiscard]] std::shared_ptr<Atlas> lt(const std::shared_ptr<Atlas>& other) const;
    [[nodiscard]] std::shared_ptr<Atlas> le(const std::shared_ptr<Atlas>& other) const;
    [[nodiscard]] std::shared_ptr<Atlas> gt(const std::shared_ptr<Atlas>& other) const;
    [[nodiscard]] std::shared_ptr<Atlas> ge(const std::shared_ptr<Atlas>& other) const;
    
    // Compare with scalar
    [[nodiscard]] std::shared_ptr<Atlas> eq(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> ne(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> lt(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> le(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> gt(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> ge(double value) const;
    
    // =============== Arithmetic Operations ===============
    
    // Element-wise arithmetic between Atlases (matching depths)
    [[nodiscard]] std::shared_ptr<Atlas> add(const std::shared_ptr<Atlas>& other, double fill_value = NAN) const;
    [[nodiscard]] std::shared_ptr<Atlas> sub(const std::shared_ptr<Atlas>& other, double fill_value = NAN) const;
    [[nodiscard]] std::shared_ptr<Atlas> mul(const std::shared_ptr<Atlas>& other, double fill_value = NAN) const;
    [[nodiscard]] std::shared_ptr<Atlas> div(const std::shared_ptr<Atlas>& other, double fill_value = NAN) const;
    [[nodiscard]] std::shared_ptr<Atlas> mod(const std::shared_ptr<Atlas>& other, double fill_value = NAN) const;
    [[nodiscard]] std::shared_ptr<Atlas> pow(const std::shared_ptr<Atlas>& other, double fill_value = NAN) const;
    
    // Scalar arithmetic
    [[nodiscard]] std::shared_ptr<Atlas> add(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> sub(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> mul(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> div(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> mod(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> pow(double value) const;
    
    // Operator overloads
    [[nodiscard]] std::shared_ptr<Atlas> operator+(const std::shared_ptr<Atlas>& other) const;
    [[nodiscard]] std::shared_ptr<Atlas> operator-(const std::shared_ptr<Atlas>& other) const;
    [[nodiscard]] std::shared_ptr<Atlas> operator*(const std::shared_ptr<Atlas>& other) const;
    [[nodiscard]] std::shared_ptr<Atlas> operator/(const std::shared_ptr<Atlas>& other) const;
    
    [[nodiscard]] std::shared_ptr<Atlas> operator+(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> operator-(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> operator*(double value) const;
    [[nodiscard]] std::shared_ptr<Atlas> operator/(double value) const;
    
    // =============== Renaming ===============
    
    [[nodiscard]] std::shared_ptr<Atlas> rename_columns(
        const std::unordered_map<std::string, std::string>& columns) const;
    [[nodiscard]] std::shared_ptr<Atlas> rename_depths(
        const std::unordered_map<std::string, std::string>& depths) const;
    [[nodiscard]] std::shared_ptr<Atlas> add_prefix(const std::string& prefix) const;
    [[nodiscard]] std::shared_ptr<Atlas> add_suffix(const std::string& suffix) const;
    
    // =============== Combining and Merging ===============
    
    // Concatenate Atlases along depth dimension
    static std::shared_ptr<Atlas> concat(
        const std::vector<std::shared_ptr<Atlas>>& atlases,
        bool ignore_depth_names = false);
    
    // Append another Atlas
    [[nodiscard]] std::shared_ptr<Atlas> append(
        const std::shared_ptr<Atlas>& other,
        bool ignore_depth_names = false) const;
    
    // Add a single DataFrame as new depth
    [[nodiscard]] std::shared_ptr<Atlas> append_depth(
        const std::shared_ptr<DataFrame>& frame,
        const std::string& depth_name = "") const;
    
    // Merge operation (for each matching depth)
    [[nodiscard]] std::shared_ptr<Atlas> merge(
        const std::shared_ptr<Atlas>& other,
        const std::string& on,
        const std::string& how = "inner") const;
    
    // Join operation
    [[nodiscard]] std::shared_ptr<Atlas> join(
        const std::shared_ptr<Atlas>& other,
        const std::string& how = "left") const;
    
    // =============== Reshaping ===============
    
    // Stack all DataFrames into a single DataFrame with depth as column
    [[nodiscard]] std::shared_ptr<DataFrame> stack_depths() const;
    
    // Unstack a column to create depth dimension
    static std::shared_ptr<Atlas> unstack_column(
        const std::shared_ptr<DataFrame>& df,
        const std::string& depth_column);
    
    // Squeeze dimensions
    [[nodiscard]] std::shared_ptr<DataFrame> squeeze() const;  // If only one depth, return that DataFrame
    
    // =============== Boolean Reductions ===============
    
    [[nodiscard]] bool all() const;  // All values across all depths
    [[nodiscard]] bool any() const;  // Any value across all depths
    [[nodiscard]] std::shared_ptr<Atlas> all_within() const;  // All within each DataFrame
    [[nodiscard]] std::shared_ptr<Atlas> any_within() const;  // Any within each DataFrame
    [[nodiscard]] std::shared_ptr<DataFrame> all_depth() const;  // All across depths
    [[nodiscard]] std::shared_ptr<DataFrame> any_depth() const;  // Any across depths
    
    // =============== Utility Methods ===============
    
    [[nodiscard]] bool equals(const std::shared_ptr<Atlas>& other) const;
    [[nodiscard]] std::shared_ptr<Atlas> copy() const;
    [[nodiscard]] std::shared_ptr<Atlas> assign(
        const std::unordered_map<std::string, std::shared_ptr<DataFrame>>& kwargs) const;
    
    // Validate that all DataFrames have consistent structure
    [[nodiscard]] bool has_consistent_structure() const;
    void validate_structure() const;
    
    // =============== I/O Operations ===============
    
    // Convert to various formats
    [[nodiscard]] std::string to_json(const std::string& orient = "depths") const;
    [[nodiscard]] std::string to_string() const;
    void to_directory(const std::string& dir_path) const;  // Save each DataFrame to separate file
    
    // Load from directory (each file becomes a depth)
    static std::shared_ptr<Atlas> from_directory(
        const std::string& dir_path,
        const std::string& pattern = "*.csv");
    
    // Create from collection of DataFrames
    static std::shared_ptr<Atlas> from_frames(
        const std::vector<std::shared_ptr<DataFrame>>& frames,
        const std::vector<std::string>& depth_names = {});
    
    static std::shared_ptr<Atlas> from_frames(
        const std::unordered_map<std::string, std::shared_ptr<DataFrame>>& frames);

private:
    // Internal storage: ordered vector of DataFrames
    std::vector<std::shared_ptr<DataFrame>> frames_;
    
    // Depth names (optional, parallel to frames_)
    std::vector<std::string> depth_names_;
    
    // Quick lookup by depth name
    std::unordered_map<std::string, int64_t> depth_name_to_index_;
    
    // Helper methods
    void rebuild_depth_index();
    void validate_depth_bounds(int64_t depth) const;
    std::string generate_depth_name(int64_t index) const;
    
    // Vectorized operation helpers
    template<typename Func>
    std::shared_ptr<Atlas> apply_to_all_frames(Func&& func) const;
    
    template<typename Func>
    std::shared_ptr<DataFrame> reduce_across_depths(Func&& func) const;
};

} // namespace pluss::table