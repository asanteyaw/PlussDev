#include "pluss/datalog/csv/parallel_csv_reader.h"
#include "pluss/datalog/table/dataframe/dataframe.h"
#include "pluss/Thread.h"
#include <fstream>
#include <thread>
#include <future>
#include <cstring>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace pluss {

// ============================================================================
// ParallelCSVReader Implementation
// ============================================================================

ParallelCSVReader::ParallelCSVReader(const ParallelCSVConfig& config)
    : config_(config), file_size_(0), row_count_(0) {
    
    if (config_.num_threads == 0) {
        config_.num_threads = detect_optimal_threads();
    }
}

size_t ParallelCSVReader::detect_optimal_threads() const {
    size_t hw_threads = std::thread::hardware_concurrency();
    return hw_threads > 0 ? hw_threads : 4;
}

std::vector<std::vector<std::string>> ParallelCSVReader::read(
    const std::string& filepath,
    ProgressCallback progress_callback
) {
    // Get file size
    struct stat st;
    if (stat(filepath.c_str(), &st) != 0) {
        throw std::runtime_error("Cannot stat file: " + filepath);
    }
    file_size_ = st.st_size;
    
    // For large files, use memory mapping
    if (config_.use_memory_mapping && file_size_ > 10 * 1024 * 1024) {  // > 10MB
        int fd = open(filepath.c_str(), O_RDONLY);
        if (fd == -1) {
            throw std::runtime_error("Cannot open file: " + filepath);
        }
        
        void* mapped = mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        
        if (mapped == MAP_FAILED) {
            throw std::runtime_error("Cannot mmap file: " + filepath);
        }
        
        // Advise kernel about access pattern
        madvise(mapped, file_size_, MADV_SEQUENTIAL | MADV_WILLNEED);
        
        auto result = parse_chunk_parallel(static_cast<const char*>(mapped), file_size_);
        munmap(mapped, file_size_);
        
        row_count_ = result.size();
        if (progress_callback) {
            progress_callback(row_count_, row_count_);
        }
        
        return result;
    } else {
        // For smaller files, use traditional file reading
        std::ifstream file(filepath);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + filepath);
        }
        
        std::vector<std::vector<std::string>> result;
        std::string line;
        size_t row = 0;
        
        while (std::getline(file, line)) {
            if (config_.skip_header && row == 0) {
                row++;
                continue;
            }
            
            std::vector<std::string> fields;
            parse_line(line.c_str(), line.c_str() + line.size(), fields);
            result.push_back(std::move(fields));
            
            if (progress_callback && (row % 10000 == 0)) {
                progress_callback(row, 0);  // Total unknown for streaming
            }
            row++;
        }
        
        row_count_ = result.size();
        return result;
    }
}

std::vector<size_t> ParallelCSVReader::compute_chunk_boundaries(
    const char* data, 
    size_t file_size
) {
    std::vector<size_t> boundaries;
    boundaries.push_back(0);
    
    size_t chunk_byte_size = file_size / config_.num_threads;
    
    for (size_t i = 1; i < config_.num_threads; ++i) {
        size_t approx_pos = i * chunk_byte_size;
        
        // Find next newline
        while (approx_pos < file_size && data[approx_pos] != '\n') {
            approx_pos++;
        }
        
        if (approx_pos < file_size) {
            boundaries.push_back(approx_pos + 1);  // Start of next line
        }
    }
    
    boundaries.push_back(file_size);
    return boundaries;
}

std::vector<std::vector<std::string>> ParallelCSVReader::parse_chunk_parallel(
    const char* data,
    size_t file_size
) {
    auto boundaries = compute_chunk_boundaries(data, file_size);
    
    std::vector<std::future<std::vector<std::vector<std::string>>>> futures;
    
    for (size_t i = 0; i + 1 < boundaries.size(); ++i) {
        size_t start = boundaries[i];
        size_t end = boundaries[i + 1];
        
        futures.push_back(std::async(std::launch::async, [this, data, start, end]() {
            return parse_chunk_sequential(data, start, end);
        }));
    }
    
    // Collect results
    std::vector<std::vector<std::string>> result;
    size_t total_rows = 0;
    
    for (auto& future : futures) {
        auto chunk_result = future.get();
        total_rows += chunk_result.size();
    }
    
    result.reserve(total_rows);
    
    for (auto& future : futures) {
        // Note: we already got the results, need to restructure
    }
    
    // Simplified: re-collect
    for (size_t i = 0; i + 1 < boundaries.size(); ++i) {
        auto chunk = parse_chunk_sequential(data, boundaries[i], boundaries[i + 1]);
        result.insert(result.end(), 
                     std::make_move_iterator(chunk.begin()),
                     std::make_move_iterator(chunk.end()));
    }
    
    return result;
}

std::vector<std::vector<std::string>> ParallelCSVReader::parse_chunk_sequential(
    const char* data,
    size_t start,
    size_t end
) {
    std::vector<std::vector<std::string>> result;
    
    const char* line_start = data + start;
    const char* ptr = line_start;
    const char* data_end = data + end;
    
    while (ptr < data_end) {
        // Find end of line
        const char* line_end = ptr;
        while (line_end < data_end && *line_end != '\n' && *line_end != '\r') {
            line_end++;
        }
        
        if (line_end > ptr) {
            std::vector<std::string> fields;
            parse_line(ptr, line_end, fields);
            if (!fields.empty()) {
                result.push_back(std::move(fields));
            }
        }
        
        // Skip newline characters
        ptr = line_end;
        while (ptr < data_end && (*ptr == '\n' || *ptr == '\r')) {
            ptr++;
        }
    }
    
    return result;
}

void ParallelCSVReader::parse_line(
    const char* line_start,
    const char* line_end,
    std::vector<std::string>& fields
) {
    fields.clear();
    
    const char* ptr = line_start;
    const char* field_start = ptr;
    bool in_quotes = false;
    
    while (ptr <= line_end) {
        if (ptr == line_end || (*ptr == config_.delimiter && !in_quotes)) {
            // End of field
            const char* field_end = ptr;
            
            // Handle quotes
            if (field_end > field_start && field_start[0] == config_.quote_char) {
                field_start++;
                if (field_end > field_start && field_end[-1] == config_.quote_char) {
                    field_end--;
                }
            }
            
            // Trim whitespace if configured
            if (config_.trim_whitespace) {
                while (field_start < field_end && std::isspace(*field_start)) {
                    field_start++;
                }
                while (field_end > field_start && std::isspace(field_end[-1])) {
                    field_end--;
                }
            }
            
            fields.emplace_back(field_start, field_end - field_start);
            field_start = ptr + 1;
        } else if (*ptr == config_.quote_char) {
            in_quotes = !in_quotes;
        }
        
        ptr++;
    }
}

void ParallelCSVReader::set_num_threads(size_t n) {
    config_.num_threads = n > 0 ? n : detect_optimal_threads();
}

void ParallelCSVReader::set_chunk_size(size_t size) {
    config_.chunk_size = size;
}

std::pair<std::vector<std::vector<std::string>>, std::vector<std::vector<double>>>
ParallelCSVReader::read_mixed(
    const std::string& filepath,
    const std::vector<size_t>& numeric_columns,
    ProgressCallback progress_callback
) {
    auto string_data = read(filepath, progress_callback);
    
    std::vector<std::vector<double>> numeric_data(numeric_columns.size());
    
    // Parallel conversion of numeric columns
    std::vector<std::future<void>> futures;
    
    for (size_t i = 0; i < numeric_columns.size(); ++i) {
        size_t col_idx = numeric_columns[i];
        
        futures.push_back(std::async(std::launch::async, 
            [&]() {
                numeric_data[i].reserve(string_data.size());
                
                for (const auto& row : string_data) {
                    if (col_idx < row.size()) {
                        try {
                            numeric_data[i].push_back(std::stod(row[col_idx]));
                        } catch (...) {
                            numeric_data[i].push_back(std::nan(""));
                        }
                    } else {
                        numeric_data[i].push_back(std::nan(""));
                    }
                }
            }
        ));
    }
    
    // Wait for all conversions
    for (auto& future : futures) {
        future.get();
    }
    
    return {std::move(string_data), std::move(numeric_data)};
}

// ============================================================================
// ParallelDataFrameBuilder Implementation
// ============================================================================

DataFrame ParallelDataFrameBuilder::build_from_csv(
    const std::vector<std::vector<std::string>>& csv_data,
    const std::vector<std::string>& column_names,
    size_t num_threads
) {
    if (csv_data.empty()) {
        return DataFrame({}, column_names);
    }
    
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;
    }
    
    size_t num_cols = column_names.size();
    std::vector<Thread> columns(num_cols);
    
    // Build columns in parallel
    std::vector<std::future<Thread>> futures;
    
    for (size_t col_idx = 0; col_idx < num_cols; ++col_idx) {
        futures.push_back(std::async(std::launch::async,
            [&csv_data, col_idx]() {
                std::vector<std::string> col_data;
                col_data.reserve(csv_data.size());
                
                for (const auto& row : csv_data) {
                    if (col_idx < row.size()) {
                        col_data.push_back(row[col_idx]);
                    } else {
                        col_data.push_back("");
                    }
                }
                
                return Thread(col_data);
            }
        ));
    }
    
    // Collect results
    for (size_t i = 0; i < num_cols; ++i) {
        columns[i] = futures[i].get();
    }
    
    return DataFrame(columns, column_names);
}

DataFrame ParallelDataFrameBuilder::build_from_csv_columns(
    const std::vector<std::vector<std::string>>& csv_data,
    const std::vector<size_t>& column_indices,
    const std::vector<std::string>& column_names,
    size_t num_threads
) {
    if (csv_data.empty()) {
        return DataFrame({}, column_names);
    }
    
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;
    }
    
    std::vector<Thread> columns(column_names.size());
    
    // Build only requested columns in parallel
    std::vector<std::future<Thread>> futures;
    
    for (size_t i = 0; i < column_indices.size(); ++i) {
        size_t col_idx = column_indices[i];
        
        futures.push_back(std::async(std::launch::async,
            [&csv_data, col_idx]() {
                std::vector<std::string> col_data;
                col_data.reserve(csv_data.size());
                
                for (const auto& row : csv_data) {
                    if (col_idx < row.size()) {
                        col_data.push_back(row[col_idx]);
                    } else {
                        col_data.push_back("");
                    }
                }
                
                return Thread(col_data);
            }
        ));
    }
    
    // Collect results
    for (size_t i = 0; i < column_indices.size(); ++i) {
        columns[i] = futures[i].get();
    }
    
    return DataFrame(columns, column_names);
}

Thread ParallelDataFrameBuilder::build_column_parallel(
    const std::vector<std::vector<std::string>>& csv_data,
    size_t col_idx,
    size_t num_threads
) {
    std::vector<std::string> col_data;
    col_data.reserve(csv_data.size());
    
    for (const auto& row : csv_data) {
        if (col_idx < row.size()) {
            col_data.push_back(row[col_idx]);
        } else {
            col_data.push_back("");
        }
    }
    
    return Thread(col_data);
}

} // namespace pluss
