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
DataFrame::DataFrame(const std::vector<Thread>& threads) : impl_(std::make_unique<Impl>()) {
    for (const auto& thread : threads) {
        std::string col_name = thread.name();
        if (col_name.empty()) {
            col_name = "col_" + std::to_string(impl_->column_order.size());
        }
        impl_->data.emplace(col_name, thread);
        impl_->column_order.push_back(col_name);
    }
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

// ========== PANDAS-STYLE METHODS (Thread-First Design) ==========

std::shared_ptr<DataFrame> DataFrame::copy() const {
    return std::make_shared<DataFrame>(*this);
}

std::shared_ptr<DataFrame> DataFrame::head(int64_t n) const {
    // Thread-first: delegate to Thread.head() for each column
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        df = df->set_col(col_name, thread.head(n));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::tail(int64_t n) const {
    // Thread-first: delegate to Thread.tail() for each column
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        df = df->set_col(col_name, thread.tail(n));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::drop(const std::vector<std::string>& cols) const {
    auto df = copy();
    for (const auto& col : cols) {
        df->impl_->data.erase(col);
        auto& order = df->impl_->column_order;
        order.erase(std::remove(order.begin(), order.end(), col), order.end());
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::sort_values(const std::string& by, bool ascending) const {
    // Thread-first: delegate to Thread.sort_values(), then reorder all columns
    auto thread = get_col(by);
    auto sorted_thread = thread.sort_values(ascending);
    
    // Get the sort indices (simplified - would need proper implementation)
    auto df = std::make_shared<DataFrame>();
    df = df->set_col(by, sorted_thread);
    
    // TODO: Apply same ordering to other columns
    for (const auto& col_name : columns()) {
        if (col_name != by) {
            df = df->set_col(col_name, get_col(col_name));
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::fillna(double value) const {
    // Thread-first: delegate to Thread.fillna() for each column
    auto df = std::make_shared<DataFrame>();
    auto fill_thread = Thread(pluss::value(value), "fill");
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        df = df->set_col(col_name, thread.fillna(fill_thread));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::dropna(const std::string& /*how*/, int64_t /*thresh*/) const {
    // Thread-first: delegate to Thread.dropna() for each column
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        df = df->set_col(col_name, thread.dropna());
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::diff(int64_t periods) const {
    // Thread-first: delegate to Thread.diff() for each column
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        df = df->set_col(col_name, thread.diff(periods));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::shift(int64_t periods) const {
    // Thread-first: delegate to Thread.shift() for each column
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        df = df->set_col(col_name, thread.shift(periods));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::cumsum() const {
    // Thread-first: delegate to Thread.cumsum() for each column
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        df = df->set_col(col_name, thread.cumsum());
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::cumprod() const {
    // Thread-first: delegate to Thread.cumprod() for each column
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        df = df->set_col(col_name, thread.cumprod());
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::cummax() const {
    // Thread-first: delegate to Thread.cummax() for each column
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        df = df->set_col(col_name, thread.cummax());
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::cummin() const {
    // Thread-first: delegate to Thread.cummin() for each column
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        df = df->set_col(col_name, thread.cummin());
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::rename(const std::unordered_map<std::string, std::string>& cols) const {
    auto df = copy();
    for (const auto& [old_name, new_name] : cols) {
        if (df->impl_->data.count(old_name)) {
            auto thread = df->impl_->data[old_name];
            thread.set_name(new_name);
            df->impl_->data[new_name] = thread;
            df->impl_->data.erase(old_name);
            
            auto& order = df->impl_->column_order;
            std::replace(order.begin(), order.end(), old_name, new_name);
        }
    }
    return df;
}

// Aggregation methods (return scalars/Series)
double DataFrame::sum(const std::string& col_name) const {
    auto thread = get_col(col_name);
    if (thread.is_series()) {
        return thread.as_series().sum().item<double>();
    }
    throw std::runtime_error("sum() requires numeric column");
}

double DataFrame::mean(const std::string& col_name) const {
    auto thread = get_col(col_name);
    if (thread.is_series()) {
        return thread.as_series().mean().item<double>();
    }
    throw std::runtime_error("mean() requires numeric column");
}

int64_t DataFrame::count(const std::string& col_name) const {
    return get_col(col_name).size();
}

int64_t DataFrame::nunique(const std::string& col_name) const {
    auto thread = get_col(col_name);
    return thread.unique().size();
}

double DataFrame::min(const std::string& col_name) const {
    auto thread = get_col(col_name);
    if (thread.is_series()) {
        return thread.as_series().min().item<double>();
    }
    throw std::runtime_error("min() requires numeric column");
}

double DataFrame::max(const std::string& col_name) const {
    auto thread = get_col(col_name);
    if (thread.is_series()) {
        return thread.as_series().max().item<double>();
    }
    throw std::runtime_error("max() requires numeric column");
}

double DataFrame::std(const std::string& col_name, int ddof) const {
    auto thread = get_col(col_name);
    if (thread.is_series()) {
        auto s = thread.as_series();
        torch::Tensor tensor = s.data();
        if (tensor.numel() <= ddof) return 0.0;
        double mean_val = mean(col_name);
        auto diff = tensor - mean_val;
        auto sq = diff * diff;
        auto var_val = sq.sum() / (tensor.numel() - ddof);
        return std::sqrt(var_val.item<double>());
    }
    throw std::runtime_error("std() requires numeric column");
}

double DataFrame::var(const std::string& col_name, int ddof) const {
    auto thread = get_col(col_name);
    if (thread.is_series()) {
        auto s = thread.as_series();
        torch::Tensor tensor = s.data();
        if (tensor.numel() <= ddof) return 0.0;
        double mean_val = mean(col_name);
        auto diff = tensor - mean_val;
        auto sq = diff * diff;
        return (sq.sum() / (tensor.numel() - ddof)).item<double>();
    }
    throw std::runtime_error("var() requires numeric column");
}

double DataFrame::prod(const std::string& col_name) const {
    auto thread = get_col(col_name);
    if (thread.is_series()) {
        return thread.as_series().prod().item<double>();
    }
    throw std::runtime_error("prod() requires numeric column");
}

double DataFrame::median(const std::string& col_name) const {
    auto thread = get_col(col_name);
    if (thread.is_series()) {
        return thread.as_series().median().item<double>();
    }
    throw std::runtime_error("median() requires numeric column");
}

double DataFrame::quantile(const std::string& col_name, double q) const {
    auto thread = get_col(col_name);
    if (thread.is_series()) {
        return thread.as_series().quantile(q).item<double>();
    }
    throw std::runtime_error("quantile() requires numeric column");
}

std::pair<int64_t, int64_t> DataFrame::shape() const {
    int64_t num_rows = 0;
    if (!impl_->column_order.empty()) {
        num_rows = impl_->data.at(impl_->column_order[0]).size();
    }
    return {num_rows, static_cast<int64_t>(impl_->column_order.size())};
}

std::shared_ptr<DataFrame> DataFrame::where(const std::shared_ptr<DataFrame>& cond, double other) const {
    (void)cond; (void)other;
    return copy();
}

// =============== Additional Operations for Atlas ===============

std::shared_ptr<DataFrame> DataFrame::filter(const std::vector<std::string>& items) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : items) {
        if (impl_->data.count(col_name)) {
            df = df->set_col(col_name, impl_->data.at(col_name));
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::drop_duplicates(const std::vector<std::string>& /*subset*/) const {
    return copy();
}

std::shared_ptr<DataFrame> DataFrame::fillna(const std::unordered_map<std::string, double>& values) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        if (values.count(col_name)) {
            auto fill_thread = Thread(pluss::value(values.at(col_name)), "fill");
            df = df->set_col(col_name, thread.fillna(fill_thread));
        } else {
            df = df->set_col(col_name, thread);
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::ffill() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        df = df->set_col(col_name, get_col(col_name).ffill());
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::bfill() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        df = df->set_col(col_name, get_col(col_name).bfill());
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::interpolate(const std::string& method) const {
    (void)method;
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        df = df->set_col(col_name, get_col(col_name).interpolate());
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::isna() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        df = df->set_col(col_name, get_col(col_name).isna());
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::notna() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        df = df->set_col(col_name, get_col(col_name).notna());
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::add_prefix(const std::string& prefix) const {
    std::unordered_map<std::string, std::string> rename_map;
    for (const auto& col : columns()) {
        rename_map[col] = prefix + col;
    }
    return rename(rename_map);
}

std::shared_ptr<DataFrame> DataFrame::add_suffix(const std::string& suffix) const {
    std::unordered_map<std::string, std::string> rename_map;
    for (const auto& col : columns()) {
        rename_map[col] = col + suffix;
    }
    return rename(rename_map);
}

std::shared_ptr<DataFrame> DataFrame::abs() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        if (thread.is_series()) {
            df = df->set_col(col_name, Thread(thread.as_series().abs(), col_name));
        } else {
            df = df->set_col(col_name, thread);
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::round(int decimals) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        if (thread.is_series()) {
            df = df->set_col(col_name, Thread(thread.as_series().round(decimals), col_name));
        } else {
            df = df->set_col(col_name, thread);
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::clip(double lower, double upper) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        if (thread.is_series()) {
            df = df->set_col(col_name, Thread(thread.as_series().clip(lower, upper), col_name));
        } else {
            df = df->set_col(col_name, thread);
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::transform(const std::string& /*func*/) const {
    return copy();
}

std::shared_ptr<DataFrame> DataFrame::pct_change(int64_t periods) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        df = df->set_col(col_name, get_col(col_name).pct_change(periods));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::sqrt() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        if (thread.is_series()) {
            df = df->set_col(col_name, Thread(thread.as_series().sqrt(), col_name));
        } else {
            df = df->set_col(col_name, thread);
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::log() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        if (thread.is_series()) {
            df = df->set_col(col_name, Thread(thread.as_series().log(), col_name));
        } else {
            df = df->set_col(col_name, thread);
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::log10() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        if (thread.is_series()) {
            df = df->set_col(col_name, Thread(thread.as_series().log10(), col_name));
        } else {
            df = df->set_col(col_name, thread);
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::exp() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        if (thread.is_series()) {
            df = df->set_col(col_name, Thread(thread.as_series().exp(), col_name));
        } else {
            df = df->set_col(col_name, thread);
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::sin() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        if (thread.is_series()) {
            df = df->set_col(col_name, Thread(thread.as_series().sin(), col_name));
        } else {
            df = df->set_col(col_name, thread);
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::cos() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        if (thread.is_series()) {
            df = df->set_col(col_name, Thread(thread.as_series().cos(), col_name));
        } else {
            df = df->set_col(col_name, thread);
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::tan() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        auto thread = get_col(col_name);
        if (thread.is_series()) {
            df = df->set_col(col_name, Thread(thread.as_series().tan(), col_name));
        } else {
            df = df->set_col(col_name, thread);
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::sum() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        double val = sum(col_name);
        df = df->set_col(col_name, Thread(pluss::Series(torch::tensor({val})), col_name));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::mean() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        double val = mean(col_name);
        df = df->set_col(col_name, Thread(pluss::Series(torch::tensor({val})), col_name));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::median() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        double val = median(col_name);
        df = df->set_col(col_name, Thread(pluss::Series(torch::tensor({val})), col_name));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::std(int ddof) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        double val = std(col_name, ddof);
        df = df->set_col(col_name, Thread(pluss::Series(torch::tensor({val})), col_name));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::var(int ddof) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        double val = var(col_name, ddof);
        df = df->set_col(col_name, Thread(pluss::Series(torch::tensor({val})), col_name));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::min() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        double val = min(col_name);
        df = df->set_col(col_name, Thread(pluss::Series(torch::tensor({val})), col_name));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::max() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        double val = max(col_name);
        df = df->set_col(col_name, Thread(pluss::Series(torch::tensor({val})), col_name));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::prod() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        double val = prod(col_name);
        df = df->set_col(col_name, Thread(pluss::Series(torch::tensor({val})), col_name));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::count() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        int64_t val = count(col_name);
        df = df->set_col(col_name, Thread(pluss::Series(torch::tensor({static_cast<double>(val)})), col_name));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::nunique() const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        int64_t val = nunique(col_name);
        df = df->set_col(col_name, Thread(pluss::Series(torch::tensor({static_cast<double>(val)})), col_name));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::quantile(double q) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        double val = quantile(col_name, q);
        df = df->set_col(col_name, Thread(pluss::Series(torch::tensor({val})), col_name));
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::quantile(const std::vector<double>& /*q*/) const {
    return copy();
}

std::shared_ptr<DataFrame> DataFrame::describe(const std::vector<double>& /*percentiles*/) const {
    return copy();
}

std::shared_ptr<DataFrame> DataFrame::corr(const std::string& /*method*/) const {
    return copy();
}

std::shared_ptr<DataFrame> DataFrame::cov(int /*ddof*/) const {
    return copy();
}

std::shared_ptr<DataFrame> DataFrame::rank(const std::string& /*method*/) const {
    return copy();
}

std::shared_ptr<DataFrame> DataFrame::eq(const std::shared_ptr<DataFrame>& other) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.eq(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::ne(const std::shared_ptr<DataFrame>& other) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.ne(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::lt(const std::shared_ptr<DataFrame>& other) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.lt(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::le(const std::shared_ptr<DataFrame>& other) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.le(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::gt(const std::shared_ptr<DataFrame>& other) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.gt(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::ge(const std::shared_ptr<DataFrame>& other) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.ge(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::add(const std::shared_ptr<DataFrame>& other, double /*fill_value*/) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.add(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::sub(const std::shared_ptr<DataFrame>& other, double /*fill_value*/) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.sub(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::mul(const std::shared_ptr<DataFrame>& other, double /*fill_value*/) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.mul(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::div(const std::shared_ptr<DataFrame>& other, double /*fill_value*/) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.div(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::mod(const std::shared_ptr<DataFrame>& other, double /*fill_value*/) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.mod(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::pow(const std::shared_ptr<DataFrame>& other, double /*fill_value*/) const {
    auto df = std::make_shared<DataFrame>();
    for (const auto& col_name : columns()) {
        if (impl_->data.count(col_name) && other->impl_->data.count(col_name)) {
            auto t1 = impl_->data.at(col_name);
            auto t2 = other->impl_->data.at(col_name);
            if (t1.is_series() && t2.is_series()) {
                df = df->set_col(col_name, t1.pow(t2));
            }
        }
    }
    return df;
}

std::shared_ptr<DataFrame> DataFrame::merge(
    const std::shared_ptr<DataFrame>& /*other*/,
    const std::string& /*on*/,
    const std::string& /*how*/) const {
    return copy();
}

std::shared_ptr<DataFrame> DataFrame::merge(
    const std::shared_ptr<DataFrame>& /*other*/,
    const std::vector<std::string>& /*on*/,
    const std::string& /*how*/) const {
    return copy();
}

std::shared_ptr<DataFrame> DataFrame::join(
    const std::shared_ptr<DataFrame>& /*other*/,
    const std::string& /*how*/,
    const std::string& /*lsuffix*/,
    const std::string& /*rsuffix*/) const {
    return copy();
}

std::shared_ptr<DataFrame> DataFrame::all() const {
    return copy();
}

std::shared_ptr<DataFrame> DataFrame::any() const {
    return copy();
}

bool DataFrame::equals(const std::shared_ptr<DataFrame>& other) const {
    if (columns() != other->columns()) return false;
    for (const auto& col : columns()) {
        if (count(col) != other->count(col)) return false;
    }
    return true;
}

std::string DataFrame::to_json(const std::string& /*orient*/) const {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& col : columns()) {
        if (!first) oss << ", ";
        first = false;
        oss << "\"" << col << "\": []";
    }
    oss << "}";
    return oss.str();
}

} // namespace pluss::table
