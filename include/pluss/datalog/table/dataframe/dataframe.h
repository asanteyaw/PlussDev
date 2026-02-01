#pragma once

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <pluss/pluss.h>

namespace pluss::table {

class DataFrame {
public:
    DataFrame();
    explicit DataFrame(const std::unordered_map<std::string, pluss::Series>& data);
    DataFrame(const std::unordered_map<std::string, pluss::Series>& data, const std::vector<std::string>& column_order);
    DataFrame(const DataFrame& other);
    DataFrame& operator=(const DataFrame& other);
    DataFrame(DataFrame&& other) noexcept;
    DataFrame& operator=(DataFrame&& other) noexcept;
    ~DataFrame();

    // Loader
    static std::shared_ptr<DataFrame> load(const std::string& format, const std::string& filename);
    static std::shared_ptr<DataFrame> read_csv(const std::string& filename);

    // Get column names
    [[nodiscard]] std::vector<std::string> columns() const;

    // Conditional filtering (mask should be a boolean Series)
    [[nodiscard]] std::shared_ptr<DataFrame> loc(const pluss::Series& condition) const;

    // Index-based selection
    [[nodiscard]] std::shared_ptr<DataFrame> iloc(int64_t row_start, int64_t row_end) const;
    [[nodiscard]] std::unordered_map<std::string, pluss::Series> iloc(int64_t row) const;

    // Move to device (CPU/GPU)
    [[nodiscard]] std::shared_ptr<DataFrame> to(const torch::Device& device) const;

    [[nodiscard]] std::vector<pluss::table::Thread> values() const;

    [[nodiscard]] std::shared_ptr<DataFrame> T() const;

    // Display methods
    void head(int64_t n = 5) const;
    void tail(int64_t n = 5) const;
    void info() const;
    void describe() const;

    // Single column selection (returns Series)
    [[nodiscard]] pluss::table::Thread get_col(const std::string& col_name) const;

    // Multiple column selection (returns DataFrame)
    [[nodiscard]] std::shared_ptr<DataFrame> get_cols(const std::vector<std::string>& column_names) const;

    // single column creation
    [[nodiscard]] std::shared_ptr<DataFrame> set_col(const std::string& col_name, const pluss::table::Thread& values) const;

    // multiple column creation
    [[nodiscard]] std::shared_ptr<DataFrame> set_cols(const std::vector<std::string>& column_names, const std::vector<pluss::table::Thread>& values) const;

    // Save CSV
    void to_csv(const std::string& filename) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    [[nodiscard]] std::shared_ptr<DataFrame> select_many(const std::vector<std::string>& column_names) const;
    [[nodiscard]] pluss::Series select_one(const std::string& col_name) const;
    [[nodiscard]] std::shared_ptr<DataFrame> set_column(const std::string& col_name, const pluss::table::Thread& values) const;
    [[nodiscard]] std::shared_ptr<DataFrame> set_columns(const std::vector<std::string>& column_names, const std::vector<pluss::table::Thread>& values) const;
};

} // namespace pluss::table





