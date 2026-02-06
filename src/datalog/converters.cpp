#include <pluss/datalog/converters.h>
#include <pluss/note_factory.h>
#include <pluss/series_factory.h>
#include <unordered_set>
#include <stdexcept>
#include <algorithm>

namespace pluss::converters {

// ========== TABLIZE: Handle → Table ==========

table::Thread tablize(const handle::Feed& feed) {
    return table::Thread(feed.values(), feed.name());
}

table::DataFrame tablize(const handle::Catalog& catalog, const std::string& key_col_name) {
    if (catalog.empty()) {
        throw std::runtime_error("Cannot tablize empty Catalog");
    }
    
    if (!can_tablize(catalog)) {
        throw std::runtime_error("Cannot tablize Catalog: Feeds have different lengths");
    }
    
    // Start with empty DataFrame
    auto df = std::make_shared<table::DataFrame>();
    
    // Get all field names and collect as columns
    auto keys = catalog.keys();
    if (keys.empty()) {
        return *df;
    }
    
    // First, add key column
    std::vector<pluss::Chord> key_chords;
    key_chords.reserve(keys.size());
    for (const auto& key : keys) {
        key_chords.emplace_back(key);
    }
    df = df->set_col(key_col_name, table::Thread(pluss::make_note(key_chords), key_col_name));
    
    // Get all field names from catalog
    std::vector<std::string> field_names;
    for (const auto& key : keys) {
        const auto& feed = catalog.at(key);
        std::string fname = feed.name();
        if (std::find(field_names.begin(), field_names.end(), fname) == field_names.end()) {
            field_names.push_back(fname);
        }
    }
    
    // For each field, collect values row by row
    for (const auto& field_name : field_names) {
        // Determine type from first entry that has this field
        bool is_series_type = true;
        bool is_note_type = false;
        bool checked = false;
        
        for (const auto& key : keys) {
            if (catalog.has(key)) {
                const auto& feed = catalog.at(key);
                if (feed.name() == field_name) {
                    is_series_type = feed.is_series();
                    is_note_type = feed.is_note();
                    checked = true;
                    break;
                }
            }
        }
        
        if (!checked) continue;
        
        if (is_series_type) {
            std::vector<double> values;
            values.reserve(keys.size());
            for (const auto& key : keys) {
                const auto& feed = catalog.at(key);
                if (feed.name() == field_name && feed.is_series()) {
                    auto s = feed.as_series().to(at::kDouble);
                    if (s.numel() > 0) {
                        values.push_back(s[0].item<double>());
                    } else {
                        values.push_back(std::nan(""));
                    }
                } else {
                    values.push_back(std::nan(""));
                }
            }
            df = df->set_col(field_name, table::Thread(pluss::value(values), field_name));
            
        } else if (is_note_type) {
            std::vector<pluss::Chord> chords;
            chords.reserve(keys.size());
            for (const auto& key : keys) {
                const auto& feed = catalog.at(key);
                if (feed.name() == field_name && feed.is_note()) {
                    auto vec = feed.as_note().vector();
                    if (!vec.empty()) {
                        chords.push_back(vec[0]);
                    } else {
                        chords.emplace_back("");
                    }
                } else {
                    chords.emplace_back("");
                }
            }
            df = df->set_col(field_name, table::Thread(pluss::make_note(chords), field_name));
        }
    }
    
    return *df;
}

table::Atlas tablize(const handle::Archive& archive, const std::string& key_col_name) {
    if (!can_tablize(archive, key_col_name)) {
        throw std::runtime_error("Cannot tablize Archive: some Catalogs have inconsistent Feed lengths");
    }
    
    std::unordered_map<std::string, std::shared_ptr<table::DataFrame>> frames;
    
    for (const auto& [catalog_name, catalog] : archive) {
        try {
            auto df = tablize(catalog, key_col_name);
            frames[catalog_name] = std::make_shared<table::DataFrame>(std::move(df));
        } catch (const std::exception& e) {
            // Skip catalogs that can't be converted
            continue;
        }
    }
    
    return table::Atlas(frames);
}

// ========== HANDELIZE: Table → Handle ==========

handle::Feed handelize(const table::Thread& thread) {
    return handle::Feed(thread.values(), thread.name());
}

handle::Catalog handelize(const table::DataFrame& df, const std::string& key_col) {
    if (!can_handelize(df, key_col)) {
        throw std::runtime_error("Cannot handelize DataFrame: key column missing, not Note type, or has duplicate values");
    }
    
    handle::Catalog catalog("");
    
    // Get keys from key column
    auto keys_thread = df.get_col(key_col);
    auto keys = keys_thread.as_note().vector();
    
    // Get row count from key column
    int64_t n_rows = keys_thread.size();
    
    // Get all column names except key column
    auto all_cols = df.columns();
    std::vector<std::string> value_cols;
    for (const auto& col : all_cols) {
        if (col != key_col) {
            value_cols.push_back(col);
        }
    }
    
    // For each row, create a catalog entry
    for (int64_t i = 0; i < n_rows; ++i) {
        std::string key = keys[i].str();
        
        for (const auto& col_name : value_cols) {
            auto thread = df.get_col(col_name);
            
            // Extract single value at row i
            if (thread.is_series()) {
                auto s = thread.as_series();
                if (i < s.numel()) {
                    auto value = s.narrow(0, i, 1);
                    catalog[key] = handle::Feed(value, col_name);
                }
            } else if (thread.is_note()) {
                auto vec = thread.as_note().vector();
                if (i < static_cast<int64_t>(vec.size())) {
                    std::vector<pluss::Chord> single_chord = {vec[i]};
                    catalog[key] = handle::Feed(pluss::make_note(single_chord), col_name);
                }
            } else if (thread.is_timeline()) {
                // Timeline handling
                auto tl = thread.as_timeline();
                if (tl.has_date()) {
                    auto ds = tl.date_to_Series();
                    if (i < ds.numel()) {
                        auto value = ds.narrow(0, i, 1);
                        catalog[key] = handle::Feed(value, col_name);
                    }
                }
            }
        }
    }
    
    return catalog;
}

handle::Archive handelize(const table::Atlas& atlas, const std::string& key_col) {
    if (!can_handelize(atlas, key_col)) {
        throw std::runtime_error("Cannot handelize Atlas: some DataFrames cannot be converted");
    }
    
    handle::Archive archive("");
    
    // Iterate through all depths
    int64_t n_depths = atlas.ndepth();
    for (int64_t i = 0; i < n_depths; ++i) {
        try {
            auto df_ptr = atlas.at(i);
            if (df_ptr) {
                auto catalog = handelize(*df_ptr, key_col);
                // Use depth index as key since Atlas doesn't necessarily have names
                archive[std::to_string(i)] = catalog;
            }
        } catch (const std::exception& e) {
            // Skip DataFrames that can't be converted
            continue;
        }
    }
    
    return archive;
}

// ========== VALIDATION ==========

bool can_handelize(const table::DataFrame& df, const std::string& key_col) {
    // Check if key column exists
    auto all_cols = df.columns();
    if (std::find(all_cols.begin(), all_cols.end(), key_col) == all_cols.end()) {
        return false;
    }
    
    // Check if key column is Note type
    try {
        auto keys_thread = df.get_col(key_col);
        if (!keys_thread.is_note()) {
            return false;
        }
        
        // Check for unique keys
        auto keys = keys_thread.as_note().vector();
        std::unordered_set<std::string> unique_keys;
        for (const auto& chord : keys) {
            if (!unique_keys.insert(chord.str()).second) {
                return false;  // Duplicate found
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool can_tablize(const handle::Catalog& catalog) {
    if (catalog.empty()) {
        return false;
    }
    
    // Check all feeds have same length
    int64_t expected_size = -1;
    auto keys = catalog.keys();
    
    for (const auto& key : keys) {
        try {
            const auto& feed = catalog.at(key);
            int64_t feed_size = feed.size();
            
            if (expected_size == -1) {
                expected_size = feed_size;
            } else if (feed_size != expected_size) {
                return false;
            }
        } catch (...) {
            return false;
        }
    }
    
    return expected_size > 0;
}

bool can_tablize(const handle::Archive& archive, const std::string& /*key_col_name*/) {
    if (archive.empty()) {
        return false;
    }
    
    // Check all catalogs can be converted
    auto keys = archive.keys();
    for (const auto& key : keys) {
        try {
            const auto& catalog = archive.at(key);
            if (!can_tablize(catalog)) {
                return false;
            }
        } catch (...) {
            return false;
        }
    }
    
    return true;
}

bool can_handelize(const table::Atlas& atlas, const std::string& key_col) {
    if (atlas.empty()) {
        return false;
    }
    
    // Check all DataFrames can be converted
    int64_t n_depths = atlas.ndepth();
    for (int64_t i = 0; i < n_depths; ++i) {
        try {
            auto df_ptr = atlas.at(i);
            if (!df_ptr || !can_handelize(*df_ptr, key_col)) {
                return false;
            }
        } catch (...) {
            return false;
        }
    }
    
    return true;
}

}  // namespace pluss::converters
