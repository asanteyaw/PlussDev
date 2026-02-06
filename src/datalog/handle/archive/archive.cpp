#include <pluss/datalog/handle/archive/archive.h>
#include <stdexcept>
#include <iostream>
#include <sstream>

namespace pluss::handle {

// ===== Catalog Access =====

Catalog& Archive::operator[](const std::string& key) {
    return catalogs_[key];
}

const Catalog& Archive::operator[](const std::string& key) const {
    return catalogs_.at(key);
}

Catalog& Archive::at(const std::string& key) {
    return catalogs_.at(key);
}

const Catalog& Archive::at(const std::string& key) const {
    return catalogs_.at(key);
}

bool Archive::has(const std::string& key) const {
    return catalogs_.find(key) != catalogs_.end();
}

void Archive::remove(const std::string& key) {
    catalogs_.erase(key);
}

void Archive::clear() {
    catalogs_.clear();
}

// ===== Metadata =====

std::vector<std::string> Archive::keys() const {
    std::vector<std::string> result;
    result.reserve(catalogs_.size());
    for (const auto& [key, _] : catalogs_) {
        result.push_back(key);
    }
    return result;
}

// ===== Batch Operations =====

void Archive::apply_all(std::function<void(Catalog&)> func) {
    for (auto& [_, catalog] : catalogs_) {
        func(catalog);
    }
}

Archive Archive::filter_all(const std::string& field, std::function<pluss::Series(const Feed&)> condition) const {
    Archive result(name_);
    
    for (const auto& [key, catalog] : catalogs_) {
        if (catalog.has(field)) {
            try {
                auto mask = condition(catalog.at(field));
                result[key] = catalog.filter(mask);
            } catch (...) {
                // Skip catalogs that can't be filtered
                continue;
            }
        }
    }
    
    return result;
}

// ===== Display =====

void Archive::display(int64_t max_rows) const {
    std::cout << "Archive: " << name_ << "\n";
    std::cout << "Catalogs: " << num_catalogs() << "\n\n";
    
    if (catalogs_.empty()) {
        std::cout << "(empty)\n";
        return;
    }
    
    for (const auto& [key, catalog] : catalogs_) {
        std::cout << "\n--- Catalog: " << key << " ---\n";
        catalog.display(max_rows);
    }
}

void Archive::info() const {
    std::cout << "=== Archive Info ===\n";
    std::cout << "Name: " << name_ << "\n";
    std::cout << "Catalogs: " << num_catalogs() << "\n\n";
    
    std::cout << "Catalog Details:\n";
    for (const auto& [key, catalog] : catalogs_) {
        std::cout << "  " << key << ": " << catalog.num_fields() << " fields, " 
                  << catalog.size() << " rows\n";
    }
}

std::string Archive::to_string() const {
    std::ostringstream oss;
    oss << "Archive(" << name_ << ", catalogs=" << num_catalogs() << ")";
    return oss.str();
}

// ===== Serialization =====

std::string Archive::to_json() const {
    std::ostringstream oss;
    oss << "{\"name\":\"" << name_ << "\",\"catalogs\":{";
    
    bool first = true;
    for (const auto& [key, catalog] : catalogs_) {
        if (!first) oss << ",";
        first = false;
        oss << "\"" << key << "\":" << catalog.to_json();
    }
    
    oss << "}}";
    return oss.str();
}

Archive Archive::from_json(const std::string& /*json*/, const std::string& name) {
    // Placeholder
    return Archive(name);
}

}  // namespace pluss::handle
