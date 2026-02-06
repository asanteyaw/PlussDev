#include <pluss/datalog/handle/feed/feed.h>
#include <pluss/NoteOps.h>
#include <pluss/note_factory.h>
#include <pluss/series_factory.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <limits>
#include <random>
#include <cstdlib>
#include <sstream>
#include <iomanip>

namespace pluss::handle {

// Homogeneous vectorized Feed operations (dictionary-esque, mirroring Thread)

// ===== Indexing and slicing =====

Feed Feed::head(int64_t n) const {
    if (n <= 0 || size() == 0) return Feed(pluss::zeros({0}), name_);
    const int64_t actual_n = std::min(n, size());
    
    if (is_series()) {
        return Feed(as_series().narrow(0, 0, actual_n), name_);
    }
    if (is_note()) {
        return Feed(pluss::ops::slice(as_note(), 0, 0, static_cast<int>(actual_n)), name_);
    }
    if (is_timeline()) {
        auto s = to_series();
        return Feed(s.narrow(0, 0, actual_n), name_);
    }
    return *this;
}

Feed Feed::tail(int64_t n) const {
    if (n <= 0 || size() == 0) return Feed(pluss::zeros({0}), name_);
    const int64_t sz = size();
    const int64_t actual_n = std::min(n, sz);
    const int64_t start = sz - actual_n;
    
    if (is_series()) {
        return Feed(as_series().narrow(0, start, actual_n), name_);
    }
    if (is_note()) {
        return Feed(pluss::ops::slice(as_note(), 0, static_cast<int>(start), static_cast<int>(sz)), name_);
    }
    if (is_timeline()) {
        auto s = to_series();
        return Feed(s.narrow(0, start, actual_n), name_);
    }
    return *this;
}

Feed Feed::slice(int64_t start, int64_t end) const {
    if (start < 0 || end <= start || size() == 0) return Feed(pluss::zeros({0}), name_);
    const int64_t sz = size();
    const int64_t actual_start = std::min(start, sz);
    const int64_t actual_end = std::min(end, sz);
    const int64_t length = actual_end - actual_start;
    
    if (is_series()) {
        return Feed(as_series().narrow(0, actual_start, length), name_);
    }
    if (is_note()) {
        return Feed(pluss::ops::slice(as_note(), 0, static_cast<int>(actual_start), static_cast<int>(actual_end)), name_);
    }
    if (is_timeline()) {
        auto s = to_series();
        return Feed(s.narrow(0, actual_start, length), name_);
    }
    return *this;
}

Feed Feed::at(int64_t index) const {
    if (index < 0 || index >= size()) return Feed(pluss::zeros({0}), name_);
    
    if (is_series()) {
        return Feed(as_series().narrow(0, index, 1), name_);
    }
    if (is_note()) {
        return Feed(pluss::ops::slice(as_note(), 0, static_cast<int>(index), static_cast<int>(index + 1)), name_);
    }
    if (is_timeline()) {
        auto s = to_series();
        return Feed(s.narrow(0, index, 1), name_);
    }
    return *this;
}

Feed Feed::masked_select(const pluss::Series& mask) const {
    if (is_series()) {
        return Feed(as_series().index({mask}), name_);
    }
    if (is_timeline()) {
        return Feed(to_series().index({mask}), name_);
    }
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            const auto cond_numel = mask.numel();
            const auto vec_size = static_cast<int64_t>(vec.size());
            const auto min_size = std::min(cond_numel, vec_size);
            
            auto cond_contig = mask.contiguous();
            auto cond_accessor = cond_contig.accessor<bool, 1>();
            int64_t true_count = 0;
            for (int64_t i = 0; i < min_size; ++i) {
                true_count += cond_accessor[i];
            }
            
            std::vector<pluss::Chord> chords;
            chords.reserve(true_count);
            for (int64_t i = 0; i < min_size; ++i) {
                if (cond_accessor[i]) {
                    chords.emplace_back(vec[i]);
                }
            }
            
            return Feed(pluss::make_note(chords), name_);
        } catch (...) {
            return *this;
        }
    }
    return *this;
}

// ===== Filtering =====

Feed Feed::filter(const pluss::Series& condition) const {
    return masked_select(condition);
}

Feed Feed::drop_duplicates() const {
    return unique();
}

Feed Feed::unique() const {
    if (is_series()) {
        auto sorted_vals = std::get<0>(as_series().sort(0));
        int64_t numel = sorted_vals.numel();
        if (numel == 0) return *this;
        if (numel == 1) return Feed(sorted_vals, name_);
        
        auto shifted = sorted_vals.slice(0, 0, numel - 1);
        auto current = sorted_vals.slice(0, 1, numel);
        auto diff_mask = current.ne(shifted);
        
        auto first_true = pluss::ones({1}, pluss::SeriesOptions().dtype(at::kBool));
        auto keep_mask = pluss::from_torch(torch::cat({first_true.tensor(), diff_mask.tensor()}, 0));
        
        return Feed(sorted_vals.masked_select(keep_mask), name_);
    }
    if (is_timeline()) {
        return Feed(to_series(), name_).unique();
    }
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::unordered_set<std::string> seen;
            std::vector<pluss::Chord> chords;
            for (const auto& chord : vec) {
                if (seen.insert(chord.str()).second) {
                    chords.push_back(chord);
                }
            }
            return Feed(pluss::make_note(chords), name_);
        } catch (...) {
            return *this;
        }
    }
    return *this;
}

// ===== Sorting =====

Feed Feed::sort(bool ascending) const {
    if (is_series()) {
        auto sorted_vals = std::get<0>(as_series().sort(0, !ascending));
        return Feed(sorted_vals, name_);
    }
    if (is_timeline()) {
        auto s = to_series();
        auto sorted_vals = std::get<0>(s.sort(0, !ascending));
        return Feed(sorted_vals, name_);
    }
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<std::string> strs;
            for (const auto& chord : vec) strs.push_back(chord.str());
            
            if (ascending) {
                std::sort(strs.begin(), strs.end());
            } else {
                std::sort(strs.begin(), strs.end(), std::greater<>());
            }
            
            std::vector<pluss::Chord> chords;
            for (const auto &s : strs) chords.emplace_back(s);
            return Feed(pluss::make_note(chords), name_);
        } catch (...) {
            return *this;
        }
    }
    return *this;
}

Feed Feed::argsort(bool ascending) const {
    if (is_series()) {
        auto indices = std::get<1>(as_series().sort(0, !ascending));
        return Feed(indices, name_ + "_argsort");
    }
    if (is_timeline()) {
        auto s = to_series();
        auto indices = std::get<1>(s.sort(0, !ascending));
        return Feed(indices, name_ + "_argsort");
    }
    return Feed(pluss::arange(0, size(), pluss::SeriesOptions().dtype(at::kLong)), name_ + "_argsort");
}

// ===== Transformations =====

Feed Feed::apply(std::function<Feed(const Feed&)> func) const {
    return func(*this);
}

Feed Feed::map(std::function<pluss::Series(const pluss::Series&)> func) const {
    if (is_series()) {
        return Feed(func(as_series()), name_);
    }
    if (is_timeline()) {
        return Feed(func(to_series()), name_);
    }
    return *this;
}

// ===== String operations (Note only) =====

Feed Feed::str_upper() const {
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<pluss::Chord> chords;
            for (const auto& chord : vec) {
                std::string s = chord.str();
                std::transform(s.begin(), s.end(), s.begin(), ::toupper);
                chords.emplace_back(s);
            }
            return Feed(pluss::make_note(chords), name_);
        } catch (...) {}
    }
    return *this;
}

Feed Feed::str_lower() const {
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<pluss::Chord> chords;
            for (const auto& chord : vec) {
                std::string s = chord.str();
                std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                chords.emplace_back(s);
            }
            return Feed(pluss::make_note(chords), name_);
        } catch (...) {}
    }
    return *this;
}

Feed Feed::str_capitalize() const {
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<pluss::Chord> chords;
            for (const auto& chord : vec) {
                std::string s = chord.str();
                if (!s.empty()) {
                    s[0] = ::toupper(s[0]);
                }
                chords.emplace_back(s);
            }
            return Feed(pluss::make_note(chords), name_);
        } catch (...) {}
    }
    return *this;
}

Feed Feed::str_trim() const {
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<pluss::Chord> chords;
            for (const auto& chord : vec) {
                std::string s = chord.str();
                size_t start = s.find_first_not_of(" \t\n\r");
                size_t end = s.find_last_not_of(" \t\n\r");
                if (start != std::string::npos) {
                    s = s.substr(start, end - start + 1);
                } else {
                    s = "";
                }
                chords.emplace_back(s);
            }
            return Feed(pluss::make_note(chords), name_);
        } catch (...) {}
    }
    return *this;
}

Feed Feed::str_replace(const std::string& pattern, const std::string& replacement) const {
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<pluss::Chord> chords;
            for (const auto& chord : vec) {
                std::string s = chord.str();
                size_t pos = 0;
                while ((pos = s.find(pattern, pos)) != std::string::npos) {
                    s.replace(pos, pattern.length(), replacement);
                    pos += replacement.length();
                }
                chords.emplace_back(s);
            }
            return Feed(pluss::make_note(chords), name_);
        } catch (...) {}
    }
    return *this;
}

Feed Feed::str_contains(const std::string& pattern) const {
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<bool> results;
            for (const auto& chord : vec) {
                results.push_back(chord.str().find(pattern) != std::string::npos);
            }
            std::vector<uint8_t> bytes(results.begin(), results.end());
            return Feed(pluss::from_blob(bytes.data(), {static_cast<int64_t>(bytes.size())}, 
                                         pluss::SeriesOptions().dtype(at::kBool)).clone(), name_);
        } catch (...) {}
    }
    return Feed(pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool)), name_);
}

Feed Feed::str_startswith(const std::string& prefix) const {
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<bool> results;
            for (const auto& chord : vec) {
                std::string s = chord.str();
                results.push_back(s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix);
            }
            std::vector<uint8_t> bytes(results.begin(), results.end());
            return Feed(pluss::from_blob(bytes.data(), {static_cast<int64_t>(bytes.size())}, 
                                         pluss::SeriesOptions().dtype(at::kBool)).clone(), name_);
        } catch (...) {}
    }
    return Feed(pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool)), name_);
}

Feed Feed::str_endswith(const std::string& suffix) const {
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<bool> results;
            for (const auto& chord : vec) {
                std::string s = chord.str();
                results.push_back(s.size() >= suffix.size() && 
                                 s.substr(s.size() - suffix.size()) == suffix);
            }
            std::vector<uint8_t> bytes(results.begin(), results.end());
            return Feed(pluss::from_blob(bytes.data(), {static_cast<int64_t>(bytes.size())}, 
                                         pluss::SeriesOptions().dtype(at::kBool)).clone(), name_);
        } catch (...) {}
    }
    return Feed(pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool)), name_);
}

Feed Feed::str_length() const {
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<int64_t> lengths;
            for (const auto& chord : vec) {
                lengths.push_back(static_cast<int64_t>(chord.str().size()));
            }
            return Feed(pluss::from_blob(lengths.data(), {static_cast<int64_t>(lengths.size())}, 
                                         pluss::SeriesOptions().dtype(at::kLong)), name_);
        } catch (...) {}
    }
    return Feed(pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kLong)), name_);
}

Feed Feed::str_split(const std::string& delimiter) const {
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<pluss::Chord> split_results;
            for (const auto& chord : vec) {
                std::string s = chord.str();
                size_t start = 0;
                size_t end = s.find(delimiter);
                while (end != std::string::npos) {
                    split_results.emplace_back(s.substr(start, end - start));
                    start = end + delimiter.length();
                    end = s.find(delimiter, start);
                }
                split_results.emplace_back(s.substr(start));
            }
            return Feed(pluss::make_note(split_results), name_);
        } catch (...) {}
    }
    return *this;
}

// ===== Numeric operations (Series only) =====

Feed Feed::abs() const {
    if (is_series()) {
        return Feed(as_series().abs(), name_);
    }
    if (is_timeline()) {
        return Feed(to_series().abs(), name_);
    }
    return *this;
}

Feed Feed::sqrt() const {
    if (is_series()) {
        return Feed(as_series().sqrt(), name_);
    }
    if (is_timeline()) {
        return Feed(to_series().sqrt(), name_);
    }
    return *this;
}

Feed Feed::pow(double exponent) const {
    if (is_series()) {
        return Feed(as_series().pow(exponent), name_);
    }
    if (is_timeline()) {
        return Feed(to_series().pow(exponent), name_);
    }
    return *this;
}

Feed Feed::log() const {
    if (is_series()) {
        return Feed(as_series().log(), name_);
    }
    if (is_timeline()) {
        return Feed(to_series().log(), name_);
    }
    return *this;
}

Feed Feed::exp() const {
    if (is_series()) {
        return Feed(as_series().exp(), name_);
    }
    if (is_timeline()) {
        return Feed(to_series().exp(), name_);
    }
    return *this;
}

Feed Feed::floor() const {
    if (is_series()) {
        return Feed(as_series().floor(), name_);
    }
    if (is_timeline()) {
        return Feed(to_series().floor(), name_);
    }
    return *this;
}

Feed Feed::ceil() const {
    if (is_series()) {
        return Feed(as_series().ceil(), name_);
    }
    if (is_timeline()) {
        return Feed(to_series().ceil(), name_);
    }
    return *this;
}

Feed Feed::round(int decimals) const {
    if (is_series()) {
        double factor = std::pow(10.0, decimals);
        return Feed((as_series() * factor).round() / factor, name_);
    }
    if (is_timeline()) {
        double factor = std::pow(10.0, decimals);
        auto s = to_series();
        return Feed((s * factor).round() / factor, name_);
    }
    return *this;
}

Feed Feed::clip(double min_val, double max_val) const {
    if (is_series()) {
        return Feed(as_series().clamp(min_val, max_val), name_);
    }
    if (is_timeline()) {
        return Feed(to_series().clamp(min_val, max_val), name_);
    }
    return *this;
}

Feed Feed::normalize() const {
    if (is_series()) {
        auto s = as_series();
        auto min_v = s.min().item<double>();
        auto max_v = s.max().item<double>();
        if (max_v - min_v == 0) return Feed(pluss::zeros_like(s), name_);
        return Feed((s - min_v) / (max_v - min_v), name_);
    }
    if (is_timeline()) {
        auto s = to_series();
        auto min_v = s.min().item<double>();
        auto max_v = s.max().item<double>();
        if (max_v - min_v == 0) return Feed(pluss::zeros_like(s), name_);
        return Feed((s - min_v) / (max_v - min_v), name_);
    }
    return *this;
}

Feed Feed::standardize() const {
    if (is_series()) {
        auto s = as_series();
        auto mean_v = s.mean().item<double>();
        auto std_v = s.std().item<double>();
        if (std_v == 0) return Feed(pluss::zeros_like(s), name_);
        return Feed((s - mean_v) / std_v, name_);
    }
    if (is_timeline()) {
        auto s = to_series();
        auto mean_v = s.mean().item<double>();
        auto std_v = s.std().item<double>();
        if (std_v == 0) return Feed(pluss::zeros_like(s), name_);
        return Feed((s - mean_v) / std_v, name_);
    }
    return *this;
}

// ===== Missing value handling =====

Feed Feed::fill_na(double value) const {
    if (is_series()) {
        return Feed(as_series().nan_to_num(value), name_);
    }
    if (is_timeline()) {
        return Feed(to_series().nan_to_num(value), name_);
    }
    return *this;
}

Feed Feed::drop_na() const {
    if (is_series()) {
        auto mask = as_series().isnan().logical_not();
        return Feed(as_series().index({mask}), name_);
    }
    if (is_timeline()) {
        auto s = to_series();
        auto mask = s.isnan().logical_not();
        return Feed(s.index({mask}), name_);
    }
    return *this;
}

Feed Feed::ffill() const {
    if (is_series()) {
        auto s = as_series();
        auto s_double = s.to(at::kDouble);
        auto result = s_double.clone();
        auto accessor = result.accessor<double, 1>();
        double last_valid = std::nan("");
        for (int64_t i = 0; i < result.numel(); ++i) {
            if (!std::isnan(accessor[i])) {
                last_valid = accessor[i];
            } else if (!std::isnan(last_valid)) {
                accessor[i] = last_valid;
            }
        }
        return Feed(result, name_);
    }
    return *this;
}

Feed Feed::bfill() const {
    if (is_series()) {
        auto s = as_series();
        auto s_double = s.to(at::kDouble);
        auto result = s_double.clone();
        auto accessor = result.accessor<double, 1>();
        double next_valid = std::nan("");
        for (int64_t i = result.numel() - 1; i >= 0; --i) {
            if (!std::isnan(accessor[i])) {
                next_valid = accessor[i];
            } else if (!std::isnan(next_valid)) {
                accessor[i] = next_valid;
            }
        }
        return Feed(result, name_);
    }
    return *this;
}

bool Feed::has_na() const {
    if (is_series()) {
        return as_series().isnan().any().item<bool>();
    }
    if (is_timeline()) {
        return to_series().isnan().any().item<bool>();
    }
    return false;
}

int64_t Feed::count_na() const {
    if (is_series()) {
        return as_series().isnan().sum().item<int64_t>();
    }
    if (is_timeline()) {
        return to_series().isnan().sum().item<int64_t>();
    }
    return 0;
}

// ===== Statistical operations =====

double Feed::sum() const {
    if (is_series()) return as_series().sum().item<double>();
    if (is_timeline()) return to_series().sum().item<double>();
    return 0.0;
}

double Feed::mean() const {
    if (is_series()) return as_series().mean().item<double>();
    if (is_timeline()) return to_series().mean().item<double>();
    return 0.0;
}

double Feed::median() const {
    if (is_series()) return as_series().median().item<double>();
    if (is_timeline()) return to_series().median().item<double>();
    return 0.0;
}

double Feed::std() const {
    if (is_series()) return as_series().std().item<double>();
    if (is_timeline()) return to_series().std().item<double>();
    return 0.0;
}

double Feed::var() const {
    if (is_series()) return as_series().var().item<double>();
    if (is_timeline()) return to_series().var().item<double>();
    return 0.0;
}

double Feed::min() const {
    if (is_series()) return as_series().min().item<double>();
    if (is_timeline()) return to_series().min().item<double>();
    return 0.0;
}

double Feed::max() const {
    if (is_series()) return as_series().max().item<double>();
    if (is_timeline()) return to_series().max().item<double>();
    return 0.0;
}

int64_t Feed::count() const {
    return size();
}

double Feed::quantile(double q) const {
    if (is_series()) return as_series().quantile(q).item<double>();
    if (is_timeline()) return to_series().quantile(q).item<double>();
    return 0.0;
}

// ===== Cumulative operations =====

Feed Feed::cumsum() const {
    if (is_series()) return Feed(as_series().cumsum(0), name_);
    if (is_timeline()) return Feed(to_series().cumsum(0), name_);
    return *this;
}

Feed Feed::cumprod() const {
    if (is_series()) return Feed(as_series().cumprod(0), name_);
    if (is_timeline()) return Feed(to_series().cumprod(0), name_);
    return *this;
}

Feed Feed::cummax() const {
    if (is_series()) return Feed(std::get<0>(as_series().cummax(0)), name_);
    if (is_timeline()) return Feed(std::get<0>(to_series().cummax(0)), name_);
    return *this;
}

Feed Feed::cummin() const {
    if (is_series()) return Feed(std::get<0>(as_series().cummin(0)), name_);
    if (is_timeline()) return Feed(std::get<0>(to_series().cummin(0)), name_);
    return *this;
}

// ===== Rolling window operations =====

Feed Feed::rolling_mean(int64_t window) const {
    if (window <= 0) return *this;
    if (is_series()) {
        auto s = as_series();
        auto sz = s.numel();
        if (sz < window) return *this;
        
        std::vector<double> result;
        result.reserve(sz - window + 1);
        auto s_double = s.to(at::kDouble);
        auto cont_s = s_double.contiguous();
        auto accessor = cont_s.accessor<double, 1>();
        
        for (int64_t i = 0; i <= sz - window; ++i) {
            double sum = 0.0;
            for (int64_t j = i; j < i + window; ++j) {
                sum += accessor[j];
            }
            result.push_back(sum / window);
        }
        
        return Feed(pluss::from_blob(result.data(), {static_cast<int64_t>(result.size())}, 
                                     pluss::SeriesOptions().dtype(at::kDouble)), name_);
    }
    return *this;
}

Feed Feed::rolling_sum(int64_t window) const {
    if (window <= 0) return *this;
    if (is_series()) {
        auto s = as_series();
        auto sz = s.numel();
        if (sz < window) return *this;
        
        std::vector<double> result;
        result.reserve(sz - window + 1);
        auto s_double = s.to(at::kDouble);
        auto cont_s = s_double.contiguous();
        auto accessor = cont_s.accessor<double, 1>();
        
        for (int64_t i = 0; i <= sz - window; ++i) {
            double sum = 0.0;
            for (int64_t j = i; j < i + window; ++j) {
                sum += accessor[j];
            }
            result.push_back(sum);
        }
        
        return Feed(pluss::from_blob(result.data(), {static_cast<int64_t>(result.size())}, 
                                     pluss::SeriesOptions().dtype(at::kDouble)), name_);
    }
    return *this;
}

Feed Feed::rolling_std(int64_t window) const {
    if (window <= 0) return *this;
    if (is_series()) {
        auto s = as_series();
        auto sz = s.numel();
        if (sz < window) return *this;
        
        std::vector<double> result;
        result.reserve(sz - window + 1);
        auto s_double = s.to(at::kDouble);
        auto cont_s = s_double.contiguous();
        auto accessor = cont_s.accessor<double, 1>();
        
        for (int64_t i = 0; i <= sz - window; ++i) {
            double mean = 0.0, sq_sum = 0.0;
            for (int64_t j = i; j < i + window; ++j) {
                mean += accessor[j];
            }
            mean /= window;
            
            for (int64_t j = i; j < i + window; ++j) {
                double diff = accessor[j] - mean;
                sq_sum += diff * diff;
            }
            result.push_back(std::sqrt(sq_sum / window));
        }
        
        return Feed(pluss::from_blob(result.data(), {static_cast<int64_t>(result.size())}, 
                                     pluss::SeriesOptions().dtype(at::kDouble)), name_);
    }
    return *this;
}

Feed Feed::rolling_min(int64_t window) const {
    if (window <= 0) return *this;
    if (is_series()) {
        auto s = as_series();
        auto sz = s.numel();
        if (sz < window) return *this;
        
        std::vector<double> result;
        result.reserve(sz - window + 1);
        auto s_double = s.to(at::kDouble);
        auto cont_s = s_double.contiguous();
        auto accessor = cont_s.accessor<double, 1>();
        
        for (int64_t i = 0; i <= sz - window; ++i) {
            double min_val = accessor[i];
            for (int64_t j = i + 1; j < i + window; ++j) {
                min_val = std::min(min_val, accessor[j]);
            }
            result.push_back(min_val);
        }
        
        return Feed(pluss::from_blob(result.data(), {static_cast<int64_t>(result.size())}, 
                                     pluss::SeriesOptions().dtype(at::kDouble)), name_);
    }
    return *this;
}

Feed Feed::rolling_max(int64_t window) const {
    if (window <= 0) return *this;
    if (is_series()) {
        auto s = as_series();
        auto sz = s.numel();
        if (sz < window) return *this;
        
        std::vector<double> result;
        result.reserve(sz - window + 1);
        auto s_double = s.to(at::kDouble);
        auto cont_s = s_double.contiguous();
        auto accessor = cont_s.accessor<double, 1>();
        
        for (int64_t i = 0; i <= sz - window; ++i) {
            double max_val = accessor[i];
            for (int64_t j = i + 1; j < i + window; ++j) {
                max_val = std::max(max_val, accessor[j]);
            }
            result.push_back(max_val);
        }
        
        return Feed(pluss::from_blob(result.data(), {static_cast<int64_t>(result.size())}, 
                                     pluss::SeriesOptions().dtype(at::kDouble)), name_);
    }
    return *this;
}

// ===== Shift and diff =====

Feed Feed::shift(int64_t periods) const {
    if (is_series()) {
        if (periods == 0) return *this;
        
        const auto sz = size();
        auto opts = SeriesOptions().dtype(as_series().dtype().toScalarType());
        if (periods > 0) {
            auto nans = pluss::full({periods}, std::nan(""), opts);
            auto trimmed = as_series().narrow(0, 0, sz - periods);
            auto combined = torch::cat({nans.data(), trimmed.data()}, 0);
            return Feed(pluss::from_torch(combined), name_);
        } else {
            auto nans = pluss::full({-periods}, std::nan(""), opts);
            auto trimmed = as_series().narrow(0, -periods, sz + periods);
            auto combined = torch::cat({trimmed.data(), nans.data()}, 0);
            return Feed(pluss::from_torch(combined), name_);
        }
    }
    return *this;
}

Feed Feed::diff(int64_t periods) const {
    if (is_series()) {
        if (periods <= 0 || size() <= periods) {
            auto opts = SeriesOptions().dtype(as_series().dtype().toScalarType());
            return Feed(pluss::full({size()}, std::nan(""), opts), name_);
        }
        auto shifted = shift(periods);
        return Feed(as_series().sub(shifted.as_series()), name_);
    }
    return *this;
}

Feed Feed::pct_change(int64_t periods) const {
    if (is_series()) {
        if (periods <= 0 || size() <= periods) {
            auto opts = SeriesOptions().dtype(as_series().dtype().toScalarType());
            return Feed(pluss::full({size()}, std::nan(""), opts), name_);
        }
        auto shifted = shift(periods);
        return Feed((as_series() - shifted.as_series()) / shifted.as_series(), name_);
    }
    return *this;
}

// ===== Comparison operations =====

pluss::Series Feed::eq(const Feed& other) const {
    if (is_series() && other.is_series()) {
        return as_series().eq(other.as_series());
    }
    return pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool));
}

pluss::Series Feed::ne(const Feed& other) const {
    if (is_series() && other.is_series()) {
        return as_series().ne(other.as_series());
    }
    return pluss::ones({size()}, pluss::SeriesOptions().dtype(at::kBool));
}

pluss::Series Feed::lt(const Feed& other) const {
    if (is_series() && other.is_series()) {
        return as_series().lt(other.as_series());
    }
    return pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool));
}

pluss::Series Feed::le(const Feed& other) const {
    if (is_series() && other.is_series()) {
        return as_series().le(other.as_series());
    }
    return pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool));
}

pluss::Series Feed::gt(const Feed& other) const {
    if (is_series() && other.is_series()) {
        return as_series().gt(other.as_series());
    }
    return pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool));
}

pluss::Series Feed::ge(const Feed& other) const {
    if (is_series() && other.is_series()) {
        return as_series().ge(other.as_series());
    }
    return pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool));
}

pluss::Series Feed::isin(const Feed& values) const {
    if (is_series() && values.is_series()) {
        auto s = as_series();
        auto vals = values.as_series();
        std::vector<bool> result;
        result.reserve(s.numel());
        
        auto s_double = s.to(at::kDouble);
        auto vals_double = vals.to(at::kDouble);
        auto s_cont = s_double.contiguous();
        auto vals_cont = vals_double.contiguous();
        auto s_accessor = s_cont.accessor<double, 1>();
        auto vals_accessor = vals_cont.accessor<double, 1>();
        
        for (int64_t i = 0; i < s.numel(); ++i) {
            bool found = false;
            for (int64_t j = 0; j < vals.numel(); ++j) {
                if (s_accessor[i] == vals_accessor[j]) {
                    found = true;
                    break;
                }
            }
            result.push_back(found);
        }
        
        // Convert vector<bool> to tensor
        std::vector<uint8_t> result_bytes(result.begin(), result.end());
        return pluss::from_blob(result_bytes.data(), {static_cast<int64_t>(result.size())}, 
                               pluss::SeriesOptions().dtype(at::kBool)).clone();
    }
    return pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool));
}

pluss::Series Feed::between(double lower, double upper) const {
    if (is_series()) {
        return as_series().ge(lower).logical_and(as_series().le(upper));
    }
    return pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool));
}

// ===== Type conversion =====

Feed Feed::to_numeric() const {
    if (is_series()) return *this;
    if (is_timeline()) return Feed(to_series(), name_);
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::vector<double> numbers;
            for (const auto& chord : vec) {
                try {
                    numbers.push_back(std::stod(chord.str()));
                } catch (...) {
                    numbers.push_back(std::nan(""));
                }
            }
            return Feed(pluss::from_blob(numbers.data(), {static_cast<int64_t>(numbers.size())}, 
                                         pluss::SeriesOptions().dtype(at::kDouble)), name_);
        } catch (...) {}
    }
    return *this;
}

Feed Feed::as_text() const {
    if (is_note()) return *this;
    if (is_series()) {
        try {
            auto s_double = as_series().to(at::kDouble);
            auto cont_series = s_double.contiguous();
            auto accessor = cont_series.accessor<double, 1>();
            std::vector<pluss::Chord> chords;
            for (int64_t i = 0; i < as_series().numel(); ++i) {
                chords.emplace_back(std::to_string(accessor[i]));
            }
            return Feed(pluss::make_note(chords), name_);
        } catch (...) {}
    }
    return *this;
}

Feed Feed::to_datetime() const {
    // Placeholder - would need Timeline conversion logic
    return *this;
}

// ===== Reshaping =====

Feed Feed::flatten() const {
    if (is_series()) {
        return Feed(as_series().flatten(), name_);
    }
    if (is_note()) {
        return *this;
    }
    return *this;
}

Feed Feed::reshape(const std::vector<int64_t>& shape) const {
    if (is_series()) {
        return Feed(as_series().reshape(shape), name_);
    }
    return *this;
}

// ===== Sampling and randomization =====

Feed Feed::sample(int64_t n, bool replacement) const {
    if (n <= 0 || size() == 0) return Feed(pluss::zeros({0}), name_);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    if (replacement) {
        std::uniform_int_distribution<int64_t> dist(0, size() - 1);
        std::vector<int64_t> indices;
        for (int64_t i = 0; i < n; ++i) {
            indices.push_back(dist(gen));
        }
        
        if (is_series()) {
            auto idx_tensor = pluss::from_blob(indices.data(), {n}, pluss::SeriesOptions().dtype(at::kLong));
            return Feed(as_series().index({idx_tensor}), name_);
        }
    } else {
        std::vector<int64_t> indices;
        for (int64_t i = 0; i < size(); ++i) indices.push_back(i);
        std::shuffle(indices.begin(), indices.end(), gen);
        indices.resize(std::min(n, size()));
        
        if (is_series()) {
            auto idx_tensor = pluss::from_blob(indices.data(), {static_cast<int64_t>(indices.size())}, 
                                              pluss::SeriesOptions().dtype(at::kLong));
            return Feed(as_series().index({idx_tensor}), name_);
        }
    }
    
    return *this;
}

Feed Feed::shuffle() const {
    return sample(size(), false);
}

// ===== Value counts and binning =====

std::unordered_map<std::string, int64_t> Feed::value_counts() const {
    std::unordered_map<std::string, int64_t> counts;
    
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            for (const auto& chord : vec) {
                counts[chord.str()]++;
            }
        } catch (...) {}
    } else if (is_series()) {
        try {
            auto s = as_series();
            auto s_double = s.to(at::kDouble);
            auto cont_s = s_double.contiguous();
            auto accessor = cont_s.accessor<double, 1>();
            for (int64_t i = 0; i < s.numel(); ++i) {
                counts[std::to_string(accessor[i])]++;
            }
        } catch (...) {}
    }
    
    return counts;
}

Feed Feed::bin(int64_t n_bins) const {
    if (n_bins <= 0 || !is_series()) return *this;
    
    auto s = as_series();
    auto min_val = s.min().item<double>();
    auto max_val = s.max().item<double>();
    
    if (min_val == max_val) return Feed(pluss::zeros_like(s), name_);
    
    double bin_width = (max_val - min_val) / n_bins;
    auto s_double = s.to(at::kDouble);
    auto cont_s = s_double.contiguous();
    auto accessor = cont_s.accessor<double, 1>();
    std::vector<int64_t> bins;
    
    for (int64_t i = 0; i < s.numel(); ++i) {
        int64_t bin = static_cast<int64_t>((accessor[i] - min_val) / bin_width);
        bins.push_back(std::min(bin, n_bins - 1));
    }
    
    return Feed(pluss::from_blob(bins.data(), {static_cast<int64_t>(bins.size())}, 
                                pluss::SeriesOptions().dtype(at::kLong)), name_);
}

Feed Feed::cut(const std::vector<double>& edges) const {
    if (edges.size() < 2 || !is_series()) return *this;
    
    auto s = as_series();
    auto s_double = s.to(at::kDouble);
    auto cont_s = s_double.contiguous();
    auto accessor = cont_s.accessor<double, 1>();
    std::vector<int64_t> bins;
    
    for (int64_t i = 0; i < as_series().numel(); ++i) {
        double val = accessor[i];
        int64_t bin = -1;
        for (size_t j = 0; j < edges.size() - 1; ++j) {
            if (val >= edges[j] && val < edges[j + 1]) {
                bin = j;
                break;
            }
        }
        if (val == edges.back()) bin = edges.size() - 2;
        bins.push_back(bin);
    }
    
    return Feed(pluss::from_blob(bins.data(), {static_cast<int64_t>(bins.size())}, 
                                pluss::SeriesOptions().dtype(at::kLong)), name_);
}

// ===== Display and conversion =====

void Feed::display(int64_t max_rows) const {
    std::cout << "Feed: " << name_ << "\n";
    std::cout << "Type: ";
    if (is_series()) std::cout << "Series";
    else if (is_note()) std::cout << "Note";
    else if (is_timeline()) std::cout << "Timeline";
    std::cout << "\n";
    std::cout << "Size: " << size() << "\n";
    
    if (is_series()) {
        auto s = as_series().to(at::kDouble);  // Ensure double type
        std::cout << "Values: ";
        auto cont_s = s.contiguous();
        auto accessor = cont_s.accessor<double, 1>();
        for (int64_t i = 0; i < std::min(max_rows, s.numel()); ++i) {
            std::cout << accessor[i];
            if (i < std::min(max_rows, s.numel()) - 1) std::cout << ", ";
        }
        if (s.numel() > max_rows) std::cout << "...";
        std::cout << "\n";
    } else if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::cout << "Values: ";
            for (size_t i = 0; i < std::min(static_cast<size_t>(max_rows), vec.size()); ++i) {
                std::cout << vec[i].str();
                if (i < std::min(static_cast<size_t>(max_rows), vec.size()) - 1) std::cout << ", ";
            }
            if (vec.size() > static_cast<size_t>(max_rows)) std::cout << "...";
            std::cout << "\n";
        } catch (...) {}
    }
}

void Feed::info() const {
    std::cout << "=== Feed Info ===\n";
    std::cout << "Name: " << name_ << "\n";
    std::cout << "Type: " << dtype() << "\n";
    std::cout << "Size: " << size() << "\n";
    std::cout << "Dimension: " << dim() << "\n";
    
    if (is_series()) {
        std::cout << "Min: " << min() << "\n";
        std::cout << "Max: " << max() << "\n";
        std::cout << "Mean: " << mean() << "\n";
        std::cout << "Std: " << std() << "\n";
    }
}

std::string Feed::dtype() const {
    if (is_series()) return "Series";
    if (is_note()) return "Note";
    if (is_timeline()) return "Timeline";
    return "Unknown";
}

std::string Feed::to_string() const {
    std::ostringstream oss;
    oss << "Feed(" << name_ << ", " << dtype() << ", size=" << size() << ")";
    return oss.str();
}

std::vector<double> Feed::to_vector_double() const {
    std::vector<double> result;
    if (is_series()) {
        auto s = as_series();
        auto s_double = s.to(at::kDouble);
        auto cont_s = s_double.contiguous();
        auto accessor = cont_s.accessor<double, 1>();
        for (int64_t i = 0; i < s.numel(); ++i) {
            result.push_back(accessor[i]);
        }
    }
    return result;
}

std::vector<std::string> Feed::to_vector_string() const {
    std::vector<std::string> result;
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            for (const auto& chord : vec) {
                result.push_back(chord.str());
            }
        } catch (...) {}
    }
    return result;
}

// ===== Serialization =====

std::string Feed::to_json() const {
    std::ostringstream oss;
    oss << "{\"name\":\"" << name_ << "\",\"type\":\"" << dtype() << "\",\"size\":" << size() << ",\"values\":[";
    
    if (is_series()) {
        auto s = as_series();
        auto s_double = s.to(at::kDouble);
        auto cont_s = s_double.contiguous();
        auto accessor = cont_s.accessor<double, 1>();
        for (int64_t i = 0; i < s.numel(); ++i) {
            oss << accessor[i];
            if (i < as_series().numel() - 1) oss << ",";
        }
    } else if (is_note()) {
        try {
            auto vec = as_note().vector();
            for (size_t i = 0; i < vec.size(); ++i) {
                oss << "\"" << vec[i].str() << "\"";
                if (i < vec.size() - 1) oss << ",";
            }
        } catch (...) {}
    }
    
    oss << "]}";
    return oss.str();
}

Feed Feed::from_json(const std::string& /*json*/, const std::string& name) {
    // Placeholder - would need proper JSON parsing
    return Feed(pluss::zeros({0}), name);
}

}  // namespace pluss::handle
