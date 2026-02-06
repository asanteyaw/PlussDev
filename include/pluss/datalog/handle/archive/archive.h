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
    
    // Display
    void display(int64_t max_rows = 10) const;
    void info() const;
    std::string to_string() const;
    
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
