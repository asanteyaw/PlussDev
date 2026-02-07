#pragma once

#include <variant>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <pluss/Note.h>
#include <pluss/Timeline.h>
#include <pluss/Series.h>

namespace pluss::handle {

/**
 * @brief Feed - Homogeneous dictionary field (analogous to Thread in table module)
 * 
 * Handle Design Philosophy (Dictionary-esque, Homogeneous):
 * - Feed (1D): Homogeneous field in dictionary context
 * - Catalog (2D): Collection of Feeds (dictionary-based DataFrame)
 * - Archive (3D): Collection of Catalogs (dictionary-based Atlas)
 * 
 * Feed is to Catalog what Thread is to DataFrame.
 * 
 * A Feed represents a homogeneous field that can hold:
 * - pluss::Series: Numerical tensor arrays
 * - pluss::Note: Text tensor arrays  
 * - pluss::Timeline: Date/time sequences
 * 
 * All elements in a Feed have the same type, enabling vectorized operations.
 * For heterogeneous data with varying types per field, use Register module.
 */
class Feed {
public:
    using Value = std::variant<pluss::Series, pluss::Note, pluss::Timeline>;

    Feed() = default;
    explicit Feed(Value v, std::string name = "") : value_(std::move(v)), name_(std::move(name)) {}
    Feed(const pluss::Series& s, std::string name = "") : value_(s), name_(std::move(name)) {}
    Feed(pluss::Series&& s, std::string name = "") : value_(std::move(s)), name_(std::move(name)) {}
    Feed(const pluss::Note& n, std::string name = "") : value_(n), name_(std::move(name)) {}
    Feed(pluss::Note&& n, std::string name = "") : value_(std::move(n)), name_(std::move(name)) {}
    Feed(const pluss::Timeline& t, std::string name = "") : value_(t), name_(std::move(name)) {}
    Feed(pluss::Timeline&& t, std::string name = "") : value_(std::move(t)), name_(std::move(name)) {}

    Feed(const Feed&) = default;
    Feed(Feed&&) noexcept = default;
    Feed& operator=(const Feed&) = default;
    Feed& operator=(Feed&&) noexcept = default;

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

    // Convert to Series (Timeline supported, Note throws)
    pluss::Series to_series() const {
        if (is_series()) return as_series();
        if (is_timeline()) return as_timeline().to_Series();
        throw std::runtime_error("Feed: cannot convert Note to Series");
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
            if (tl.has_date()) return tl.date_to_Series().dim();
            if (tl.has_time()) return tl.time_to_Series().dim();
            return 0;
        }
        if (is_note()) return static_cast<int64_t>(as_note().shape().size());
        return 0;
    }

    // Vectorized operations (mirror Thread operations from table module)
    
    // Indexing and slicing
    Feed head(int64_t n) const;
    Feed tail(int64_t n) const;
    Feed slice(int64_t start, int64_t end) const;
    Feed at(int64_t index) const;
    Feed masked_select(const pluss::Series& mask) const;
    Feed take(const pluss::Series& indices) const;
    
    // Filtering
    Feed filter(const pluss::Series& condition) const;
    Feed drop_duplicates() const;
    Feed unique() const;
    
    // Sorting
    Feed sort(bool ascending = true) const;
    Feed argsort(bool ascending = true) const;
    
    // Transformations
    Feed apply(std::function<Feed(const Feed&)> func) const;
    Feed map(std::function<pluss::Series(const pluss::Series&)> func) const;
    
    // String operations (Note only)
    Feed str_upper() const;
    Feed str_lower() const;
    Feed str_capitalize() const;
    Feed str_trim() const;
    Feed str_replace(const std::string& pattern, const std::string& replacement) const;
    Feed str_contains(const std::string& pattern) const;
    Feed str_startswith(const std::string& prefix) const;
    Feed str_endswith(const std::string& suffix) const;
    Feed str_length() const;
    Feed str_split(const std::string& delimiter) const;
    
    // NLP/Text Processing (Note only)
    Feed tokenize(const std::string& method = "whitespace") const;
    Feed remove_punctuation() const;
    Feed remove_stopwords(const std::vector<std::string>& stopwords = {}) const;
    Feed remove_digits() const;
    Feed stem(const std::string& algorithm = "porter") const;
    Feed lemmatize() const;
    Feed ngrams(int n = 2) const;
    Feed bag_of_words() const;
    Feed extract_keywords(int top_n = 10) const;
    Feed extract_entities(const std::string& entity_type = "all") const;
    
    // Text similarity and distance
    double cosine_similarity(const Feed& other) const;
    double jaccard_similarity(const Feed& other) const;
    double levenshtein_distance(const Feed& other) const;
    Feed pairwise_similarity(const Feed& other, const std::string& method = "cosine") const;
    
    // Text statistics
    Feed word_count() const;
    Feed unique_word_count() const;
    Feed char_count() const;
    Feed sentence_count() const;
    Feed avg_word_length() const;
    
    // Numeric operations (Series only)
    Feed abs() const;
    Feed sqrt() const;
    Feed pow(double exponent) const;
    Feed log() const;
    Feed exp() const;
    Feed floor() const;
    Feed ceil() const;
    Feed round(int decimals = 0) const;
    Feed clip(double min_val, double max_val) const;
    Feed normalize() const;
    Feed standardize() const;
    
    // Missing value handling
    Feed fill_na(double value) const;
    Feed drop_na() const;
    Feed ffill() const;
    Feed bfill() const;
    bool has_na() const;
    int64_t count_na() const;
    
    // Statistical operations
    double sum() const;
    double mean() const;
    double median() const;
    double std() const;
    double var() const;
    double min() const;
    double max() const;
    int64_t count() const;
    double quantile(double q) const;
    
    // Cumulative operations
    Feed cumsum() const;
    Feed cumprod() const;
    Feed cummax() const;
    Feed cummin() const;
    
    // Rolling window operations
    Feed rolling_mean(int64_t window) const;
    Feed rolling_sum(int64_t window) const;
    Feed rolling_std(int64_t window) const;
    Feed rolling_min(int64_t window) const;
    Feed rolling_max(int64_t window) const;
    
    // Shift and diff
    Feed shift(int64_t periods) const;
    Feed diff(int64_t periods = 1) const;
    Feed pct_change(int64_t periods = 1) const;
    
    // Comparison operations
    pluss::Series eq(const Feed& other) const;
    pluss::Series ne(const Feed& other) const;
    pluss::Series lt(const Feed& other) const;
    pluss::Series le(const Feed& other) const;
    pluss::Series gt(const Feed& other) const;
    pluss::Series ge(const Feed& other) const;
    pluss::Series isin(const Feed& values) const;
    pluss::Series between(double lower, double upper) const;
    
    // Type conversion
    Feed to_numeric() const;
    Feed as_text() const;
    Feed to_datetime() const;
    
    // Reshaping
    Feed flatten() const;
    Feed reshape(const std::vector<int64_t>& shape) const;
    
    // Sampling and randomization
    Feed sample(int64_t n, bool replacement = false) const;
    Feed shuffle() const;
    
    // Value counts and binning
    std::unordered_map<std::string, int64_t> value_counts() const;
    Feed bin(int64_t n_bins) const;
    Feed cut(const std::vector<double>& edges) const;
    
    // Display and conversion
    void display(int64_t max_rows = 10) const;
    void info() const;
    std::string dtype() const;
    std::string to_string() const;
    std::vector<double> to_vector_double() const;
    std::vector<std::string> to_vector_string() const;
    
    // Serialization
    std::string to_json() const;
    static Feed from_json(const std::string& json, const std::string& name = "");

private:
    Value value_;
    std::string name_;
};

}  // namespace pluss::handle
