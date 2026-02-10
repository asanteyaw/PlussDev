/**
 * @file stream.h
 * @brief Stream (2D) - MongoDB Collection-like container of Memos
 * 
 * Stream is a collection of Memos (documents), analogous to:
 * - MongoDB: Collection
 * - SQL: Table
 * - Pandas: DataFrame (but document-oriented)
 * 
 * Key Features:
 * - Stores multiple Memos (documents)
 * - MongoDB-style CRUD operations (insert, find, update, delete)
 * - Query builders with filtering, projection, sorting
 * - Aggregation pipeline support
 * - Indexing for efficient queries
 * - Immutable operations (returns new Stream instances)
 * 
 * Design Philosophy (Memo-First):
 * - Operations work on Memos (documents) first
 * - Flexible schema (each document can have different fields)
 * - NoSQL query patterns
 * - Pipeline-based transformations
 */

#pragma once

#include <pluss/datalog/registry/memo/memo.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>

namespace pluss::registry {

// Forward declaration
class Stream;

/**
 * @brief Query filter function type
 * Returns true if Memo matches the query
 */
using QueryFilter = std::function<bool(const Memo&)>;

/**
 * @brief Update function type
 * Transforms a Memo and returns the updated version
 */
using UpdateFunction = std::function<Memo(const Memo&)>;

/**
 * @brief Aggregation stage function type
 * Transforms a Stream as part of an aggregation pipeline
 */
using AggregationStage = std::function<Stream(const Stream&)>;

/**
 * @brief Stream - MongoDB Collection-like container (2D)
 * 
 * Stream stores a collection of Memos (documents) with MongoDB-style operations.
 * Unlike Catalog (which is dictionary-based with homogeneous Feeds), Stream
 * allows completely flexible document structures.
 */
class Stream {
public:
    // ========== Constructors ==========
    
    Stream();
    explicit Stream(const std::string& name);
    explicit Stream(const std::vector<Memo>& memos);
    Stream(const std::string& name, const std::vector<Memo>& memos);
    
    // ========== Metadata ==========
    
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    void set_name(const std::string& n) { name_ = n; }
    [[nodiscard]] int64_t size() const { return static_cast<int64_t>(memos_.size()); }
    [[nodiscard]] bool empty() const { return memos_.empty(); }
    [[nodiscard]] int64_t count() const { return size(); }
    
    // ========== MongoDB-style CRUD Operations ==========
    
    // Insert operations
    [[nodiscard]] Stream insert_one(const Memo& doc) const;
    [[nodiscard]] Stream insert_many(const std::vector<Memo>& docs) const;
    
    // Find operations (returns new Stream with matching documents)
    [[nodiscard]] Stream find(const QueryFilter& query) const;
    [[nodiscard]] Stream find_one(const QueryFilter& query) const;  // Returns Stream with at most 1 doc
    [[nodiscard]] std::optional<Memo> find_one_memo(const QueryFilter& query) const;  // Returns the Memo itself
    
    // Special find variants
    [[nodiscard]] Stream find_all() const;  // Returns all documents
    [[nodiscard]] Stream find_by_field(const std::string& field, const Memo& value) const;
    [[nodiscard]] Stream find_by_id(const std::string& id) const;  // Assumes "_id" field
    
    // Update operations
    [[nodiscard]] Stream update_one(const QueryFilter& query, const UpdateFunction& update) const;
    [[nodiscard]] Stream update_many(const QueryFilter& query, const UpdateFunction& update) const;
    [[nodiscard]] Stream replace_one(const QueryFilter& query, const Memo& replacement) const;
    
    // Delete operations
    [[nodiscard]] Stream delete_one(const QueryFilter& query) const;
    [[nodiscard]] Stream delete_many(const QueryFilter& query) const;
    
    // Upsert (update or insert)
    [[nodiscard]] Stream upsert(const QueryFilter& query, const Memo& doc) const;
    
    // ========== Query Builder Operations ==========
    
    // Projection (select specific fields)
    [[nodiscard]] Stream project(const std::vector<std::string>& fields) const;
    [[nodiscard]] Stream exclude(const std::vector<std::string>& fields) const;
    
    // Sorting
    [[nodiscard]] Stream sort(const std::string& field, bool ascending = true) const;
    [[nodiscard]] Stream sort_by(const std::vector<std::string>& fields, 
                                  const std::vector<bool>& ascending = {}) const;
    
    // Limiting and skipping
    [[nodiscard]] Stream limit(int64_t n) const;
    [[nodiscard]] Stream skip(int64_t n) const;
    [[nodiscard]] Stream slice(int64_t start, int64_t end) const;
    [[nodiscard]] Stream head(int64_t n = 10) const;
    [[nodiscard]] Stream tail(int64_t n = 10) const;
    
    // Distinct values
    [[nodiscard]] Stream distinct(const std::string& field) const;
    [[nodiscard]] std::vector<Memo> distinct_values(const std::string& field) const;
    
    // ========== Aggregation Pipeline ==========
    
    // Pipeline stages
    [[nodiscard]] Stream $match(const QueryFilter& query) const;
    [[nodiscard]] Stream $project(const std::vector<std::string>& fields) const;
    [[nodiscard]] Stream $sort(const std::string& field, bool ascending = true) const;
    [[nodiscard]] Stream $limit(int64_t n) const;
    [[nodiscard]] Stream $skip(int64_t n) const;
    [[nodiscard]] Stream $unwind(const std::string& field) const;  // Unwind array field
    [[nodiscard]] Stream $group(const std::string& by_field, 
                                 const std::string& agg_func = "count") const;
    [[nodiscard]] Stream $lookup(const Stream& foreign, 
                                  const std::string& local_field,
                                  const std::string& foreign_field, 
                                  const std::string& as) const;
    
    // Execute pipeline
    [[nodiscard]] Stream aggregate(const std::vector<AggregationStage>& pipeline) const;
    
    // ========== Aggregation Operations ==========
    
    [[nodiscard]] double sum(const std::string& field) const;
    [[nodiscard]] double mean(const std::string& field) const;
    [[nodiscard]] double min(const std::string& field) const;
    [[nodiscard]] double max(const std::string& field) const;
    [[nodiscard]] int64_t count_field(const std::string& field) const;  // Count non-null values
    
    // Grouping (simplified)
    [[nodiscard]] std::unordered_map<std::string, Stream> group_by(const std::string& field) const;
    
    // ========== Indexing and Performance ==========
    
    // Create index for faster queries (internal optimization)
    void create_index(const std::string& field);
    void drop_index(const std::string& field);
    [[nodiscard]] bool has_index(const std::string& field) const;
    [[nodiscard]] std::vector<std::string> list_indexes() const;
    
    // ========== Document Access ==========
    
    [[nodiscard]] const Memo& at(int64_t index) const;
    [[nodiscard]] Memo& at(int64_t index);
    [[nodiscard]] const Memo& operator[](int64_t index) const;
    [[nodiscard]] std::vector<Memo> to_vector() const;
    
    // Iterator support
    auto begin() const { return memos_.begin(); }
    auto end() const { return memos_.end(); }
    auto begin() { return memos_.begin(); }
    auto end() { return memos_.end(); }
    
    // ========== Functional Operations ==========
    
    [[nodiscard]] Stream filter(std::function<bool(const Memo&)> predicate) const;
    [[nodiscard]] Stream map(std::function<Memo(const Memo&)> transform) const;
    [[nodiscard]] Memo reduce(std::function<Memo(const Memo&, const Memo&)> reducer, 
                               const Memo& initial) const;
    [[nodiscard]] bool all_of(std::function<bool(const Memo&)> predicate) const;
    [[nodiscard]] bool any_of(std::function<bool(const Memo&)> predicate) const;
    [[nodiscard]] bool none_of(std::function<bool(const Memo&)> predicate) const;
    
    // ========== Joining Streams ==========
    
    [[nodiscard]] Stream merge(const Stream& other) const;  // Union
    [[nodiscard]] Stream inner_join(const Stream& other, 
                                     const std::string& on_field) const;
    [[nodiscard]] Stream left_join(const Stream& other,
                                    const std::string& on_field) const;
    [[nodiscard]] Stream cross_join(const Stream& other) const;
    
    // ========== Statistical Operations ==========
    
    [[nodiscard]] Stream sample(int64_t n) const;
    [[nodiscard]] Stream sample_n(int64_t n, bool with_replacement = false) const;
    [[nodiscard]] Stream shuffle() const;
    
    // ========== Field Operations ==========
    
    // Get all unique field names across all documents
    [[nodiscard]] std::vector<std::string> all_fields() const;
    
    // Check if field exists in any document
    [[nodiscard]] bool has_field(const std::string& field) const;
    
    // Add computed field to all documents
    [[nodiscard]] Stream add_field(const std::string& field, 
                                    std::function<Memo(const Memo&)> compute) const;
    [[nodiscard]] Stream drop_field(const std::string& field) const;
    [[nodiscard]] Stream rename_field(const std::string& old_name, 
                                       const std::string& new_name) const;
    
    // ========== I/O Operations ==========
    
    [[nodiscard]] std::string to_json() const;
    [[nodiscard]] std::string to_json_lines() const;  // JSONL format
    void to_json_file(const std::string& filename) const;
    void to_jsonl_file(const std::string& filename) const;
    
    static Stream from_json(const std::string& json_str);
    static Stream from_json_lines(const std::string& jsonl_str);
    static Stream from_json_file(const std::string& filename);
    static Stream from_jsonl_file(const std::string& filename);
    
    // CSV I/O (for compatible documents)
    void to_csv(const std::string& filename) const;
    static Stream from_csv(const std::string& filename);
    
    // ========== Display and Info ==========
    
    void display(int64_t max_docs = 10) const;
    void info() const;
    [[nodiscard]] std::string to_string() const;
    void summary() const;
    
    // ========== Validation ==========
    
    // Schema validation (optional)
    using SchemaValidator = std::function<bool(const Memo&)>;
    void set_validator(const SchemaValidator& validator);
    [[nodiscard]] bool validate() const;  // Check all documents against validator
    [[nodiscard]] Stream validate_and_filter() const;  // Remove invalid documents
    
    // ========== Operators ==========
    
    bool operator==(const Stream& other) const;
    bool operator!=(const Stream& other) const;
    
    // Direct access (advanced)
    [[nodiscard]] const std::vector<Memo>& memos() const noexcept { return memos_; }

private:
    std::vector<Memo> memos_;
    std::string name_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int64_t>>> indexes_;
    std::optional<SchemaValidator> validator_;
    
    // Helper methods
    [[nodiscard]] std::vector<int64_t> find_indices(const QueryFilter& query) const;
    [[nodiscard]] Stream from_indices(const std::vector<int64_t>& indices) const;
    void rebuild_index(const std::string& field);
};

} // namespace pluss::registry
