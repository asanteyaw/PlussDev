#include <pluss/pluss.h>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fstream>
#include <unordered_set>

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
    auto sorted_indices_feed = sort_feed.argsort(ascending);
    auto sorted_indices_series = sorted_indices_feed.as_series();
    
    // Use indices to reorder all fields
    Catalog result(name_);
    for (const auto& [key, feed] : fields_) {
        result[key] = feed.take(sorted_indices_series);
    }
    return result;
}

Catalog Catalog::sort_by(const std::vector<std::string>& fields, const std::vector<bool>& ascending) const {
    if (fields.empty()) return *this;
    
    // Validate all fields exist
    for (const auto& field : fields) {
        if (!has(field)) {
            throw std::runtime_error("Field not found for sorting: " + field);
        }
    }
    
    // Build ascending vector if not provided
    std::vector<bool> asc_vec = ascending;
    if (asc_vec.size() < fields.size()) {
        asc_vec.resize(fields.size(), true);
    }
    
    // Start with sorted result by last field, then apply earlier fields in reverse
    Catalog result = *this;
    for (int i = static_cast<int>(fields.size()) - 1; i >= 0; --i) {
        result = result.sort(fields[i], asc_vec[i]);
    }
    
    return result;
}

// ===== Grouping =====

std::unordered_map<std::string, Catalog> Catalog::group_by(const std::string& key_field) const {
    std::unordered_map<std::string, Catalog> groups;
    
    if (!has(key_field)) {
        throw std::runtime_error("Field not found for grouping: " + key_field);
    }
    
    const auto& key_feed = fields_.at(key_field);
    
    // Build groups based on field type
    if (key_feed.is_series()) {
        // For numeric data, group by value
        auto series = key_feed.as_series();
        std::unordered_map<double, std::vector<int64_t>> value_indices;
        
        for (int64_t i = 0; i < series.numel(); ++i) {
            double val = series[i].item<double>();
            value_indices[val].push_back(i);
        }
        
        // Create catalog for each group
        for (const auto& [val, indices] : value_indices) {
            std::string group_key = std::to_string(val);
            Catalog group_cat(name_ + "_" + group_key);
            
            // Build index series for filtering
            auto idx_tensor = torch::series(indices, torch::kLong);
            pluss::Series idx_series = pluss::from_torch(idx_tensor);
            
            // Take rows for this group
            for (const auto& [field_name, feed] : fields_) {
                group_cat[field_name] = feed.take(idx_series);
            }
            
            groups[group_key] = group_cat;
        }
        
    } else if (key_feed.is_note()) {
        // For text data, group by string value
        auto note = key_feed.as_note();
        auto vec = note.vector();
        std::unordered_map<std::string, std::vector<int64_t>> string_indices;
        
        for (size_t i = 0; i < vec.size(); ++i) {
            std::string val = vec[i].str();
            string_indices[val].push_back(static_cast<int64_t>(i));
        }
        
        // Create catalog for each group
        for (const auto& [val, indices] : string_indices) {
            Catalog group_cat(name_ + "_" + val);
            
            // Build index series for filtering
            auto idx_tensor = torch::series(indices, torch::kLong);
            pluss::Series idx_series = pluss::from_torch(idx_tensor);
            
            // Take rows for this group
            for (const auto& [field_name, feed] : fields_) {
                group_cat[field_name] = feed.take(idx_series);
            }
            
            groups[val] = group_cat;
        }
    }
    
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
    if (!has(on)) {
        throw std::runtime_error("Key field not found in left catalog: " + on);
    }
    if (!other.has(on)) {
        throw std::runtime_error("Key field not found in right catalog: " + on);
    }
    
    const auto& left_key = fields_.at(on);
    const auto& right_key = other.fields_.at(on);
    
    Catalog result(name_ + "_merged");
    
    // Build index maps
    std::unordered_map<std::string, std::vector<int64_t>> left_map, right_map;
    
    // Build left map
    if (left_key.is_series()) {
        auto series = left_key.as_series();
        for (int64_t i = 0; i < series.numel(); ++i) {
            std::string key = std::to_string(series[i].item<double>());
            left_map[key].push_back(i);
        }
    } else if (left_key.is_note()) {
        auto vec = left_key.as_note().vector();
        for (size_t i = 0; i < vec.size(); ++i) {
            std::string key = vec[i].str();
            left_map[key].push_back(static_cast<int64_t>(i));
        }
    }
    
    // Build right map
    if (right_key.is_series()) {
        auto series = right_key.as_series();
        for (int64_t i = 0; i < series.numel(); ++i) {
            std::string key = std::to_string(series[i].item<double>());
            right_map[key].push_back(i);
        }
    } else if (right_key.is_note()) {
        auto vec = right_key.as_note().vector();
        for (size_t i = 0; i < vec.size(); ++i) {
            std::string key = vec[i].str();
            right_map[key].push_back(static_cast<int64_t>(i));
        }
    }
    
    // Perform join based on type
    std::vector<int64_t> left_indices, right_indices;
    
    if (how == "inner") {
        // Inner join - only keys present in both
        for (const auto& [key, left_idx] : left_map) {
            if (right_map.find(key) != right_map.end()) {
                const auto& right_idx = right_map[key];
                for (auto l : left_idx) {
                    for (auto r : right_idx) {
                        left_indices.push_back(l);
                        right_indices.push_back(r);
                    }
                }
            }
        }
    } else if (how == "left") {
        // Left join - all left keys
        for (const auto& [key, left_idx] : left_map) {
            if (right_map.find(key) != right_map.end()) {
                const auto& right_idx = right_map[key];
                for (auto l : left_idx) {
                    for (auto r : right_idx) {
                        left_indices.push_back(l);
                        right_indices.push_back(r);
                    }
                }
            } else {
                // No match in right, use -1 for right index (will fill with NA)
                for (auto l : left_idx) {
                    left_indices.push_back(l);
                    right_indices.push_back(-1);
                }
            }
        }
    } else if (how == "right") {
        // Right join - all right keys
        for (const auto& [key, right_idx] : right_map) {
            if (left_map.find(key) != left_map.end()) {
                const auto& left_idx = left_map[key];
                for (auto l : left_idx) {
                    for (auto r : right_idx) {
                        left_indices.push_back(l);
                        right_indices.push_back(r);
                    }
                }
            } else {
                // No match in left, use -1 for left index
                for (auto r : right_idx) {
                    left_indices.push_back(-1);
                    right_indices.push_back(r);
                }
            }
        }
    } else if (how == "outer") {
        // Outer join - all keys from both
        std::unordered_set<std::string> all_keys;
        for (const auto& [key, _] : left_map) all_keys.insert(key);
        for (const auto& [key, _] : right_map) all_keys.insert(key);
        
        for (const auto& key : all_keys) {
            bool has_left = left_map.find(key) != left_map.end();
            bool has_right = right_map.find(key) != right_map.end();
            
            if (has_left && has_right) {
                const auto& left_idx = left_map[key];
                const auto& right_idx = right_map[key];
                for (auto l : left_idx) {
                    for (auto r : right_idx) {
                        left_indices.push_back(l);
                        right_indices.push_back(r);
                    }
                }
            } else if (has_left) {
                const auto& left_idx = left_map[key];
                for (auto l : left_idx) {
                    left_indices.push_back(l);
                    right_indices.push_back(-1);
                }
            } else {
                const auto& right_idx = right_map[key];
                for (auto r : right_idx) {
                    left_indices.push_back(-1);
                    right_indices.push_back(r);
                }
            }
        }
    }
    
    // Build result catalog
    auto left_idx_tensor = torch::series(left_indices, torch::kLong);
    auto right_idx_tensor = torch::series(right_indices, torch::kLong);
    pluss::Series left_idx_series = pluss::from_torch(left_idx_tensor);
    pluss::Series right_idx_series = pluss::from_torch(right_idx_tensor);
    
    // Add left fields
    for (const auto& [field_name, feed] : fields_) {
        // Filter out -1 indices for left (would need NA handling)
        std::vector<int64_t> valid_left;
        for (auto idx : left_indices) {
            if (idx >= 0) valid_left.push_back(idx);
        }
        if (!valid_left.empty() || how == "inner") {
            result[field_name] = feed.take(left_idx_series);
        }
    }
    
    // Add right fields (skip the key field to avoid duplication)
    for (const auto& [field_name, feed] : other.fields_) {
        if (field_name != on) {
            std::string new_name = field_name + "_right";
            result[new_name] = feed.take(right_idx_series);
        }
    }
    
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

// ===== Document/Text Operations =====

Catalog Catalog::text_preprocess(const std::string& field) const {
    if (!has(field)) {
        throw std::runtime_error("Field not found: " + field);
    }
    
    Catalog result = *this;
    const auto& feed = fields_.at(field);
    
    // Apply comprehensive text preprocessing pipeline
    auto processed = feed
        .str_lower()
        .remove_punctuation()
        .str_trim()
        .remove_stopwords();
    
    result[field] = processed;
    return result;
}

Catalog Catalog::extract_keywords(const std::string& field, int top_n) const {
    if (!has(field)) {
        throw std::runtime_error("Field not found: " + field);
    }
    
    Catalog result = *this;
    const auto& feed = fields_.at(field);
    
    auto keywords = feed.extract_keywords(top_n);
    result[field + "_keywords"] = keywords;
    
    return result;
}

Catalog Catalog::compute_similarity(const std::string& field1, const std::string& field2,
                                     const std::string& method) const {
    if (!has(field1) || !has(field2)) {
        throw std::runtime_error("One or both fields not found");
    }
    
    Catalog result = *this;
    const auto& feed1 = fields_.at(field1);
    const auto& feed2 = fields_.at(field2);
    
    // Compute pairwise similarity
    auto sim_scores = feed1.pairwise_similarity(feed2, method);
    result["similarity_score"] = sim_scores;
    
    return result;
}

Catalog Catalog::filter_by_text(const std::string& field, const std::string& pattern) const {
    if (!has(field)) {
        throw std::runtime_error("Field not found: " + field);
    }
    
    const auto& feed = fields_.at(field);
    auto mask = feed.str_contains(pattern);
    
    return filter(mask.as_series());
}

Catalog Catalog::add_text_features(const std::string& field) const {
    if (!has(field)) {
        throw std::runtime_error("Field not found: " + field);
    }
    
    Catalog result = *this;
    const auto& feed = fields_.at(field);
    
    // Add various text feature columns
    result[field + "_word_count"] = feed.word_count();
    result[field + "_char_count"] = feed.char_count();
    result[field + "_unique_words"] = feed.unique_word_count();
    result[field + "_avg_word_len"] = feed.avg_word_length();
    result[field + "_sentence_count"] = feed.sentence_count();
    
    return result;
}

Feed Catalog::aggregate_text_stats(const std::string& field, const std::string& stat_type) const {
    if (!has(field)) {
        throw std::runtime_error("Field not found: " + field);
    }
    
    const auto& feed = fields_.at(field);
    
    if (stat_type == "word_count") {
        auto wc = feed.word_count();
        double total = wc.sum();
        return Feed(pluss::value({total}), "total_words");
    } else if (stat_type == "avg_length") {
        auto lengths = feed.str_length();
        double avg = lengths.mean();
        return Feed(pluss::value({avg}), "avg_text_length");
    } else if (stat_type == "unique_words") {
        auto uw = feed.unique_word_count();
        double total = uw.sum();
        return Feed(pluss::value({total}), "total_unique_words");
    }
    
    return Feed(pluss::value({0.0}), "unknown_stat");
}

Catalog Catalog::tfidf(const std::string& field) const {
    if (!has(field)) {
        throw std::runtime_error("Field not found: " + field);
    }
    
    Catalog result = *this;
    const auto& feed = fields_.at(field);
    
    // Simplified TF-IDF computation
    // 1. Tokenize all documents
    auto tokenized = feed.tokenize("whitespace");
    
    // 2. Compute document frequencies (simplified - would need proper implementation)
    // For now, return tokenized version
    result[field + "_tfidf"] = tokenized;
    
    return result;
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

// ===== CSV I/O =====

Catalog Catalog::load(const std::string& format, const std::string& filename, const std::string& name) {
    if (format != "csv") {
        throw std::runtime_error("Only CSV format is currently supported for Catalog::load");
    }
    
    // Use DataFrame's public load() method
    auto df = pluss::table::DataFrame::load(format, filename);
    
    // Create catalog with row indices as keys
    Catalog catalog(name.empty() ? filename : name);
    
    // Get all columns from DataFrame
    for (const auto& col_name : df->columns()) {
        try {
            auto thread = df->get_col(col_name);
            
            // Convert Thread to Feed 
            // Thread has as_series(), as_note(), or as_timeline() methods
            if (thread.is_series()) {
                Feed feed(thread.as_series(), col_name);
                catalog[col_name] = feed;
            } else if (thread.is_note()) {
                Feed feed(thread.as_note(), col_name);
                catalog[col_name] = feed;
            } else if (thread.is_timeline()) {
                Feed feed(thread.as_timeline(), col_name);
                catalog[col_name] = feed;
            }
        } catch (...) {
            // Skip columns that can't be converted
            continue;
        }
    }
    
    return catalog;
}

Catalog Catalog::read_csv(const std::string& filename, const std::string& name) {
    return load("csv", filename, name);
}

Catalog Catalog::read_csv_with_key(const std::string& filename, const std::string& key_column, const std::string& name) {
    // Use DataFrame's public load() method
    auto df = pluss::table::DataFrame::load("csv", filename);
    
    // Verify key column exists by checking if get_col throws
    bool has_key = false;
    try {
        df->get_col(key_column);
        has_key = true;
    } catch (...) {
        has_key = false;
    }
    
    if (!has_key) {
        throw std::runtime_error("Key column not found in CSV: " + key_column);
    }
    
    // Use converters to convert DataFrame to Catalog with specified key
    // For now, we'll use the simpler approach without key-based indexing
    Catalog catalog = load("csv", filename, name);
    return catalog;
}

void Catalog::to_csv(const std::string& filename) const {
    // Convert Catalog to DataFrame using converters
    auto df = pluss::converters::tablize(*this, "_key");
    
    // Use DataFrame's CSV writing (if available)
    // For now, manually write CSV
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }
    
    // Write header
    auto field_names = keys();
    for (size_t i = 0; i < field_names.size(); ++i) {
        file << field_names[i];
        if (i < field_names.size() - 1) file << ",";
    }
    file << "\n";
    
    // Write rows
    int64_t n_rows = size();
    for (int64_t row = 0; row < n_rows; ++row) {
        for (size_t i = 0; i < field_names.size(); ++i) {
            const auto& field_name = field_names[i];
            const auto& feed = fields_.at(field_name);
            
            if (feed.is_series()) {
                try {
                    auto s = feed.as_series();
                    if (row < s.numel()) {
                        file << s[row].item<double>();
                    }
                } catch (...) {
                    file << "";
                }
            } else if (feed.is_note()) {
                try {
                    auto vec = feed.as_note().vector();
                    if (row < static_cast<int64_t>(vec.size())) {
                        std::string val = vec[row].str();
                        // Escape quotes and wrap in quotes if contains comma
                        if (val.find(',') != std::string::npos || val.find('"') != std::string::npos) {
                            std::string escaped = val;
                            size_t pos = 0;
                            while ((pos = escaped.find('"', pos)) != std::string::npos) {
                                escaped.insert(pos, "\"");
                                pos += 2;
                            }
                            file << "\"" << escaped << "\"";
                        } else {
                            file << val;
                        }
                    }
                } catch (...) {
                    file << "";
                }
            }
            
            if (i < field_names.size() - 1) file << ",";
        }
        file << "\n";
    }
    
    file.close();
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
