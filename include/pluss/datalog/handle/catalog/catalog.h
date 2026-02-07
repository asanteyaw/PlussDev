#pragma once

#include <pluss/datalog/handle/feed/feed.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace pluss::handle {

/**
 * @brief Catalog - Dictionary-based collection of homogeneous Feeds (analogous to DataFrame)
 * 
 * A Catalog is a 2D dictionary collection where:
 * - Keys are field names (strings)
 * - Values are Feeds (homogeneous 1D vectors)
 * - All Feeds in a Catalog must have the same length
 * 
 * Catalog is to Feed what DataFrame is to Thread.
 * 
 * Example:
 * ```cpp
 * Catalog cat("users");
 * cat["id"] = Feed(pluss::arange(1, 100), "id");
 * cat["name"] = Feed(pluss::make_note(names), "name");
 * cat["score"] = Feed(scores, "score");
 * 
 * auto filtered = cat.filter(cat["score"].gt(80.0));
 * cat.display();
 * ```
 */
class Catalog {
public:
    Catalog() = default;
    explicit Catalog(std::string name) : name_(std::move(name)) {}

    // Field access
    Feed& operator[](const std::string& key);
    const Feed& operator[](const std::string& key) const;
    Feed& at(const std::string& key);
    const Feed& at(const std::string& key) const;
    
    bool has(const std::string& key) const;
    void remove(const std::string& key);
    void clear();
    
    // Metadata
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    void set_name(const std::string& n) { name_ = n; }
    [[nodiscard]] int64_t size() const;  // Number of rows
    [[nodiscard]] int64_t num_fields() const { return static_cast<int64_t>(fields_.size()); }
    [[nodiscard]] std::vector<std::string> keys() const;
    [[nodiscard]] bool empty() const { return fields_.empty(); }
    
    // Indexing and slicing (row operations)
    Catalog head(int64_t n) const;
    Catalog tail(int64_t n) const;
    Catalog slice(int64_t start, int64_t end) const;
    Catalog filter(const pluss::Series& condition) const;
    Catalog masked_select(const pluss::Series& mask) const;
    Catalog iloc(int64_t start, int64_t end) const;
    
    // Field operations (column operations)
    Catalog select(const std::vector<std::string>& field_names) const;
    Catalog drop(const std::vector<std::string>& field_names) const;
    Catalog rename(const std::unordered_map<std::string, std::string>& mapping) const;
    
    // Sorting
    Catalog sort(const std::string& by_field, bool ascending = true) const;
    Catalog sort_by(const std::vector<std::string>& fields, const std::vector<bool>& ascending = {}) const;
    
    // Grouping (returns simplified aggregation - full groupby would return GroupBy object)
    std::unordered_map<std::string, Catalog> group_by(const std::string& key_field) const;
    
    // Aggregations
    Feed sum(const std::string& field) const;
    Feed mean(const std::string& field) const;
    Feed min(const std::string& field) const;
    Feed max(const std::string& field) const;
    int64_t count() const;
    
    // Joins
    Catalog merge(const Catalog& other, const std::string& on, const std::string& how = "inner") const;
    
    // Transformations
    Catalog apply(const std::string& field, std::function<Feed(const Feed&)> func) const;
    void apply_inplace(const std::string& field, std::function<Feed(const Feed&)> func);
    
    // Document/Text Operations (for text field collections)
    Catalog text_preprocess(const std::string& field) const;
    Catalog extract_keywords(const std::string& field, int top_n = 10) const;
    Catalog compute_similarity(const std::string& field1, const std::string& field2, 
                              const std::string& method = "cosine") const;
    Catalog filter_by_text(const std::string& field, const std::string& pattern) const;
    Catalog add_text_features(const std::string& field) const;
    Feed aggregate_text_stats(const std::string& field, const std::string& stat_type) const;
    Catalog tfidf(const std::string& field) const;
    
    // Display
    void display(int64_t max_rows = 10) const;
    void info() const;
    std::string to_string() const;
    
    // I/O operations (unified load API like DataFrame)
    static Catalog load(const std::string& format, const std::string& filename, const std::string& name = "");
    
    // CSV I/O (backwards compatibility - delegates to load)
    static Catalog read_csv(const std::string& filename, const std::string& name = "");
    static Catalog read_csv_with_key(const std::string& filename, const std::string& key_column, const std::string& name = "");
    void to_csv(const std::string& filename) const;
    
    // Serialization
    std::string to_json() const;
    static Catalog from_json(const std::string& json, const std::string& name = "");
    
    // Iterator support
    auto begin() { return fields_.begin(); }
    auto end() { return fields_.end(); }
    auto begin() const { return fields_.begin(); }
    auto end() const { return fields_.end(); }

private:
    std::unordered_map<std::string, Feed> fields_;
    std::string name_;
    
    void validate_lengths() const;
};

}  // namespace pluss::handle

