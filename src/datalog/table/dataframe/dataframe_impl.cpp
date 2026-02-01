#include <pluss/datalog/table/dataframe/dataframe.h>
#include <pluss/datalog/table/io/loader.h>
#include <pluss/datalog/table/thread/thread.h>

#include <pluss/pluss.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <string>

namespace pluss::table {

// PIMPL implementation struct definition
struct DataFrame::Impl {
    std::unordered_map<std::string, pluss::table::Thread> data;
    std::vector<std::string> column_order;
    Impl() = default;
    Impl(const std::unordered_map<std::string, pluss::table::Thread>& d, const std::vector<std::string>& order)
        : data(d), column_order(order) {}
};

// Implementations

// Special members
DataFrame::DataFrame() : impl_(std::make_unique<Impl>()) {}
DataFrame::DataFrame(const std::unordered_map<std::string, pluss::Series>& data) : impl_(std::make_unique<Impl>()) {
    for (const auto &kv : data) {
        impl_->data.emplace(kv.first, pluss::table::Thread(kv.second, kv.first));
        impl_->column_order.push_back(kv.first);
    }
}
DataFrame::DataFrame(const std::unordered_map<std::string, pluss::Series>& data, const std::vector<std::string>& column_order)
    : impl_(std::make_unique<Impl>()) {
    for (const auto &kv : data) impl_->data.emplace(kv.first, pluss::table::Thread(kv.second, kv.first));
    impl_->column_order = column_order;
}
DataFrame::DataFrame(const DataFrame& other) : impl_(std::make_unique<Impl>(other.impl_->data, other.impl_->column_order)) {}
DataFrame& DataFrame::operator=(const DataFrame& other) { if (this != &other) impl_ = std::make_unique<Impl>(other.impl_->data, other.impl_->column_order); return *this; }
DataFrame::DataFrame(DataFrame&& other) noexcept = default;
DataFrame& DataFrame::operator=(DataFrame&& other) noexcept = default;
DataFrame::~DataFrame() = default;

std::shared_ptr<DataFrame> DataFrame::load(const std::string& format, const std::string& filename) {
    return io::load(format, filename);
}

std::shared_ptr<DataFrame> DataFrame::read_csv(const std::string& filename) {
    return io::load_csv(filename);
}

std::shared_ptr<DataFrame> DataFrame::T() const {
    auto transposed = std::make_shared<DataFrame>();
    if (impl_->data.empty()) return transposed;
    size_t num_rows = impl_->data.begin()->second.size();
    for (size_t i = 0; i < num_rows; ++i) {
        std::vector<float> row;
        for (const auto& col : columns()) {
            const auto &t = impl_->data.at(col);
            if (t.is_series()) row.push_back(at::Tensor(static_cast<const at::TensorBase&>(t.as_series()))[i].item<float>());
            else if (t.is_timeline()) row.push_back(at::Tensor(static_cast<const at::TensorBase&>(t.as_timeline().to_Series()))[i].item<float>());
            else row.push_back(NAN);
        }
        transposed->impl_->data["Row_" + std::to_string(i)] = pluss::value(row);
        transposed->impl_->column_order.push_back("Row_" + std::to_string(i));
    }
    return transposed;
}

std::vector<pluss::table::Thread> DataFrame::values() const {
    std::vector<pluss::table::Thread> out; out.reserve(impl_->data.size());
    for (const auto& [name, t] : impl_->data) out.emplace_back(t);
    return out;
}

std::shared_ptr<DataFrame> DataFrame::set_cols(const std::vector<std::string>& column_names, const std::vector<pluss::table::Thread>& values) const {
    if (column_names.size() != values.size()) throw std::invalid_argument("mismatched columns/values");
    auto df = std::make_shared<DataFrame>(*this);
    for (size_t i = 0; i < column_names.size(); ++i) df->impl_->data[column_names[i]] = values[i];
    return df;
}

std::shared_ptr<DataFrame> DataFrame::set_col(const std::string& col_name, const pluss::table::Thread& values) const {
    if (col_name.empty()) throw std::invalid_argument("empty column name");
    if (values.dim() != 1) throw std::invalid_argument("column must be 1D");
    return set_column(col_name, values);
}

std::vector<std::string> DataFrame::columns() const {
    if (!impl_->column_order.empty()) return impl_->column_order;
    std::vector<std::string> out; out.reserve(impl_->data.size());
    for (const auto& [k, _] : impl_->data) out.push_back(k);
    return out;
}

pluss::table::Thread DataFrame::get_col(const std::string& col_name) const {
    return select_one(col_name);
}

std::shared_ptr<DataFrame> DataFrame::get_cols(const std::vector<std::string>& column_names) const {
    return select_many(column_names);
}

std::shared_ptr<DataFrame> DataFrame::to(const torch::Device& device) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& [k, t] : impl_->data) {
        if (t.is_series()) {
            auto s = t.as_series();
            df->impl_->data[k] = pluss::table::Thread(pluss::Series(at::Tensor(static_cast<const at::TensorBase&>(s)).to(device)), k);
        } else if (t.is_timeline()) {
            auto s = t.as_timeline().to_Series();
            df->impl_->data[k] = pluss::table::Thread(pluss::Series(at::Tensor(static_cast<const at::TensorBase&>(s)).to(device)), k);
        } else {
            df->impl_->data[k] = t; // Notes unchanged
        }
    }
    return df;
}

void DataFrame::to_csv(const std::string& filename) const {
    std::ofstream f(filename);
    for (const auto& [k, _] : impl_->data) f << k << ",";
    f << "\n";
    if (impl_->data.empty()) return;
    // determine number of rows from first column
    int64_t rows = impl_->data.begin()->second.size();
    for (int64_t i = 0; i < rows; ++i) {
        bool first = true;
        for (const auto& [_, t] : impl_->data) {
            if (!first) f << ",";
            first = false;
            if (t.is_series()) {
                auto s = t.as_series();
                auto val = at::Tensor(static_cast<const at::TensorBase&>(s))[i];
                if (val.is_floating_point()) f << val.item<float>();
                else f << val.item<int64_t>();
            } else if (t.is_timeline()) {
                auto s = t.as_timeline().to_Series();
                auto val = at::Tensor(static_cast<const at::TensorBase&>(s))[i];
                if (val.is_floating_point()) f << val.item<float>();
                else f << val.item<int64_t>();
            } else if (t.is_note()) {
                try {
                    auto vec = t.as_note().vector();
                    if (static_cast<size_t>(i) < vec.size()) f << vec[i].str(); else f << "";
                } catch (...) { f << ""; }
            }
        }
        f << "\n";
    }
}

std::shared_ptr<DataFrame> DataFrame::loc(const pluss::Series& condition) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& [k, t] : impl_->data) {
        if (t.is_series()) {
            auto s = t.as_series();
            df->impl_->data[k] = pluss::table::Thread(pluss::Series(at::Tensor(static_cast<const at::TensorBase&>(s)).index({at::Tensor(static_cast<const at::TensorBase&>(condition))})), k);
        } else if (t.is_timeline()) {
            auto s = t.as_timeline().to_Series();
            df->impl_->data[k] = pluss::table::Thread(pluss::Series(at::Tensor(static_cast<const at::TensorBase&>(s)).index({at::Tensor(static_cast<const at::TensorBase&>(condition))})), k);
        } else {
            // for Notes, filter using boolean mask by selecting vector elements
            try {
                auto vec = t.as_note().vector();
                std::vector<std::string> sel;
                auto mask = at::Tensor(static_cast<const at::TensorBase&>(condition));
                for (int64_t i = 0; i < mask.size(0) && static_cast<size_t>(i) < vec.size(); ++i) {
                    if (mask[i].item<int64_t>() != 0) sel.push_back(vec[i].str());
                }
                std::vector<pluss::Chord> chords; chords.reserve(sel.size());
                for (const auto &sstr : sel) chords.emplace_back(sstr);
                df->impl_->data[k] = pluss::table::Thread(pluss::make_note(chords), k);
            } catch (...) { df->impl_->data[k] = t; }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::iloc(int64_t row_start, int64_t row_end) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& [k, t] : impl_->data) {
        if (t.is_series()) {
            auto s = t.as_series();
            df->impl_->data[k] = pluss::table::Thread(pluss::Series(at::Tensor(static_cast<const at::TensorBase&>(s)).index({torch::indexing::Slice(row_start, row_end)})), k);
        } else if (t.is_timeline()) {
            auto s = t.as_timeline().to_Series();
            df->impl_->data[k] = pluss::table::Thread(pluss::Series(at::Tensor(static_cast<const at::TensorBase&>(s)).index({torch::indexing::Slice(row_start, row_end)})), k);
        } else if (t.is_note()) {
            try {
                auto vec = t.as_note().vector();
                std::vector<std::string> sel;
                for (int64_t i = row_start; i < row_end && static_cast<size_t>(i) < vec.size(); ++i) sel.push_back(vec[i].str());
                std::vector<pluss::Chord> chords; chords.reserve(sel.size());
                for (const auto &sstr : sel) chords.emplace_back(sstr);
                df->impl_->data[k] = pluss::table::Thread(pluss::make_note(chords), k);
            } catch (...) { df->impl_->data[k] = t; }
        }
    }
    return df;
}

std::unordered_map<std::string, pluss::table::Thread> DataFrame::iloc(int64_t row) const {
    std::unordered_map<std::string, pluss::table::Thread> out;
    for (const auto& [k, t] : impl_->data) {
        if (t.is_series()) {
            auto s = t.as_series();
            out[k] = pluss::table::Thread(pluss::Series(at::Tensor(static_cast<const at::TensorBase&>(s))[row]), k);
        } else if (t.is_timeline()) {
            auto s = t.as_timeline().to_Series();
            out[k] = pluss::table::Thread(pluss::Series(at::Tensor(static_cast<const at::TensorBase&>(s))[row]), k);
        } else if (t.is_note()) {
            try {
                auto vec = t.as_note().vector();
                if (static_cast<size_t>(row) < vec.size()) out[k] = pluss::table::Thread(pluss::make_note(vec[row].str()), k);
                else out[k] = pluss::table::Thread(pluss::make_note(std::string("")), k);
            } catch (...) { out[k] = t; }
        }
    }
    return out;
}

pluss::table::Thread DataFrame::select_one(const std::string& col_name) const {
    if (impl_->data.find(col_name) == impl_->data.end()) throw std::runtime_error("Column not found: " + col_name);
    return impl_->data.at(col_name);
}

std::shared_ptr<DataFrame> DataFrame::select_many(const std::vector<std::string>& column_names) const {
    std::unordered_map<std::string, pluss::Series> out_series;
    std::unordered_map<std::string, pluss::table::Thread> out_thread;
    for (const auto& c : column_names) out_thread[c] = select_one(c);
    return std::make_shared<DataFrame>(std::unordered_map<std::string, pluss::Series>{}, column_names)->set_cols(column_names, [=]() {
        std::vector<pluss::table::Thread> vec; vec.reserve(column_names.size());
        for (const auto& c : column_names) vec.push_back(out_thread.at(c));
        return vec;
    }());
}

std::shared_ptr<DataFrame> DataFrame::set_column(const std::string& col_name, const pluss::table::Thread& values) const {
    auto df = std::make_shared<DataFrame>(*this);
    df->impl_->data[col_name] = values;
    if (std::find(df->impl_->column_order.begin(), df->impl_->column_order.end(), col_name) == df->impl_->column_order.end()) df->impl_->column_order.push_back(col_name);
    return df;
}

std::shared_ptr<DataFrame> DataFrame::set_columns(const std::vector<std::string>& column_names, const std::vector<pluss::table::Thread>& values) const {
    auto df = std::make_shared<DataFrame>(*this);
    for (size_t i = 0; i < column_names.size(); ++i) {
        df->impl_->data[column_names[i]] = values[i];
        if (std::find(df->impl_->column_order.begin(), df->impl_->column_order.end(), column_names[i]) == df->impl_->column_order.end()) df->impl_->column_order.push_back(column_names[i]);
    }
    return df;
}

} // namespace pluss::table
