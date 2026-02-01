#include "table/dataframe.h"
#include "table/io/loader.h"

#include <pluss/pluss.h>
#include <iostream>              // Input/output
#include <fstream>               // File handling
// Clean, PIMPL-based DataFrame implementation using pluss::Series and Thread for columns
#include "table/dataframe.h"
#include "table/io/loader.h"
#include "table/thread/thread.h"

#include <pluss/pluss.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <string>

namespace pluss::table {

// Implementations

std::shared_ptr<DataFrame> DataFrame::load(const std::string& format, const std::string& filename) {
    return io::load(format, filename);
}

std::shared_ptr<DataFrame> DataFrame::read_csv(const std::string& filename) {
    return io::load_csv(filename);
}

void DataFrame::head(int64_t n) const {
    if (impl_->data.empty()) { std::cout << "Empty DataFrame\n"; return; }

    const int col_width = 12;
    int terminal_width = 200;
    if (std::getenv("COLUMNS") != nullptr) terminal_width = std::stoi(std::getenv("COLUMNS"));
    int max_cols = terminal_width / col_width;

    auto col_names = this->columns();
    for (size_t i = 0; i < std::min(col_names.size(), static_cast<size_t>(max_cols)); ++i) std::cout << std::setw(col_width) << std::left << col_names[i] << " ";
    std::cout << "\n" << std::string(col_width * std::min(col_names.size(), static_cast<size_t>(max_cols)), '-') << "\n";

    int64_t rows = static_cast<int64_t>(impl_->data.begin()->second.size(0));
    int64_t rows_to_print = std::min<int64_t>(n, rows);
    for (int64_t i = 0; i < rows_to_print; ++i) {
        for (size_t j = 0; j < std::min(col_names.size(), static_cast<size_t>(max_cols)); ++j) {
            const auto& s = impl_->data.at(col_names[j]);
            float v = s.index({i}).item<float>();
            std::cout << std::setw(col_width) << std::fixed << std::setprecision((v == std::floor(v)) ? 0 : 4) << v << " ";
        }
        std::cout << "\n";
    }
}

void DataFrame::tail(int64_t n) const {
    if (impl_->data.empty()) { std::cout << "Empty DataFrame\n"; return; }
    int64_t rows = static_cast<int64_t>(impl_->data.begin()->second.size(0));
    int64_t start = std::max<int64_t>(0, rows - n);
    auto df_tail = this->iloc(start, rows);
    df_tail->head(n);
}

void DataFrame::info() const {
    std::cout << "<class 'pluss::table::DataFrame'>\n";
    size_t num_rows = impl_->data.empty() ? 0 : impl_->data.begin()->second.size(0);
    std::cout << "Index: 0 to " << (num_rows == 0 ? 0 : num_rows - 1) << "\n";
    std::cout << "Data columns (total " << impl_->data.size() << "):\n";

    std::cout << std::left << std::setw(5) << "#" << std::setw(20) << "Column" << std::setw(20) << "Non-Null Count" << std::setw(10) << "Dtype" << "\n";
    std::cout << std::string(55, '-') << "\n";

    int idx = 0;
    if (!impl_->column_order.empty()) {
        for (const auto& col : impl_->column_order) {
            if (impl_->data.find(col) != impl_->data.end()) {
                const auto& s = impl_->data.at(col);
                std::cout << std::setw(5) << idx++ << std::setw(20) << col << std::setw(20) << s.numel() << std::setw(10) << "?" << "\n";
            }
        }
    } else {
        for (const auto& [col, s] : impl_->data) {
            std::cout << std::setw(5) << idx++ << std::setw(20) << col << std::setw(20) << s.numel() << std::setw(10) << "?" << "\n";
        }
    }
}

void DataFrame::describe() const {
    if (impl_->data.empty()) { std::cout << "Empty DataFrame\n"; return; }
    std::vector<std::string> stat_cols = {"column","count","mean","std","min","25%","50%","75%","max"};
    std::cout << std::setw(14) << std::right << stat_cols[0];
    for (size_t i = 1; i < stat_cols.size(); ++i) std::cout << std::setw(14) << std::right << stat_cols[i];
    std::cout << "\n" << std::string(14 * stat_cols.size(), '-') << "\n";

    for (const auto& [col, s] : impl_->data) {
        auto t = s.to(config({{"dtype","float32"}}));
        float mean = t.mean().item<float>();
        float stddev = t.std().item<float>();
        float min = t.min().item<float>();
        float q25 = t.quantile(0.25).item<float>();
        float median = t.median().item<float>();
        float q75 = t.quantile(0.75).item<float>();
        float max = t.max().item<float>();
        std::cout << std::setw(14) << std::right << col
                  << std::setw(14) << std::right << static_cast<float>(t.numel())
                  << std::setw(14) << std::right << mean
                  << std::setw(14) << std::right << stddev
                  << std::setw(14) << std::right << min
                  << std::setw(14) << std::right << q25
                  << std::setw(14) << std::right << median
                  << std::setw(14) << std::right << q75
                  << std::setw(14) << std::right << max << "\n";
    }
}

std::shared_ptr<DataFrame> DataFrame::T() const {
    auto transposed = std::make_shared<DataFrame>();
    if (impl_->data.empty()) return transposed;
    size_t num_rows = impl_->data.begin()->second.size(0);
    for (size_t i = 0; i < num_rows; ++i) {
        std::vector<float> row;
        for (const auto& col : columns()) row.push_back(impl_->data.at(col).index({(int64_t)i}).item<float>());
        transposed->impl_->data["Row_" + std::to_string(i)] = pluss::value(row);
        transposed->impl_->column_order.push_back("Row_" + std::to_string(i));
    }
    return transposed;
}

std::vector<pluss::table::Thread> DataFrame::values() const {
    std::vector<pluss::table::Thread> out; out.reserve(impl_->data.size());
    for (const auto& [name, s] : impl_->data) out.emplace_back(s, name);
    return out;
}

std::shared_ptr<DataFrame> DataFrame::set_cols(const std::vector<std::string>& column_names, const std::vector<pluss::table::Thread>& values) const {
    if (column_names.size() != values.size()) throw std::invalid_argument("mismatched columns/values");
    auto df = std::make_shared<DataFrame>(*this);
    for (size_t i = 0; i < column_names.size(); ++i) df->impl_->data[column_names[i]] = values[i].to_series();
    return df;
}

std::shared_ptr<DataFrame> DataFrame::set_col(const std::string& col_name, const pluss::table::Thread& values) const {
    if (col_name.empty()) throw std::invalid_argument("empty column name");
    if (values.dim() != 1) throw std::invalid_argument("column must be 1D");
    return set_column(col_name, values);
}

std::vector<std::string> DataFrame::columns() const {
    std::vector<std::string> out; out.reserve(impl_->data.size());
    for (const auto& [k, _] : impl_->data) out.push_back(k);
    return out;
}

pluss::table::Thread DataFrame::get_col(const std::string& col_name) const {
    auto s = select_one(col_name);
    return pluss::table::Thread(s, col_name);
}

std::shared_ptr<DataFrame> DataFrame::get_cols(const std::vector<std::string>& column_names) const {
    return select_many(column_names);
}

std::shared_ptr<DataFrame> DataFrame::to(const torch::Device& device) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& [k, s] : impl_->data) df->impl_->data[k] = s.to(device);
    return df;
}

void DataFrame::to_csv(const std::string& filename) const {
    std::ofstream f(filename);
    for (const auto& [k, _] : impl_->data) f << k << ",";
    f << "\n";
    int64_t rows = static_cast<int64_t>(impl_->data.begin()->second.size(0));
    for (int64_t i = 0; i < rows; ++i) {
        for (const auto& [_, s] : impl_->data) f << s.index({i}).item<float>() << ",";
        f << "\n";
    }
}

std::shared_ptr<DataFrame> DataFrame::loc(const pluss::Series& condition) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& [k, s] : impl_->data) df->impl_->data[k] = s.index({condition});
    return df;
}

std::shared_ptr<DataFrame> DataFrame::iloc(int64_t row_start, int64_t row_end) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& [k, s] : impl_->data) df->impl_->data[k] = s.index({torch::indexing::Slice(row_start, row_end)});
    return df;
}

std::unordered_map<std::string, pluss::Series> DataFrame::iloc(int64_t row) const {
    std::unordered_map<std::string, pluss::Series> out;
    for (const auto& [k, s] : impl_->data) out[k] = s.index({row});
    return out;
}

pluss::Series DataFrame::select_one(const std::string& col_name) const {
    if (impl_->data.find(col_name) == impl_->data.end()) throw std::runtime_error("Column not found: " + col_name);
    return impl_->data.at(col_name);
}

std::shared_ptr<DataFrame> DataFrame::select_many(const std::vector<std::string>& column_names) const {
    std::unordered_map<std::string, pluss::Series> out;
    for (const auto& c : column_names) out[c] = select_one(c);
    return std::make_shared<DataFrame>(out, column_names);
}

std::shared_ptr<DataFrame> DataFrame::set_column(const std::string& col_name, const pluss::table::Thread& values) const {
    auto df = std::make_shared<DataFrame>(*this);
    df->impl_->data[col_name] = values.to_series();
    if (std::find(df->impl_->column_order.begin(), df->impl_->column_order.end(), col_name) == df->impl_->column_order.end()) df->impl_->column_order.push_back(col_name);
    return df;
}

std::shared_ptr<DataFrame> DataFrame::set_columns(const std::vector<std::string>& column_names, const std::vector<pluss::table::Thread>& values) const {
    auto df = std::make_shared<DataFrame>(*this);
    for (size_t i = 0; i < column_names.size(); ++i) {
        df->impl_->data[column_names[i]] = values[i].to_series();
        if (std::find(df->impl_->column_order.begin(), df->impl_->column_order.end(), column_names[i]) == df->impl_->column_order.end()) df->impl_->column_order.push_back(column_names[i]);
    }
    return df;
}

} // namespace pluss::table


std::shared_ptr<DataFrame> DataFrame::iloc(int64_t row_start, int64_t row_end) const {
