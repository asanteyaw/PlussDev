/**
 * @file memo.h
 * @brief Memo (1D) - MongoDB Document-like flexible data structure
 * 
 * Memo is the fundamental building block of the Register module (Memo-First design).
 * It represents a flexible, heterogeneous document similar to MongoDB's BSON documents.
 * 
 * Key Features:
 * - Stores heterogeneous data (scalars, arrays, nested documents)
 * - Type-safe variant-based implementation
 * - MongoDB-style operators ($eq, $gt, $in, etc.)
 * - Dot notation for nested field access
 * - Efficient vectorized operations
 * - Immutable operations (returns new Memo instances)
 * 
 * Design Philosophy (Memo-First):
 * - Operations focus on individual documents/fields first
 * - More flexible than Feed (which is homogeneous)
 * - Pure NoSQL/MongoDB-style semantics
 * - Support for complex nested structures
 */

#pragma once

#include <pluss/Series.h>
#include <pluss/Note.h>
#include <pluss/Timeline.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <memory>
#include <optional>
#include <functional>

namespace pluss::registry {

// Forward declaration
class Memo;

/**
 * @brief Variant type for Memo value storage  
 * Simplified to use only core pluss types: Series, Note, Timeline, and nested documents
 * All scalar types (int, double, bool, string) are represented as Series or Note
 * Series: encapsulates all numeric types (int, double, bool, numeric arrays)
 * Note: encapsulates all text types (string, Chord, text arrays)
 * Timeline: time-series data
 * Document: nested Memo structures
 */
using MemoValue = std::variant<
    std::monostate,                                               // null
    std::shared_ptr<pluss::Series>,                              // Series (all numeric types)
    std::shared_ptr<pluss::Note>,                                // Note (all text types)
    std::shared_ptr<pluss::Timeline>,                            // Timeline (time-series)
    std::shared_ptr<std::unordered_map<std::string, Memo>>       // nested document
>;

/**
 * @brief Document value type for nested Memo structures
 * Maps field names to Memo values
 */
using DocumentValue = std::unordered_map<std::string, Memo>;

/**
 * @brief Memo - MongoDB-style flexible document (1D)
 * 
 * Memo is analogous to:
 * - MongoDB: Document (BSON document)
 * - Python dict: Flexible key-value store
 * - JSON: Object with mixed types
 * 
 * Unlike Feed (homogeneous), Memo stores heterogeneous data and supports
 * complex nested structures, making it ideal for document-oriented data.
 */
class Memo {
public:
    // ========== Constructors ==========
    
    Memo();  // null memo
    explicit Memo(const pluss::Series& value);
    explicit Memo(const pluss::Note& value);
    explicit Memo(const pluss::Timeline& value);
    explicit Memo(const DocumentValue& value);
    explicit Memo(const std::shared_ptr<DocumentValue>& value);
    
    // Convenience constructors (wrap scalars in Series/Note)
    static Memo from_scalar(double value);
    static Memo from_scalar(int64_t value);
    static Memo from_scalar(bool value);
    static Memo from_text(const std::string& value);
    static Memo from_text(const pluss::Chord& value);
    
    // Named field constructor
    Memo(const MemoValue& value, const std::string& field_name);
    
    // ========== Type Checking ==========
    
    [[nodiscard]] bool is_null() const noexcept;
    [[nodiscard]] bool is_series() const noexcept;
    [[nodiscard]] bool is_note() const noexcept;
    [[nodiscard]] bool is_timeline() const noexcept;
    [[nodiscard]] bool is_document() const noexcept;
    [[nodiscard]] bool is_numeric() const noexcept;  // alias for is_series
    [[nodiscard]] bool is_text() const noexcept;     // alias for is_note
    
    // ========== Type Extraction ==========
    
    [[nodiscard]] pluss::Series as_series() const;
    [[nodiscard]] pluss::Note as_note() const;
    [[nodiscard]] pluss::Timeline as_timeline() const;
    [[nodiscard]] DocumentValue as_document() const;
    
    // ========== Document Operations (for nested Memos) ==========
    
    // Field access
    [[nodiscard]] Memo operator[](const std::string& field) const;
    [[nodiscard]] Memo get_field(const std::string& field) const;
    [[nodiscard]] Memo get_nested(const std::string& path) const;  // Dot notation: "user.address.city"
    
    // Field mutation (returns new Memo)
    [[nodiscard]] Memo set_field(const std::string& field, const Memo& value) const;
    [[nodiscard]] Memo set_nested(const std::string& path, const Memo& value) const;
    [[nodiscard]] Memo unset_field(const std::string& field) const;
    [[nodiscard]] Memo rename_field(const std::string& old_name, const std::string& new_name) const;
    
    // Field queries
    [[nodiscard]] bool has_field(const std::string& field) const;
    [[nodiscard]] std::vector<std::string> field_names() const;
    [[nodiscard]] int64_t num_fields() const;
    
    // ========== MongoDB-style Update Operators ==========
    
    // Field operators
    [[nodiscard]] Memo $set(const std::string& field, const Memo& value) const;
    [[nodiscard]] Memo $unset(const std::string& field) const;
    [[nodiscard]] Memo $rename(const std::string& old_name, const std::string& new_name) const;
    [[nodiscard]] Memo $setOnInsert(const std::string& field, const Memo& value) const;
    
    // Numeric operators (for numeric fields)
    [[nodiscard]] Memo $inc(const std::string& field, double amount) const;
    [[nodiscard]] Memo $mul(const std::string& field, double factor) const;
    [[nodiscard]] Memo $min(const std::string& field, double value) const;
    [[nodiscard]] Memo $max(const std::string& field, double value) const;
    
    // Array operators (for array fields)
    [[nodiscard]] Memo $push(const std::string& field, const Memo& value) const;
    [[nodiscard]] Memo $pop(const std::string& field, int position = 1) const;  // 1 for last, -1 for first
    [[nodiscard]] Memo $pull(const std::string& field, const Memo& value) const;
    [[nodiscard]] Memo $pullAll(const std::string& field, const std::vector<Memo>& values) const;
    [[nodiscard]] Memo $addToSet(const std::string& field, const Memo& value) const;
    
    // ========== Query Operators (return bool for matching) ==========
    
    // Comparison
    [[nodiscard]] bool $eq(const Memo& other) const;
    [[nodiscard]] bool $ne(const Memo& other) const;
    [[nodiscard]] bool $gt(const Memo& other) const;
    [[nodiscard]] bool $gte(const Memo& other) const;
    [[nodiscard]] bool $lt(const Memo& other) const;
    [[nodiscard]] bool $lte(const Memo& other) const;
    
    // Array/Set operators
    [[nodiscard]] bool $in(const std::vector<Memo>& values) const;
    [[nodiscard]] bool $nin(const std::vector<Memo>& values) const;
    
    // Existence
    [[nodiscard]] bool $exists(const std::string& field) const;
    [[nodiscard]] bool $type(const std::string& expected_type) const;
    
    // String operators
    [[nodiscard]] bool $regex(const std::string& pattern) const;
    [[nodiscard]] bool $text(const std::string& search_text) const;
    
    // ========== Array Operations ==========
    
    [[nodiscard]] int64_t size() const;  // For arrays
    [[nodiscard]] Memo at(int64_t index) const;
    [[nodiscard]] Memo slice(int64_t start, int64_t end) const;
    [[nodiscard]] Memo filter(std::function<bool(const Memo&)> predicate) const;
    [[nodiscard]] Memo map(std::function<Memo(const Memo&)> transform) const;
    [[nodiscard]] Memo reduce(std::function<Memo(const Memo&, const Memo&)> reducer, const Memo& initial) const;
    
    // Array queries
    [[nodiscard]] bool contains(const Memo& value) const;
    [[nodiscard]] int64_t index_of(const Memo& value) const;
    
    // ========== Aggregation Operations ==========
    
    [[nodiscard]] Memo sum() const;
    [[nodiscard]] Memo mean() const;
    [[nodiscard]] Memo min() const;
    [[nodiscard]] Memo max() const;
    [[nodiscard]] Memo count() const;
    [[nodiscard]] Memo unique() const;
    
    // ========== String Operations ==========
    
    [[nodiscard]] Memo to_upper() const;
    [[nodiscard]] Memo to_lower() const;
    [[nodiscard]] Memo trim() const;
    [[nodiscard]] Memo concat(const Memo& other) const;
    [[nodiscard]] Memo substr(int64_t start, int64_t length) const;
    [[nodiscard]] bool starts_with(const std::string& prefix) const;
    [[nodiscard]] bool ends_with(const std::string& suffix) const;
    [[nodiscard]] bool str_contains(const std::string& substring) const;
    
    // ========== Conversion Operations ==========
    
    [[nodiscard]] Memo to_series() const;   // Ensure value is Series
    [[nodiscard]] Memo to_note() const;     // Ensure value is Note
    [[nodiscard]] Memo to_timeline() const; // Ensure value is Timeline
    
    // ========== Display and Serialization ==========
    
    [[nodiscard]] std::string to_json() const;
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] std::string type_name() const;
    void display() const;
    void info() const;
    
    // Static constructors
    static Memo from_json(const std::string& json_str);
    static Memo null();
    static Memo array(const std::vector<Memo>& elements);
    static Memo document(const DocumentValue& doc);
    
    // ========== Metadata ==========
    
    [[nodiscard]] const std::string& field_name() const noexcept { return field_name_; }
    void set_field_name(const std::string& name) { field_name_ = name; }
    
    // ========== Operators ==========
    
    bool operator==(const Memo& other) const;
    bool operator!=(const Memo& other) const;
    bool operator<(const Memo& other) const;
    bool operator<=(const Memo& other) const;
    bool operator>(const Memo& other) const;
    bool operator>=(const Memo& other) const;
    
    // Direct access to underlying value (advanced)
    [[nodiscard]] const MemoValue& value() const noexcept { return value_; }

private:
    MemoValue value_;
    std::string field_name_;
    
    // Helper methods
    [[nodiscard]] Memo clone_document() const;
    [[nodiscard]] std::shared_ptr<DocumentValue> get_document_ptr() const;
    void ensure_document() const;
    void ensure_array() const;
    [[nodiscard]] std::string value_to_string() const;
};

} // namespace pluss::registry
