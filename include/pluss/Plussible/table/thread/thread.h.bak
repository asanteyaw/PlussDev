#pragma once

#include <variant>
#include <string>
#include <pluss/Note.h>
#include <pluss/Timeline.h>
#include <pluss/Series.h>

namespace pluss::table {

// A Thread represents a DataFrame column. It holds the column name and a variant
// payload which may be a `pluss::Series`, `pluss::Note`, or `pluss::Timeline`.
class Thread {
public:
    using Value = std::variant<pluss::Series, pluss::Note, pluss::Timeline>;

    Thread() = default;
    explicit Thread(Value v, std::string name = "") : value_(std::move(v)), name_(std::move(name)) {}
    Thread(const pluss::Series& s, std::string name = "") : value_(s), name_(std::move(name)) {}
    Thread(pluss::Series&& s, std::string name = "") : value_(std::move(s)), name_(std::move(name)) {}
    Thread(const pluss::Note& n, std::string name = "") : value_(n), name_(std::move(name)) {}
    Thread(pluss::Note&& n, std::string name = "") : value_(std::move(n)), name_(std::move(name)) {}
    Thread(const pluss::Timeline& t, std::string name = "") : value_(t), name_(std::move(name)) {}
    Thread(pluss::Timeline&& t, std::string name = "") : value_(std::move(t)), name_(std::move(name)) {}

    Thread(const Thread&) = default;
    Thread(Thread&&) noexcept = default;
    Thread& operator=(const Thread&) = default;
    Thread& operator=(Thread&&) noexcept = default;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    void set_name(const std::string& n) { name_ = n; }

    // Unified access to underlying content (Series, Note, or Timeline)
    [[nodiscard]] const Value& values() const noexcept { return value_; }
    [[nodiscard]] Value& values() noexcept { return value_; }

    [[nodiscard]] bool is_series() const noexcept { return std::holds_alternative<pluss::Series>(value_); }
    [[nodiscard]] bool is_note() const noexcept { return std::holds_alternative<pluss::Note>(value_); }
    [[nodiscard]] bool is_timeline() const noexcept { return std::holds_alternative<pluss::Timeline>(value_); }

    [[nodiscard]] const pluss::Series& as_series() const { return std::get<pluss::Series>(value_); }
    [[nodiscard]] pluss::Series& as_series() { return std::get<pluss::Series>(value_); }

    [[nodiscard]] const pluss::Note& as_note() const { return std::get<pluss::Note>(value_); }
    [[nodiscard]] pluss::Note& as_note() { return std::get<pluss::Note>(value_); }

    [[nodiscard]] const pluss::Timeline& as_timeline() const { return std::get<pluss::Timeline>(value_); }
    [[nodiscard]] pluss::Timeline& as_timeline() { return std::get<pluss::Timeline>(value_); }

    // Attempt to convert the stored value to a Series. Timelines can be converted
    // via `to_Series()`. Notes are not generally convertible to numeric Series and
    // will throw.
    pluss::Series to_series() const {
        if (is_series()) return as_series();
        if (is_timeline()) return as_timeline().to_Series();
        throw std::runtime_error("Thread: cannot convert Note to Series");
    }

    // Size/shape helpers
    int64_t size() const {
        if (is_series()) return as_series().numel();
        if (is_timeline()) {
            const auto& tl = as_timeline();
            if (tl.has_date()) return tl.date_to_Series().numel();
            if (tl.has_time()) return tl.time_to_Series().numel();
            return 0;
        }
        // For Note, interpret the first shape dimension as size (vector-like)
        if (is_note()) {
            auto sh = as_note().shape();
            return sh.empty() ? 1 : sh[0];
        }
        return 0;
    }

    int64_t dim() const {
        if (is_series()) return as_series().dim();
        if (is_timeline()) {
            const auto& tl = as_timeline();
            if (!tl.has_date() && !tl.has_time()) return 0;
            // DataFrame columns are 1D, even if a Timeline stores date+time internally.
            return 1;
        }
        if (is_note()) return as_note().ndim();
        return 0;
    }

    // Get string representation of cell at given index
    std::string cell_value(int64_t idx) const {
        if (is_series()) {
            try {
                const auto& s = as_series();
                if (idx < 0 || idx >= s.numel()) return std::string();
                auto t = at::Series(s.data());
                if (t.dim() > 1) t = t.view({-1});
                const auto val = t[idx];
                if (torch::isnan(val).item<bool>()) return "NaN";
                if (torch::isinf(val).item<bool>()) {
                    return val.item<double>() > 0 ? "inf" : "-inf";
                }
                if (t.is_floating_point()) {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(4) << val.item<double>();
                    return oss.str();
                }
                std::ostringstream oss;
                oss << val.item<int64_t>();
                return oss.str();
            } catch (...) {
                return std::string();
            }
        }
        if (is_timeline()) {
            try {
                const auto& tl = as_timeline();
                auto ds_t = at::Series(static_cast<const at::TensorBase&>(tl.date_to_Series()));
                auto ts_t = at::Series(static_cast<const at::TensorBase&>(tl.time_to_Series()));
                const bool ds_scalar = (ds_t.numel() == 1);
                const bool ts_scalar = (ts_t.numel() == 1);
                if (ds_scalar && ts_scalar) return tl.to_string();

                int64_t d = 0;
                int64_t tm = 0;
                if (ds_t.numel() > 0) d = ds_t.view({-1})[idx].item<int64_t>();
                if (ts_t.numel() > 0) tm = ts_t.view({-1})[idx].item<int64_t>();
                if (d == 0 && tm == 0) return std::string();

                std::ostringstream oss;
                if (d != 0) {
                    int y = static_cast<int>(d / 10000);
                    int mo = static_cast<int>((d / 100) % 100);
                    int da = static_cast<int>(d % 100);
                    oss << std::setw(4) << std::setfill('0') << y << "-" << std::setw(2) << mo << "-" << std::setw(2) << da;
                }
                if (tm != 0) {
                    int h = static_cast<int>(tm / 10000);
                    int mi = static_cast<int>((tm / 100) % 100);
                    int se = static_cast<int>(tm % 100);
                    if (d != 0) oss << " ";
                    oss << std::setw(2) << std::setfill('0') << h << ":" << std::setw(2) << mi << ":" << std::setw(2) << se;
                }
                return oss.str();
            } catch (...) {
                return std::string();
            }
        }
        if (is_note()) {
            try {
                auto v = as_note().vector();
                if (idx >= 0 && static_cast<size_t>(idx) < v.size()) {
                    return v[static_cast<size_t>(idx)].str();
                }
            } catch (...) {
            }
        }
        return std::string();
    }

    // ========== THREADABLE OPERATIONS (Thread-First Design) ==========
    // These operations work on individual threads and can be efficiently
    // delegated by DataFrame for vectorized/parallel execution

    // Indexing and selection
    [[nodiscard]] Thread head(int64_t n) const;
    [[nodiscard]] Thread tail(int64_t n) const;
    [[nodiscard]] Thread iloc(int64_t start, int64_t end) const;
    [[nodiscard]] Thread loc(const pluss::Series& condition) const;
    [[nodiscard]] Thread take(const std::vector<int64_t>& indices) const;
    [[nodiscard]] Thread where(const pluss::Series& condition, const Thread& other) const;
    [[nodiscard]] Thread mask(const pluss::Series& condition, const Thread& other) const;
    
    // Basic transformations
    [[nodiscard]] Thread to(const torch::Device& device) const;
    [[nodiscard]] Thread unique() const;
    [[nodiscard]] Thread sort_values(bool ascending = true) const;
    [[nodiscard]] Thread clip(double lower, double upper) const;
    [[nodiscard]] Thread abs() const;
    [[nodiscard]] Thread round(int decimals = 0) const;
    [[nodiscard]] Thread replace(double old_value, double new_value) const;
    
    // Missing data handling
    [[nodiscard]] Thread fillna(const Thread& value) const;
    [[nodiscard]] Thread dropna() const;
    [[nodiscard]] Thread ffill() const;  // Forward fill
    [[nodiscard]] Thread bfill() const;  // Backward fill
    [[nodiscard]] Thread interpolate(const std::string& method = "linear") const;
    [[nodiscard]] pluss::Series isna() const;
    [[nodiscard]] pluss::Series notna() const;
    
    // Statistical and aggregation
    [[nodiscard]] Thread value_counts() const;
    [[nodiscard]] Thread rank(const std::string& method = "average") const;
    [[nodiscard]] Thread pct_change(int64_t periods = 1) const;
    [[nodiscard]] Thread nlargest(int64_t n) const;
    [[nodiscard]] Thread nsmallest(int64_t n) const;
    
    // Cumulative operations
    [[nodiscard]] Thread diff(int64_t periods = 1) const;
    [[nodiscard]] Thread shift(int64_t periods = 1) const;
    [[nodiscard]] Thread cumsum() const;
    [[nodiscard]] Thread cumprod() const;
    [[nodiscard]] Thread cummax() const;
    [[nodiscard]] Thread cummin() const;
    
    // Comparison operations
    [[nodiscard]] pluss::Series eq(const Thread& other) const;
    [[nodiscard]] pluss::Series ne(const Thread& other) const;
    [[nodiscard]] pluss::Series lt(const Thread& other) const;
    [[nodiscard]] pluss::Series le(const Thread& other) const;
    [[nodiscard]] pluss::Series gt(const Thread& other) const;
    [[nodiscard]] pluss::Series ge(const Thread& other) const;
    
    // Arithmetic operations
    [[nodiscard]] Thread add(const Thread& other) const;
    [[nodiscard]] Thread sub(const Thread& other) const;
    [[nodiscard]] Thread mul(const Thread& other) const;
    [[nodiscard]] Thread div(const Thread& other) const;
    [[nodiscard]] Thread mod(const Thread& other) const;
    [[nodiscard]] Thread pow(const Thread& other) const;
    
    // Mathematical functions
    [[nodiscard]] Thread sqrt() const;
    [[nodiscard]] Thread log() const;
    [[nodiscard]] Thread log10() const;
    [[nodiscard]] Thread exp() const;
    [[nodiscard]] Thread sin() const;
    [[nodiscard]] Thread cos() const;
    [[nodiscard]] Thread tan() const;
    
    // Aggregation operations (return scalar or single-element Thread)
    [[nodiscard]] double sum() const;
    [[nodiscard]] double mean() const;
    [[nodiscard]] double median() const;
    [[nodiscard]] double std(int ddof = 1) const;
    [[nodiscard]] double var(int ddof = 1) const;
    [[nodiscard]] double min() const;
    [[nodiscard]] double max() const;
    [[nodiscard]] double prod() const;
    [[nodiscard]] double sem() const;  // Standard error of mean
    [[nodiscard]] double mad() const;  // Mean absolute deviation
    [[nodiscard]] double skew() const;
    [[nodiscard]] double kurt() const;  // Kurtosis
    [[nodiscard]] double quantile(double q) const;
    [[nodiscard]] int64_t count() const;
    [[nodiscard]] int64_t nunique() const;
    
    // Index location methods
    [[nodiscard]] int64_t idxmax() const;
    [[nodiscard]] int64_t idxmin() const;
    [[nodiscard]] Thread mode() const;
    
    // Boolean reductions
    [[nodiscard]] bool all() const;
    [[nodiscard]] bool any() const;
    
    // Selection and filtering
    [[nodiscard]] Thread sample(int64_t n, bool replace = false) const;
    [[nodiscard]] Thread truncate(int64_t before, int64_t after) const;
    [[nodiscard]] Thread drop_duplicates() const;
    [[nodiscard]] pluss::Series duplicated() const;
    [[nodiscard]] pluss::Series isin(const std::vector<double>& values) const;
    [[nodiscard]] pluss::Series isin(const Thread& values) const;
    
    // Transformations
    [[nodiscard]] Thread transform(const std::string& func) const;
    [[nodiscard]] Thread apply(std::function<double(double)> func) const;

private:
    Value value_;
    std::string name_;
};

} // namespace pluss::table
// This is equivalent to an Archive on the entry side. It is a table of tables(DataFrames), thus we say a Store of DataFrames!
// The method that converts an Archive to a DataStore is storelize(). This is a 3D table.