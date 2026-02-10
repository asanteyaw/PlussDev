/**
 * @file bundle.h
 * @brief Bundle (3D) - MongoDB Database-like container of Streams
 * 
 * Bundle is a collection of Streams (collections), analogous to:
 * - MongoDB: Database
 * - SQL: Database (schema)
 * - Pandas: Dict of DataFrames
 * 
 * Key Features:
 * - Stores multiple named Streams (collections)
 * - Database-level operations (create, drop, list collections)
 * - Cross-collection operations (joins, lookups, unions)
 * - Transaction-like batch operations
 * - Database-wide aggregations and queries
 * - I/O operations (save/load entire database)
 * - Immutable operations (returns new Bundle instances)
 * 
 * Design Philosophy (Memo-First):
 * - Collections (Streams) work on documents (Memos)
 * - Flexible multi-collection operations
 * - NoSQL database patterns
 * - Pipeline-based transformations across collections
 */

#pragma once

#include <pluss/datalog/registry/stream/stream.h>
#include <pluss/datalog/registry/memo/memo.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <optional>

namespace pluss::registry {

/**
 * @brief Bundle - MongoDB Database-like container (3D)
 * 
 * Bundle stores multiple named Streams (collections) with database-level operations.
 * Unlike Archive (which is dictionary-based with homogeneous Catalogs), Bundle
 * allows completely flexible multi-collection document operations.
 */
class Bundle {
public:
    // ========== Constructors ==========
    
    Bundle();
    explicit Bundle(const std::string& name);
    Bundle(const std::string& name, const std::unordered_map<std::string, Stream>& streams);
    
    // ========== Metadata ==========
    
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    void set_name(const std::string& n) { name_ = n; }
    [[nodiscard]] int64_t size() const { return static_cast<int64_t>(streams_.size()); }
    [[nodiscard]] bool empty() const { return streams_.empty(); }
    [[nodiscard]] int64_t count() const { return size(); }
    [[nodiscard]] int64_t total_documents() const;  // Count across all streams
    
    // ========== MongoDB-style Collection Management ==========
    
    // Create/insert collections
    [[nodiscard]] Bundle create_stream(const std::string& stream_name) const;
    [[nodiscard]] Bundle create_stream(const std::string& stream_name, const Stream& stream) const;
    [[nodiscard]] Bundle add_stream(const std::string& stream_name, const Stream& stream) const;
    [[nodiscard]] Bundle insert_stream(const std::string& stream_name, const Stream& stream) const;
    
    // Drop collections
    [[nodiscard]] Bundle drop_stream(const std::string& stream_name) const;
    [[nodiscard]] Bundle drop_streams(const std::vector<std::string>& stream_names) const;
    [[nodiscard]] Bundle clear() const;  // Remove all streams
    
    // Rename collections
    [[nodiscard]] Bundle rename_stream(const std::string& old_name, 
                                        const std::string& new_name) const;
    
    // Check existence
    [[nodiscard]] bool has_stream(const std::string& stream_name) const;
    [[nodiscard]] std::vector<std::string> list_streams() const;
    [[nodiscard]] std::vector<std::string> stream_names() const;
    
    // ========== Stream Access ==========
    
    [[nodiscard]] const Stream& get_stream(const std::string& stream_name) const;
    [[nodiscard]] Stream& get_stream(const std::string& stream_name);
    [[nodiscard]] std::optional<Stream> try_get_stream(const std::string& stream_name) const;
    [[nodiscard]] const Stream& operator[](const std::string& stream_name) const;
    [[nodiscard]] Stream& operator[](const std::string& stream_name);
    
    // ========== Document Operations (across streams) ==========
    
    // Insert documents into a specific stream
    [[nodiscard]] Bundle insert_into(const std::string& stream_name, const Memo& doc) const;
    [[nodiscard]] Bundle insert_many_into(const std::string& stream_name, 
                                           const std::vector<Memo>& docs) const;
    
    // Update documents in a specific stream
    [[nodiscard]] Bundle update_in(const std::string& stream_name,
                                    const QueryFilter& query,
                                    const UpdateFunction& update) const;
    
    // Delete documents from a specific stream
    [[nodiscard]] Bundle delete_from(const std::string& stream_name,
                                      const QueryFilter& query) const;
    
    // Find across all streams
    [[nodiscard]] std::vector<Memo> find_all(const QueryFilter& query) const;
    [[nodiscard]] std::unordered_map<std::string, Stream> find_in_all(const QueryFilter& query) const;
    
    // ========== Cross-Stream Operations ==========
    
    // Join two streams
    [[nodiscard]] Stream join(const std::string& left_stream,
                               const std::string& right_stream,
                               const std::string& on_field,
                               const std::string& join_type = "inner") const;
    
    // Lookup (similar to MongoDB $lookup)
    [[nodiscard]] Bundle lookup(const std::string& from_stream,
                                 const std::string& to_stream,
                                 const std::string& local_field,
                                 const std::string& foreign_field,
                                 const std::string& as_field) const;
    
    // Union of multiple streams
    [[nodiscard]] Stream union_streams(const std::vector<std::string>& stream_names) const;
    [[nodiscard]] Stream union_all() const;  // Union of all streams
    
    // Intersect documents across streams
    [[nodiscard]] Stream intersect(const std::string& stream1,
                                    const std::string& stream2,
                                    const std::string& on_field) const;
    
    // Difference
    [[nodiscard]] Stream difference(const std::string& stream1,
                                     const std::string& stream2,
                                     const std::string& on_field) const;
    
    // ========== Aggregation Operations ==========
    
    // Database-wide aggregations
    [[nodiscard]] double sum_all(const std::string& field) const;
    [[nodiscard]] double mean_all(const std::string& field) const;
    [[nodiscard]] double min_all(const std::string& field) const;
    [[nodiscard]] double max_all(const std::string& field) const;
    [[nodiscard]] int64_t count_all() const;
    [[nodiscard]] int64_t count_in(const std::string& stream_name) const;
    
    // Stream-level aggregations (returns map of stream_name -> result)
    [[nodiscard]] std::unordered_map<std::string, double> sum_by_stream(const std::string& field) const;
    [[nodiscard]] std::unordered_map<std::string, double> mean_by_stream(const std::string& field) const;
    [[nodiscard]] std::unordered_map<std::string, int64_t> count_by_stream() const;
    
    // ========== Functional Operations ==========
    
    // Apply function to all streams
    [[nodiscard]] Bundle map_streams(std::function<Stream(const Stream&)> transform) const;
    [[nodiscard]] Bundle filter_streams(std::function<bool(const std::string&, const Stream&)> predicate) const;
    
    // Apply function to specific streams
    [[nodiscard]] Bundle update_stream(const std::string& stream_name,
                                        std::function<Stream(const Stream&)> transform) const;
    
    // ========== Batch Operations ==========
    
    // Transaction-like batch operations (all or nothing conceptually)
    using BatchOperation = std::function<Bundle(const Bundle&)>;
    [[nodiscard]] Bundle batch(const std::vector<BatchOperation>& operations) const;
    
    // Bulk insert across multiple streams
    [[nodiscard]] Bundle bulk_insert(const std::unordered_map<std::string, std::vector<Memo>>& docs) const;
    
    // ========== Query Builder Operations ==========
    
    // Filter all streams
    [[nodiscard]] Bundle where(std::function<bool(const std::string&, const Stream&)> predicate) const;
    
    // Select specific streams
    [[nodiscard]] Bundle select_streams(const std::vector<std::string>& stream_names) const;
    [[nodiscard]] Bundle exclude_streams(const std::vector<std::string>& stream_names) const;
    
    // Sort streams by name or size
    [[nodiscard]] std::vector<std::string> sorted_stream_names(bool by_size = false, 
                                                                 bool ascending = true) const;
    
    // ========== Database Statistics ==========
    
    struct StreamStats {
        std::string name;
        int64_t doc_count;
        std::vector<std::string> fields;
        int64_t estimated_size_bytes;
    };
    
    [[nodiscard]] StreamStats stream_stats(const std::string& stream_name) const;
    [[nodiscard]] std::unordered_map<std::string, StreamStats> all_stream_stats() const;
    [[nodiscard]] int64_t estimated_size_bytes() const;
    
    // ========== I/O Operations ==========
    
    // JSON I/O (nested structure)
    [[nodiscard]] std::string to_json() const;
    void to_json_file(const std::string& filename) const;
    static Bundle from_json(const std::string& json_str);
    static Bundle from_json_file(const std::string& filename);
    
    // Directory-based I/O (each stream as separate file)
    void to_directory(const std::string& dir_path) const;
    static Bundle from_directory(const std::string& dir_path);
    
    // JSONL format (one stream per line)
    [[nodiscard]] std::string to_jsonl() const;
    void to_jsonl_file(const std::string& filename) const;
    static Bundle from_jsonl(const std::string& jsonl_str);
    static Bundle from_jsonl_file(const std::string& filename);
    
    // Binary format (efficient storage)
    void to_binary(const std::string& filename) const;
    static Bundle from_binary(const std::string& filename);
    
    // Export individual streams
    void export_stream_json(const std::string& stream_name, 
                             const std::string& filename) const;
    void export_stream_csv(const std::string& stream_name,
                            const std::string& filename) const;
    
    // Import streams from individual files
    [[nodiscard]] Bundle import_stream_json(const std::string& stream_name,
                                             const std::string& filename) const;
    [[nodiscard]] Bundle import_stream_csv(const std::string& stream_name,
                                            const std::string& filename) const;
    
    // ========== Schema Management ==========
    
    // Get all unique fields across a stream
    [[nodiscard]] std::vector<std::string> stream_fields(const std::string& stream_name) const;
    
    // Get all unique fields across all streams
    [[nodiscard]] std::unordered_map<std::string, std::vector<std::string>> all_fields() const;
    
    // Schema validation (optional)
    using StreamValidator = std::function<bool(const Stream&)>;
    void set_stream_validator(const std::string& stream_name, const StreamValidator& validator);
    [[nodiscard]] bool validate_stream(const std::string& stream_name) const;
    [[nodiscard]] std::unordered_map<std::string, bool> validate_all() const;
    
    // ========== Display and Info ==========
    
    void display(int64_t max_streams = 10, int64_t max_docs_per_stream = 5) const;
    void info() const;
    [[nodiscard]] std::string to_string() const;
    void summary() const;
    
    // Stream-specific info
    void stream_info(const std::string& stream_name) const;
    
    // ========== Merge and Clone Operations ==========
    
    // Merge two bundles
    [[nodiscard]] Bundle merge(const Bundle& other, 
                                const std::string& conflict_strategy = "error") const;
    // conflict_strategy: "error", "keep_left", "keep_right", "merge_streams"
    
    // Clone with modifications
    [[nodiscard]] Bundle clone() const;
    [[nodiscard]] Bundle shallow_copy() const;
    
    // ========== Advanced Operations ==========
    
    // Graph-like operations (references between documents)
    [[nodiscard]] Stream resolve_references(const std::string& stream_name,
                                              const std::string& ref_field,
                                              const std::string& target_stream) const;
    
    // Denormalize (embed related documents)
    [[nodiscard]] Stream denormalize(const std::string& stream_name,
                                      const std::string& ref_field,
                                      const std::string& target_stream,
                                      const std::string& as_field) const;
    
    // Migration helper (rename/restructure)
    [[nodiscard]] Bundle migrate(const std::unordered_map<std::string, std::string>& rename_map) const;
    
    // ========== Operators ==========
    
    bool operator==(const Bundle& other) const;
    bool operator!=(const Bundle& other) const;
    
    // Direct access (advanced)
    [[nodiscard]] const std::unordered_map<std::string, Stream>& streams() const noexcept { 
        return streams_; 
    }
    
    // Iterator support
    auto begin() const { return streams_.begin(); }
    auto end() const { return streams_.end(); }
    auto begin() { return streams_.begin(); }
    auto end() { return streams_.end(); }

private:
    std::unordered_map<std::string, Stream> streams_;
    std::string name_;
    std::unordered_map<std::string, StreamValidator> validators_;
    
    // Helper methods
    void validate_stream_name(const std::string& stream_name) const;
    [[nodiscard]] Stream merge_streams(const Stream& left, const Stream& right,
                                        const std::string& strategy) const;
};

} // namespace pluss::registry
