#pragma once

#include <string>
#include <vector>
#include <functional>

namespace pluss {

// Forward declarations
class Thread;
class DataFrame;

/**
 * @brief Configuration for parallel CSV reading
 */
struct ParallelCSVConfig {
    size_t chunk_size = 100000;        // Rows per chunk
    size_t num_threads = 0;            // 0 = auto-detect
    bool use_memory_mapping = true;    // Use mmap for large files
    bool skip_header = false;          // Skip first row
    char delimiter = ',';
    char quote_char = '"';
    bool trim_whitespace = true;
    size_t buffer_size = 8192;         // I/O buffer size
};

/**
 * @brief High-performance parallel CSV reader
 * 
 * Features:
 * - Multi-threaded chunk-based parsing
 * - Memory-mapped I/O for large files
 * - SIMD-optimized string operations where available
 * - Progress callbacks for monitoring
 */
class ParallelCSVReader {
public:
    using ProgressCallback = std::function<void(size_t rows_processed, size_t total_rows)>;
    
    /**
     * @brief Construct with configuration
     */
    explicit ParallelCSVReader(const ParallelCSVConfig& config = ParallelCSVConfig());
    
    /**
     * @brief Read CSV file in parallel
     * @param filepath Path to CSV file
     * @param progress_callback Optional callback for progress updates
     * @return Vector of rows (each row is vector of strings)
     */
    std::vector<std::vector<std::string>> read(
        const std::string& filepath,
        ProgressCallback progress_callback = nullptr
    );
    
    /**
     * @brief Read CSV file and convert columns to numeric types in parallel
     * @param filepath Path to CSV file
     * @param numeric_columns Indices of columns to convert to numbers
     * @param progress_callback Optional callback for progress updates
     * @return Pair of string data and numeric data
     */
    std::pair<std::vector<std::vector<std::string>>, std::vector<std::vector<double>>> 
    read_mixed(
        const std::string& filepath,
        const std::vector<size_t>& numeric_columns,
        ProgressCallback progress_callback = nullptr
    );
    
    /**
     * @brief Set number of threads (0 = auto)
     */
    void set_num_threads(size_t n);
    
    /**
     * @brief Set chunk size for parallel processing
     */
    void set_chunk_size(size_t size);
    
    /**
     * @brief Get detected file size in bytes
     */
    size_t get_file_size() const { return file_size_; }
    
    /**
     * @brief Get number of rows in last read
     */
    size_t get_row_count() const { return row_count_; }

private:
    ParallelCSVConfig config_;
    size_t file_size_;
    size_t row_count_;
    
    // Internal parsing methods
    std::vector<std::vector<std::string>> parse_chunk_sequential(
        const char* data, 
        size_t start, 
        size_t end
    );
    
    std::vector<std::vector<std::string>> parse_chunk_parallel(
        const char* data,
        size_t file_size
    );
    
    void parse_line(
        const char* line_start,
        const char* line_end,
        std::vector<std::string>& fields
    );
    
    size_t detect_optimal_threads() const;
    std::vector<size_t> compute_chunk_boundaries(const char* data, size_t file_size);
};

/**
 * @brief Optimized DataFrame builder with parallel column conversion
 */
class ParallelDataFrameBuilder {
public:
    /**
     * @brief Build DataFrame from CSV data with parallel column processing
     * @param csv_data Raw CSV string data
     * @param column_names Column names
     * @param num_threads Number of threads (0 = auto)
     * @return DataFrame object
     */
    static DataFrame build_from_csv(
        const std::vector<std::vector<std::string>>& csv_data,
        const std::vector<std::string>& column_names,
        size_t num_threads = 0
    );
    
    /**
     * @brief Build DataFrame with specified column indices (avoids full parse)
     * @param csv_data Raw CSV data
     * @param column_indices Indices of columns to extract
     * @param column_names Names for extracted columns
     * @param num_threads Number of threads (0 = auto)
     * @return DataFrame object
     */
    static DataFrame build_from_csv_columns(
        const std::vector<std::vector<std::string>>& csv_data,
        const std::vector<size_t>& column_indices,
        const std::vector<std::string>& column_names,
        size_t num_threads = 0
    );

private:
    static Thread build_column_parallel(
        const std::vector<std::vector<std::string>>& csv_data,
        size_t col_idx,
        size_t num_threads
    );
};

} // namespace pluss
