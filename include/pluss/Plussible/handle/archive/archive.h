#pragma once

#include <pluss/datalog/handle/catalog/catalog.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace pluss::handle {

/**
 * @brief Archive - Dictionary-based collection of Catalogs (analogous to Atlas)
 * 
 * An Archive is a 3D dictionary collection where:
 * - Keys are catalog names (strings)
 * - Values are Catalogs (2D dictionary tables)
 * 
 * Archive is to Catalog what Atlas is to DataFrame.
 * 
 * Example:
 * ```cpp
 * Archive archive("company_data");
 * archive["users"] = users_catalog;
 * archive["products"] = products_catalog;
 * archive["orders"] = orders_catalog;
 * 
 * archive["users"].filter(archive["users"]["active"].eq(true)).display();
 * archive.info();
 * ```
 */
class Archive {
public:
    Archive() = default;
    explicit Archive(std::string name) : name_(std::move(name)) {}

    // Catalog access
    Catalog& operator[](const std::string& key);
    const Catalog& operator[](const std::string& key) const;
    Catalog& at(const std::string& key);
    const Catalog& at(const std::string& key) const;
    
    bool has(const std::string& key) const;
    void remove(const std::string& key);
    void clear();
    
    // Metadata
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    void set_name(const std::string& n) { name_ = n; }
    [[nodiscard]] int64_t num_catalogs() const { return static_cast<int64_t>(catalogs_.size()); }
    [[nodiscard]] std::vector<std::string> keys() const;
    [[nodiscard]] bool empty() const { return catalogs_.empty(); }
    
    // Batch operations
    void apply_all(std::function<void(Catalog&)> func);
    Archive filter_all(const std::string& field, std::function<pluss::Series(const Feed&)> condition) const;
    Archive transform_all(std::function<Catalog(const Catalog&)> func) const;
    
    // Cross-catalog operations
    Catalog union_catalogs(const std::vector<std::string>& catalog_keys) const;
    Catalog merge_catalogs(const std::string& key1, const std::string& key2, 
                          const std::string& on, const std::string& how = "inner") const;
    
    // Aggregations across catalogs
    std::unordered_map<std::string, double> aggregate_field(const std::string& field, 
                                                             const std::string& agg_func) const;
    Catalog aggregate_all(const std::string& field, const std::string& agg_func) const;
    
    // Search operations
    Archive search_catalogs(const std::string& field, const std::string& pattern) const;
    std::vector<std::string> find_catalogs_with_field(const std::string& field) const;
    
    // Statistical summaries
    void summary() const;
    std::unordered_map<std::string, int64_t> catalog_sizes() const;
    int64_t total_rows() const;
    
    // Corpus Management (for text analytics)
    Archive preprocess_text_fields(const std::string& field) const;
    Catalog corpus_statistics(const std::string& field) const;
    Archive extract_all_keywords(const std::string& field, int top_n = 10) const;
    Catalog cross_catalog_similarity(const std::string& field, const std::string& method = "cosine") const;
    Archive filter_by_topic(const std::string& field, const std::string& topic_keyword) const;
    Catalog document_term_matrix(const std::string& field) const;
    
    // Display
    void display(int64_t max_rows = 10) const;
    void info() const;
    std::string to_string() const;
    
    // I/O operations (unified load API like DataFrame)
    static Archive load_collection(const std::string& format, const std::vector<std::pair<std::string, std::string>>& files_and_keys);
    static Archive load_directory(const std::string& format, const std::string& directory_path, const std::string& pattern = "*");
    
    // CSV I/O (backwards compatibility - delegates to load_collection/load_directory)
    static Archive read_csv_collection(const std::vector<std::pair<std::string, std::string>>& files_and_keys);
    static Archive read_csv_directory(const std::string& directory_path, const std::string& pattern = "*.csv");
    void to_csv_collection(const std::string& directory_path) const;
    
    // Serialization
    std::string to_json() const;
    static Archive from_json(const std::string& json, const std::string& name = "");
    
    // Iterator support
    auto begin() { return catalogs_.begin(); }
    auto end() { return catalogs_.end(); }
    auto begin() const { return catalogs_.begin(); }
    auto end() const { return catalogs_.end(); }

private:
    std::unordered_map<std::string, Catalog> catalogs_;
    std::string name_;
};

}  // namespace pluss::handle
