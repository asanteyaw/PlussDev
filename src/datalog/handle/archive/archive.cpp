#include <pluss/datalog/handle/archive/archive.h>
#include <pluss/note_factory.h>
#include <pluss/series_factory.h>
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

Archive Archive::transform_all(std::function<Catalog(const Catalog&)> func) const {
    Archive result(name_);
    for (const auto& [key, catalog] : catalogs_) {
        result[key] = func(catalog);
    }
    return result;
}

// ===== Cross-Catalog Operations =====

Catalog Archive::union_catalogs(const std::vector<std::string>& catalog_keys) const {
    if (catalog_keys.empty()) {
        throw std::runtime_error("No catalog keys provided for union");
    }
    
    // Start with first catalog
    if (!has(catalog_keys[0])) {
        throw std::runtime_error("Catalog not found: " + catalog_keys[0]);
    }
    
    Catalog result = catalogs_.at(catalog_keys[0]);
    result.set_name(name_ + "_union");
    
    // Union with remaining catalogs
    for (size_t i = 1; i < catalog_keys.size(); ++i) {
        if (!has(catalog_keys[i])) {
            continue;
        }
        
        const auto& cat = catalogs_.at(catalog_keys[i]);
        
        // For each field in the catalog being added
        for (const auto& field_key : cat.keys()) {
            if (result.has(field_key)) {
                // Concatenate feeds
                std::vector<Feed> feeds = {result[field_key], cat.at(field_key)};
                // Note: Would need Feed::concatenate() method
                // Placeholder: just keep the first one
            } else {
                // Add new field (would need to handle length mismatch)
                // result[field_key] = cat.at(field_key);
            }
        }
    }
    
    return result;
}

Catalog Archive::merge_catalogs(const std::string& key1, const std::string& key2,
                                 const std::string& on, const std::string& how) const {
    if (!has(key1)) {
        throw std::runtime_error("Catalog not found: " + key1);
    }
    if (!has(key2)) {
        throw std::runtime_error("Catalog not found: " + key2);
    }
    
    return catalogs_.at(key1).merge(catalogs_.at(key2), on, how);
}

// ===== Aggregations =====

std::unordered_map<std::string, double> Archive::aggregate_field(
    const std::string& field, const std::string& agg_func) const {
    
    std::unordered_map<std::string, double> results;
    
    for (const auto& [key, catalog] : catalogs_) {
        if (!catalog.has(field)) continue;
        
        try {
            if (agg_func == "sum") {
                results[key] = catalog.sum(field).sum();
            } else if (agg_func == "mean") {
                results[key] = catalog.mean(field).mean();
            } else if (agg_func == "min") {
                results[key] = catalog.min(field).min();
            } else if (agg_func == "max") {
                results[key] = catalog.max(field).max();
            } else if (agg_func == "count") {
                results[key] = static_cast<double>(catalog.count());
            }
        } catch (...) {
            // Skip on error
            continue;
        }
    }
    
    return results;
}

Catalog Archive::aggregate_all(const std::string& field, const std::string& agg_func) const {
    Catalog result(name_ + "_aggregated");
    
    std::vector<std::string> catalog_names;
    std::vector<double> agg_values;
    
    auto agg_map = aggregate_field(field, agg_func);
    for (const auto& [key, value] : agg_map) {
        catalog_names.push_back(key);
        agg_values.push_back(value);
    }
    
    // Create result catalog - convert to vector<Chord>
    std::vector<Chord> catalog_chords;
    catalog_chords.reserve(catalog_names.size());
    for (const auto& name : catalog_names) {
        catalog_chords.push_back(Chord(name));
    }
    result["catalog"] = Feed(pluss::make_note(catalog_chords), "catalog");
    result[agg_func] = Feed(pluss::value(agg_values), agg_func);
    
    return result;
}

// ===== Search Operations =====

Archive Archive::search_catalogs(const std::string& field, const std::string& pattern) const {
    Archive result(name_ + "_search_results");
    
    for (const auto& [key, catalog] : catalogs_) {
        if (catalog.has(field)) {
            try {
                const auto& feed = catalog.at(field);
                if (feed.is_note()) {
                    auto mask = feed.str_contains(pattern);
                    auto filtered = catalog.filter(mask.as_series());
                    if (filtered.size() > 0) {
                        result[key] = filtered;
                    }
                }
            } catch (...) {
                continue;
            }
        }
    }
    
    return result;
}

std::vector<std::string> Archive::find_catalogs_with_field(const std::string& field) const {
    std::vector<std::string> result;
    
    for (const auto& [key, catalog] : catalogs_) {
        if (catalog.has(field)) {
            result.push_back(key);
        }
    }
    
    return result;
}

// ===== Statistical Summaries =====

void Archive::summary() const {
    std::cout << "\n=== Archive Summary ===\n";
    std::cout << "Name: " << name_ << "\n";
    std::cout << "Total Catalogs: " << num_catalogs() << "\n";
    std::cout << "Total Rows: " << total_rows() << "\n\n";
    
    std::cout << "Catalog Breakdown:\n";
    for (const auto& [key, catalog] : catalogs_) {
        std::cout << "  " << key << ":\n";
        std::cout << "    Fields: " << catalog.num_fields() << "\n";
        std::cout << "    Rows: " << catalog.size() << "\n";
        std::cout << "    Fields: [";
        auto field_keys = catalog.keys();
        for (size_t i = 0; i < field_keys.size(); ++i) {
            std::cout << field_keys[i];
            if (i < field_keys.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
    }
}

std::unordered_map<std::string, int64_t> Archive::catalog_sizes() const {
    std::unordered_map<std::string, int64_t> sizes;
    for (const auto& [key, catalog] : catalogs_) {
        sizes[key] = catalog.size();
    }
    return sizes;
}

int64_t Archive::total_rows() const {
    int64_t total = 0;
    for (const auto& [_, catalog] : catalogs_) {
        total += catalog.size();
    }
    return total;
}

// ===== Corpus Management =====

Archive Archive::preprocess_text_fields(const std::string& field) const {
    Archive result(name_ + "_preprocessed");
    
    for (const auto& [key, catalog] : catalogs_) {
        if (catalog.has(field)) {
            result[key] = catalog.text_preprocess(field);
        }
    }
    
    return result;
}

Catalog Archive::corpus_statistics(const std::string& field) const {
    Catalog stats("corpus_statistics");
    
    std::vector<std::string> catalog_names;
    std::vector<double> total_docs;
    std::vector<double> total_words;
    std::vector<double> avg_doc_lengths;
    std::vector<double> unique_word_counts;
    
    for (const auto& [key, catalog] : catalogs_) {
        if (catalog.has(field)) {
            catalog_names.push_back(key);
            total_docs.push_back(static_cast<double>(catalog.size()));
            
            try {
                auto word_counts = catalog.aggregate_text_stats(field, "word_count");
                total_words.push_back(word_counts.sum());
                
                auto avg_lengths = catalog.aggregate_text_stats(field, "avg_length");
                avg_doc_lengths.push_back(avg_lengths.mean());
                
                auto unique_words = catalog.aggregate_text_stats(field, "unique_words");
                unique_word_counts.push_back(unique_words.sum());
            } catch (...) {
                total_words.push_back(0.0);
                avg_doc_lengths.push_back(0.0);
                unique_word_counts.push_back(0.0);
            }
        }
    }
    
    // Convert to vector<Chord>
    std::vector<Chord> catalog_chords;
    catalog_chords.reserve(catalog_names.size());
    for (const auto& name : catalog_names) {
        catalog_chords.push_back(Chord(name));
    }
    stats["catalog"] = Feed(pluss::make_note(catalog_chords), "catalog");
    stats["num_documents"] = Feed(pluss::value(total_docs), "num_documents");
    stats["total_words"] = Feed(pluss::value(total_words), "total_words");
    stats["avg_doc_length"] = Feed(pluss::value(avg_doc_lengths), "avg_doc_length");
    stats["unique_words"] = Feed(pluss::value(unique_word_counts), "unique_words");
    
    return stats;
}

Archive Archive::extract_all_keywords(const std::string& field, int top_n) const {
    Archive result(name_ + "_keywords");
    
    for (const auto& [key, catalog] : catalogs_) {
        if (catalog.has(field)) {
            result[key] = catalog.extract_keywords(field, top_n);
        }
    }
    
    return result;
}

Catalog Archive::cross_catalog_similarity(const std::string& field, const std::string& method) const {
    Catalog result("cross_catalog_similarity");
    
    auto catalog_keys = keys();
    if (catalog_keys.size() < 2) {
        return result;
    }
    
    std::vector<std::string> pairs;
    std::vector<double> similarities;
    
    for (size_t i = 0; i < catalog_keys.size(); ++i) {
        for (size_t j = i + 1; j < catalog_keys.size(); ++j) {
            const std::string& key1 = catalog_keys[i];
            const std::string& key2 = catalog_keys[j];
            
            if (!catalogs_.at(key1).has(field) || !catalogs_.at(key2).has(field)) {
                continue;
            }
            
            try {
                const auto& feed1 = catalogs_.at(key1).at(field);
                const auto& feed2 = catalogs_.at(key2).at(field);
                
                double sim = 0.0;
                if (method == "cosine") {
                    sim = feed1.cosine_similarity(feed2);
                } else if (method == "jaccard") {
                    sim = feed1.jaccard_similarity(feed2);
                }
                
                pairs.push_back(key1 + " <-> " + key2);
                similarities.push_back(sim);
            } catch (...) {
                continue;
            }
        }
    }
    
    // Convert pairs to vector<Chord>
    std::vector<Chord> pair_chords;
    pair_chords.reserve(pairs.size());
    for (const auto& pair : pairs) {
        pair_chords.push_back(Chord(pair));
    }
    result["catalog_pair"] = Feed(pluss::make_note(pair_chords), "catalog_pair");
    result["similarity"] = Feed(pluss::value(similarities), "similarity");
    
    return result;
}

Archive Archive::filter_by_topic(const std::string& field, const std::string& topic_keyword) const {
    Archive result(name_ + "_filtered");
    
    for (const auto& [key, catalog] : catalogs_) {
        if (catalog.has(field)) {
            try {
                auto filtered = catalog.filter_by_text(field, topic_keyword);
                if (filtered.size() > 0) {
                    result[key] = filtered;
                }
            } catch (...) {
                continue;
            }
        }
    }
    
    return result;
}

Catalog Archive::document_term_matrix(const std::string& field) const {
    Catalog dtm("document_term_matrix");
    
    std::vector<std::string> doc_ids;
    std::unordered_map<std::string, std::vector<double>> term_frequencies;
    
    int doc_counter = 0;
    
    // Collect all documents and build vocabulary
    for (const auto& [cat_key, catalog] : catalogs_) {
        if (!catalog.has(field)) continue;
        
        try {
            const auto& feed = catalog.at(field);
            auto tokenized = feed.tokenize("whitespace").as_note().vector();
            
            for (const auto& chord : tokenized) {
                std::string doc_id = cat_key + "_" + std::to_string(doc_counter++);
                doc_ids.push_back(doc_id);
                
                // Count term frequencies in this document
                std::string token = chord.str();
                if (term_frequencies.find(token) == term_frequencies.end()) {
                    term_frequencies[token] = std::vector<double>(doc_counter, 0.0);
                }
                term_frequencies[token].back() += 1.0;
            }
        } catch (...) {
            continue;
        }
    }
    
    // Build DTM catalog - convert doc_ids to vector<Chord>
    std::vector<Chord> doc_id_chords;
    doc_id_chords.reserve(doc_ids.size());
    for (const auto& id : doc_ids) {
        doc_id_chords.push_back(Chord(id));
    }
    dtm["doc_id"] = Feed(pluss::make_note(doc_id_chords), "doc_id");
    
    for (const auto& [term, frequencies] : term_frequencies) {
        if (frequencies.size() == doc_ids.size()) {
            dtm[term] = Feed(pluss::value(frequencies), term);
        }
    }
    
    return dtm;
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

// ===== CSV I/O =====

Archive Archive::load_collection(const std::string& format, const std::vector<std::pair<std::string, std::string>>& files_and_keys) {
    if (format != "csv") {
        throw std::runtime_error("Only CSV format is currently supported for Archive::load_collection");
    }
    
    Archive archive("collection");
    
    for (const auto& [filename, key] : files_and_keys) {
        try {
            auto catalog = Catalog::load(format, filename, key);
            archive[key] = catalog;
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to load " << filename << ": " << e.what() << std::endl;
        }
    }
    
    return archive;
}

Archive Archive::read_csv_collection(const std::vector<std::pair<std::string, std::string>>& files_and_keys) {
    return load_collection("csv", files_and_keys);
}

Archive Archive::load_directory(const std::string& format, const std::string& directory_path, const std::string& /*pattern*/) {
    if (format != "csv") {
        throw std::runtime_error("Only CSV format is currently supported for Archive::load_directory");
    }
    
    Archive archive("directory");
    
    // Simplified implementation - would need filesystem library for full directory traversal
    // This is a placeholder that shows the intended structure
    // In production, would use std::filesystem to enumerate CSV files
    
    std::cerr << "Warning: load_directory requires std::filesystem implementation\n";
    std::cerr << "Use load_collection with explicit file list instead\n";
    
    return archive;
}

Archive Archive::read_csv_directory(const std::string& directory_path, const std::string& pattern) {
    return load_directory("csv", directory_path, pattern);
}

void Archive::to_csv_collection(const std::string& directory_path) const {
    for (const auto& [key, catalog] : catalogs_) {
        std::string filename = directory_path + "/" + key + ".csv";
        try {
            catalog.to_csv(filename);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to write " << filename << ": " << e.what() << "\n";
            continue;
        }
    }
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
