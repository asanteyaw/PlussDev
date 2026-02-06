#include <pluss/datalog/handle/catalog/catalog.h>
#include <pluss/Series.h>
#include <pluss/series_factory.h>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <sstream>

namespace pluss::handle {

// ===== Field Access =====

Feed& Catalog::operator[](const std::string& key) {
    return fields_[key];
}

const Feed& Catalog::operator[](const std::string& key) const {
    return fields_.at(key);
}

Feed& Catalog::at(const std::string& key) {
    return fields_.at(key);
}

const Feed& Catalog::at(const std::string& key) const {
    return fields_.at(key);
}

bool Catalog::has(const std::string& key) const {
    return fields_.find(key) != fields_.end();
}

void Catalog::remove(const std::string& key) {
    fields_.erase(key);
}

void Catalog::clear() {
    fields_.clear();
}

// ===== Metadata =====

int64_t Catalog::size() const {
    if (fields_.empty()) return 0;
    return fields_.begin()->second.size();
}

std::vector<std::string> Catalog::keys() const {
    std::vector<std::string> result;
    result.reserve(fields_.size());
    for (const auto& [key, _] : fields_) {
        result.push_back(key);
    }
    return result;
}

// ===== Indexing and Slicing =====

Catalog Catalog::head(int64_t n) const {
    Catalog result(name_);
    for (const auto& [key, feed] : fields_) {
        result[key] = feed.head(n);
    }
    return result;
}

Catalog Catalog::tail(int64_t n) const {
    Catalog result(name_);
    for (const auto& [key, feed] : fields_) {
        result[key] = feed.tail(n);
    }
    return result;
}

Catalog Catalog::slice(int64_t start, int64_t end) const {
    Catalog result(name_);
    for (const auto& [key, feed] : fields_) {
        result[key] = feed.slice(start, end);
    }
    return result;
}

Catalog Catalog::filter(const pluss::Series& condition) const {
    Catalog result(name_);
    for (const auto& [key, feed] : fields_) {
        result[key] = feed.filter(condition);
    }
    return result;
}

Catalog Catalog::masked_select(const pluss::Series& mask) const {
    Catalog result(name_);
    for (const auto& [key, feed] : fields_) {
        result[key] = feed.masked_select(mask);
    }
    return result;
}

Catalog Catalog::iloc(int64_t start, int64_t end) const {
    return slice(start, end);
}

// ===== Field Operations =====

Catalog Catalog::select(const std::vector<std::string>& field_names) const {
    Catalog result(name_);
    for (const auto& field_name : field_names) {
        if (has(field_name)) {
            result[field_name] = fields_.at(field_name);
        }
    }
    return result;
}

Catalog Catalog::drop(const std::vector<std::string>& field_names) const {
    Catalog result(name_);
    for (const auto& [key, feed] : fields_) {
        if (std::find(field_names.begin(), field_names.end(), key) == field_names.end()) {
            result[key] = feed;
        }
    }
    return result;
}

Catalog Catalog::rename(const std::unordered_map<std::string, std::string>& mapping) const {
    Catalog result(name_);
    for (const auto& [key, feed] : fields_) {
        auto it = mapping.find(key);
        if (it != mapping.end()) {
            result[it->second] = feed;
        } else {
            result[key] = feed;
        }
    }
    return result;
}

// ===== Sorting =====

Catalog Catalog::sort(const std::string& by_field, bool ascending) const {
    if (!has(by_field)) {
        throw std::runtime_error("Field not found for sorting: " + by_field);
    }
    
    const auto& sort_feed = fields_.at(by_field);
    auto sorted_indices = sort_feed.argsort(ascending);
    
    // Use indices to reorder all fields
    Catalog result(name_);
    for (const auto& [key, feed] : fields_) {
        // TODO: Implement reordering using indices
        result[key] = feed;  // Placeholder
    }
    return result;
}

Catalog Catalog::sort_by(const std::vector<std::string>& fields, const std::vector<bool>& ascending) const {
    // Multi-column sort - placeholder
    if (fields.empty()) return *this;
    return sort(fields[0], ascending.empty() ? true : ascending[0]);
}

// ===== Grouping =====

std::unordered_map<std::string, Catalog> Catalog::group_by(const std::string& key_field) const {
    std::unordered_map<std::string, Catalog> groups;
    
    if (!has(key_field)) {
        return groups;
    }
    
    // Placeholder implementation
    return groups;
}

// ===== Aggregations =====

Feed Catalog::sum(const std::string& field) const {
    if (!has(field)) {
        throw std::runtime_error("Field not found: " + field);
    }
    const auto& feed = fields_.at(field);
    double total = feed.sum();
    return Feed(pluss::value({total}), field + "_sum");
}

Feed Catalog::mean(const std::string& field) const {
    if (!has(field)) {
        throw std::runtime_error("Field not found: " + field);
    }
    const auto& feed = fields_.at(field);
    double avg = feed.mean();
    return Feed(pluss::value({avg}), field + "_mean");
}

Feed Catalog::min(const std::string& field) const {
    if (!has(field)) {
        throw std::runtime_error("Field not found: " + field);
    }
    const auto& feed = fields_.at(field);
    double min_val = feed.min();
    return Feed(pluss::value({min_val}), field + "_min");
}

Feed Catalog::max(const std::string& field) const {
    if (!has(field)) {
        throw std::runtime_error("Field not found: " + field);
    }
    const auto& feed = fields_.at(field);
    double max_val = feed.max();
    return Feed(pluss::value({max_val}), field + "_max");
}

int64_t Catalog::count() const {
    return size();
}

// ===== Joins =====

Catalog Catalog::merge(const Catalog& other, const std::string& on, const std::string& how) const {
    // Placeholder join implementation
    Catalog result(name_);
    return result;
}

// ===== Transformations =====

Catalog Catalog::apply(const std::string& field, std::function<Feed(const Feed&)> func) const {
    Catalog result = *this;
    if (has(field)) {
        result[field] = func(fields_.at(field));
    }
    return result;
}

void Catalog::apply_inplace(const std::string& field, std::function<Feed(const Feed&)> func) {
    if (has(field)) {
        fields_[field] = func(fields_[field]);
    }
}

// ===== Display =====

void Catalog::display(int64_t max_rows) const {
    std::cout << "Catalog: " << name_ << "\n";
    std::cout << "Fields: " << num_fields() << ", Rows: " << size() << "\n\n";
    
    if (fields_.empty()) {
        std::cout << "(empty)\n";
        return;
    }
    
    // Print field names
    auto field_keys = keys();
    for (size_t i = 0; i < field_keys.size(); ++i) {
        std::cout << field_keys[i];
        if (i < field_keys.size() - 1) std::cout << " | ";
    }
    std::cout << "\n";
    
    // Print separator
    for (size_t i = 0; i < field_keys.size(); ++i) {
        std::cout << std::string(field_keys[i].size(), '-');
        if (i < field_keys.size() - 1) std::cout << "-+-";
    }
    std::cout << "\n";
    
    // Print rows (simplified)
    int64_t n_rows = std::min(max_rows, size());
    for (int64_t row = 0; row < n_rows; ++row) {
        for (size_t i = 0; i < field_keys.size(); ++i) {
            const auto& feed = fields_.at(field_keys[i]);
            if (feed.is_series()) {
                try {
                    auto s = feed.as_series();
                    if (row < s.numel()) {
                        std::cout << s[row].item<double>();
                    }
                } catch (...) {
                    std::cout << "?";
                }
            } else if (feed.is_note()) {
                try {
                    auto vec = feed.as_note().vector();
                    if (row < static_cast<int64_t>(vec.size())) {
                        std::cout << vec[row].str();
                    }
                } catch (...) {
                    std::cout << "?";
                }
            }
            if (i < field_keys.size() - 1) std::cout << " | ";
        }
        std::cout << "\n";
    }
    
    if (size() > max_rows) {
        std::cout << "... (" << (size() - max_rows) << " more rows)\n";
    }
}

void Catalog::info() const {
    std::cout << "=== Catalog Info ===\n";
    std::cout << "Name: " << name_ << "\n";
    std::cout << "Fields: " << num_fields() << "\n";
    std::cout << "Rows: " << size() << "\n\n";
    
    std::cout << "Field Details:\n";
    for (const auto& [key, feed] : fields_) {
        std::cout << "  " << key << ": " << feed.dtype() << " (size=" << feed.size() << ")\n";
    }
}

std::string Catalog::to_string() const {
    std::ostringstream oss;
    oss << "Catalog(" << name_ << ", fields=" << num_fields() << ", rows=" << size() << ")";
    return oss.str();
}

// ===== Serialization =====

std::string Catalog::to_json() const {
    std::ostringstream oss;
    oss << "{\"name\":\"" << name_ << "\",\"fields\":{";
    
    bool first = true;
    for (const auto& [key, feed] : fields_) {
        if (!first) oss << ",";
        first = false;
        oss << "\"" << key << "\":" << feed.to_json();
    }
    
    oss << "}}";
    return oss.str();
}

Catalog Catalog::from_json(const std::string& /*json*/, const std::string& name) {
    // Placeholder
    return Catalog(name);
}

// ===== Private =====

void Catalog::validate_lengths() const {
    if (fields_.empty()) return;
    
    int64_t expected_size = -1;
    for (const auto& [key, feed] : fields_) {
        int64_t feed_size = feed.size();
        if (expected_size == -1) {
            expected_size = feed_size;
        } else if (feed_size != expected_size) {
            throw std::runtime_error("Catalog fields have inconsistent lengths");
        }
    }
}

}  // namespace pluss::handle
