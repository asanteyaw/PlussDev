#include <pluss/datalog/table/thread/thread.h>
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

namespace pluss::table {

// Professional, efficient, vectorized Thread operations using pluss API

Thread Thread::head(int64_t n) const {
    if (n <= 0 || size() == 0) return Thread(pluss::zeros({0}), name_);
    const int64_t actual_n = std::min(n, size());
    
    if (is_series()) {
        return Thread(as_series().narrow(0, 0, actual_n), name_);
    }
    if (is_note()) {
        return Thread(pluss::ops::slice(as_note(), 0, 0, static_cast<int>(actual_n)), name_);
    }
    return *this;
}

Thread Thread::tail(int64_t n) const {
    if (n <= 0 || size() == 0) return Thread(pluss::zeros({0}), name_);
    const int64_t sz = size();
    const int64_t actual_n = std::min(n, sz);
    const int64_t start = sz - actual_n;
    
    if (is_series()) {
        return Thread(as_series().narrow(0, start, actual_n), name_);
    }
    if (is_note()) {
        return Thread(pluss::ops::slice(as_note(), 0, static_cast<int>(start), static_cast<int>(sz)), name_);
    }
    return *this;
}

Thread Thread::iloc(int64_t start, int64_t end) const {
    if (start < 0 || end <= start || size() == 0) return Thread(pluss::zeros({0}), name_);
    const int64_t sz = size();
    const int64_t actual_start = std::min(start, sz);
    const int64_t actual_end = std::min(end, sz);
    const int64_t length = actual_end - actual_start;
    
    if (is_series()) {
        return Thread(as_series().narrow(0, actual_start, length), name_);
    }
    if (is_note()) {
        return Thread(pluss::ops::slice(as_note(), 0, static_cast<int>(actual_start), static_cast<int>(actual_end)), name_);
    }
    return *this;
}

Thread Thread::loc(const pluss::Series& condition) const {
    if (is_series()) {
        return Thread(as_series().index({condition}), name_);
    }
    if (is_timeline()) {
        return Thread(as_timeline().to_Series().index({condition}), name_);
    }
    if (is_note()) {
        try {
            // Vectorized approach: use boolean masking
            auto vec = as_note().vector();
            const auto cond_numel = condition.numel();
            const auto vec_size = static_cast<int64_t>(vec.size());
            const auto min_size = std::min(cond_numel, vec_size);
            
            // Count true values for pre-allocation (vectorized counting)
            auto cond_contig = condition.contiguous();
            auto cond_accessor = cond_contig.accessor<bool, 1>();
            int64_t true_count = 0;
            for (int64_t i = 0; i < min_size; ++i) {
                true_count += cond_accessor[i];
            }
            
            // Pre-allocate for vectorized assignment
            std::vector<pluss::Chord> chords;
            chords.reserve(true_count);
            
            // Vectorized selection with minimal branching
            for (int64_t i = 0; i < min_size; ++i) {
                if (cond_accessor[i]) {
                    chords.emplace_back(vec[i]);
                }
            }
            
            return Thread(pluss::make_note(chords), name_);
        } catch (...) {
            return *this;
        }
    }
    return *this;
}

Thread Thread::to(const torch::Device& device) const {
    if (is_series()) {
        return Thread(pluss::from_torch(as_series().data().to(device)), name_);
    }
    if (is_timeline()) {
        return Thread(pluss::from_torch(as_timeline().to_Series().data().to(device)), name_);
    }
    return *this;
}

Thread Thread::unique() const {
    if (is_series()) {
        // Efficient unique using sort + vectorized consecutive comparison
        auto sorted_vals = std::get<0>(as_series().sort(0));
        int64_t numel = sorted_vals.numel();
        if (numel == 0) return *this;
        if (numel == 1) return Thread(sorted_vals, name_);
        
        // Vectorized comparison: compare sorted[1:] != sorted[:-1]
        auto shifted = sorted_vals.slice(0, 0, numel - 1);  // all but last
        auto current = sorted_vals.slice(0, 1, numel);      // all but first
        auto diff_mask = current.ne(shifted);  // vectorized comparison
        
        // Include first element (always unique in sorted array)
        auto first_true = pluss::ones({1}, pluss::SeriesOptions().dtype(at::kBool));
        auto keep_mask = pluss::from_torch(torch::cat({first_true.tensor(), diff_mask.tensor()}, 0));
        
        return Thread(sorted_vals.masked_select(keep_mask), name_);
    }
    if (is_timeline()) {
        return Thread(as_timeline().to_Series(), name_).unique();
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
            return Thread(pluss::make_note(chords), name_);
        } catch (...) {
            return *this;
        }
    }
    return *this;
}

Thread Thread::sort_values(bool ascending) const {
    if (is_series()) {
        auto sorted_vals = std::get<0>(as_series().sort(0, !ascending));  // PyTorch sort uses descending parameter
        return Thread(sorted_vals, name_);
    }
    if (is_timeline()) {
        auto s = as_timeline().to_Series();
        auto sorted_vals = std::get<0>(s.sort(0, !ascending));  // PyTorch sort uses descending parameter
        return Thread(sorted_vals, name_);
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
            return Thread(pluss::make_note(chords), name_);
        } catch (...) {
            return *this;
        }
    }
    return *this;
}

Thread Thread::fillna(const Thread& value) const {
    if (is_series()) {
        double fill_val = value.is_series() ? value.as_series().item<double>() : 0.0;
        return Thread(as_series().nan_to_num(fill_val), name_);
    }
    return *this;
}

Thread Thread::dropna() const {
    if (is_series()) {
        auto mask = as_series().isnan().logical_not();
        return Thread(as_series().index({mask}), name_);
    }
    return *this;
}

pluss::Series Thread::isna() const {
    if (is_series()) {
        return as_series().isnan();
    }
    return pluss::zeros({size()}).to(torch::kBool);
}

pluss::Series Thread::notna() const {
    if (is_series()) {
        return as_series().isnan().logical_not();
    }
    return pluss::ones({size()}).to(torch::kBool);
}

Thread Thread::value_counts() const {
    if (is_series()) {
        // Return unique values as proxy for value counts
        return unique();
    }
    if (is_note()) {
        try {
            auto vec = as_note().vector();
            std::unordered_map<std::string, int64_t> counts;
            for (const auto& chord : vec) counts[chord.str()]++;
            
            std::vector<int64_t> count_vals;
            for (const auto& [_, count] : counts) count_vals.push_back(count);
            
            // Use from_blob to create Series directly from vector data
            return Thread(pluss::from_blob(count_vals.data(), {static_cast<int64_t>(count_vals.size())}, pluss::SeriesOptions().dtype(at::kLong)), name_ + "_counts");
        } catch (...) {
            return *this;
        }
    }
    return *this;
}

Thread Thread::rank(const std::string& /*method*/) const {
    if (is_series()) {
        auto ranks = pluss::arange(1, size() + 1, pluss::config({{"dtype", "float64"}}));
        return Thread(ranks, name_);
    }
    return *this;
}

Thread Thread::diff(int64_t periods) const {
    if (is_series()) {
        if (periods <= 0 || size() <= periods) {
            auto opts = SeriesOptions().dtype(as_series().dtype().toScalarType());
            return Thread(pluss::full({size()}, std::nan(""), opts), name_);
        }
        auto shifted = shift(periods);
        return Thread(as_series().sub(shifted.as_series()), name_);
    }
    return *this;
}

Thread Thread::shift(int64_t periods) const {
    if (is_series()) {
        if (periods == 0) return *this;
        
        const auto sz = size();
        auto opts = SeriesOptions().dtype(as_series().dtype().toScalarType());
        if (periods > 0) {
            // Create NaN tensor matching the dtype of the series
            auto nans = pluss::full({periods}, std::nan(""), opts);
            auto trimmed = as_series().narrow(0, 0, sz - periods);
            auto combined = torch::cat({nans.data(), trimmed.data()}, 0);
            return Thread(pluss::from_torch(combined), name_);
        } else {
            auto nans = pluss::full({-periods}, std::nan(""), opts);
            auto trimmed = as_series().narrow(0, -periods, sz + periods);
            auto combined = torch::cat({trimmed.data(), nans.data()}, 0);
            return Thread(pluss::from_torch(combined), name_);
        }
    }
    return *this;
}

Thread Thread::cumsum() const {
    if (is_series()) {
        return Thread(as_series().cumsum(0), name_);
    }
    return *this;
}

Thread Thread::cumprod() const {
    if (is_series()) {
        return Thread(as_series().cumprod(0), name_);
    }
    return *this;
}

Thread Thread::cummax() const {
    if (is_series()) {
        auto result = std::get<0>(as_series().cummax(0));
        return Thread(result, name_);
    }
    return *this;
}

Thread Thread::cummin() const {
    if (is_series()) {
        auto result = std::get<0>(as_series().cummin(0));
        return Thread(result, name_);
    }
    return *this;
}

// ========== NEW THREAD OPERATIONS (50+ Methods) ==========

// Indexing and selection operations
Thread Thread::take(const std::vector<int64_t>& indices) const {
    if (is_series()) {
        auto idx_tensor = pluss::value(indices);
        return Thread(as_series().index({idx_tensor}), name_);
    }
    return *this;
}

Thread Thread::where(const pluss::Series& condition, const Thread& other) const {
    if (is_series() && other.is_series()) {
        return Thread(as_series().where(condition, other.as_series()), name_);
    }
    return *this;
}

Thread Thread::mask(const pluss::Series& condition, const Thread& other) const {
    if (is_series() && other.is_series()) {
        return Thread(as_series().masked_fill(condition, other.as_series()[0].item<double>()), name_);
    }
    return *this;
}

// Mathematical transformations
Thread Thread::clip(double lower, double upper) const {
    if (is_series()) {
        return Thread(as_series().clamp(lower, upper), name_);
    }
    return *this;
}

Thread Thread::abs() const {
    if (is_series()) {
        return Thread(as_series().abs(), name_);
    }
    return *this;
}

Thread Thread::round(int decimals) const {
    if (is_series()) {
        double scale = std::pow(10.0, decimals);
        return Thread(as_series().mul(scale).round().div(scale), name_);
    }
    return *this;
}

Thread Thread::replace(double old_value, double new_value) const {
    if (is_series()) {
        auto mask = as_series().eq(old_value);
        return Thread(as_series().masked_fill(mask, new_value), name_);
    }
    return *this;
}

// Forward and backward fill
Thread Thread::ffill() const {
    if (is_series()) {
        // Forward fill: propagate last valid value forward
        auto data = as_series();
        auto result = data.clone();
        
        if (result.dtype() == torch::kFloat32) {
            auto accessor = result.accessor<float, 1>();
            for (int64_t i = 1; i < size(); ++i) {
                if (std::isnan(accessor[i])) {
                    accessor[i] = accessor[i-1];
                }
            }
        } else if (result.dtype() == torch::kFloat64) {
            auto accessor = result.accessor<double, 1>();
            for (int64_t i = 1; i < size(); ++i) {
                if (std::isnan(accessor[i])) {
                    accessor[i] = accessor[i-1];
                }
            }
        }
        
        return Thread(result, name_);
    }
    return *this;
}

Thread Thread::bfill() const {
    if (is_series()) {
        // Backward fill: propagate next valid value backward
        auto data = as_series();
        auto result = data.clone();
        
        if (result.dtype() == torch::kFloat32) {
            auto accessor = result.accessor<float, 1>();
            for (int64_t i = size() - 2; i >= 0; --i) {
                if (std::isnan(accessor[i])) {
                    accessor[i] = accessor[i+1];
                }
            }
        } else if (result.dtype() == torch::kFloat64) {
            auto accessor = result.accessor<double, 1>();
            for (int64_t i = size() - 2; i >= 0; --i) {
                if (std::isnan(accessor[i])) {
                    accessor[i] = accessor[i+1];
                }
            }
        }
        
        return Thread(result, name_);
    }
    return *this;
}

Thread Thread::interpolate(const std::string& method) const {
    if (is_series() && method == "linear") {
        // Simple linear interpolation
        auto data = as_series();
        std::vector<double> result_vec;
        result_vec.reserve(size());
        
        for (int64_t i = 0; i < size(); ++i) {
            double val = data[i].item<double>();
            if (!std::isnan(val)) {
                result_vec.push_back(val);
            } else {
                // Find previous and next valid values
                int64_t prev = i - 1;
                while (prev >= 0 && std::isnan(data[prev].item<double>())) prev--;
                
                int64_t next = i + 1;
                while (next < size() && std::isnan(data[next].item<double>())) next++;
                
                if (prev >= 0 && next < size()) {
                    double prev_val = data[prev].item<double>();
                    double next_val = data[next].item<double>();
                    double weight = static_cast<double>(i - prev) / (next - prev);
                    result_vec.push_back(prev_val + weight * (next_val - prev_val));
                } else {
                    result_vec.push_back(std::nan(""));
                }
            }
        }
        // Use from_blob to create Series directly from vector data
        return Thread(pluss::from_blob(result_vec.data(), {static_cast<int64_t>(result_vec.size())}), name_);
    }
    return *this;
}

// Statistical operations
Thread Thread::pct_change(int64_t periods) const {
    if (is_series()) {
        auto shifted = shift(periods);
        if (shifted.is_series()) {
            return Thread(as_series().sub(shifted.as_series()).div(shifted.as_series()), name_);
        }
    }
    return *this;
}

Thread Thread::nlargest(int64_t n) const {
    if (is_series()) {
        auto [sorted_vals, indices] = as_series().sort(0, true);  // Descending
        return Thread(sorted_vals.narrow(0, 0, std::min(n, size())), name_);
    }
    return *this;
}

Thread Thread::nsmallest(int64_t n) const {
    if (is_series()) {
        auto [sorted_vals, indices] = as_series().sort(0, false);  // Ascending
        return Thread(sorted_vals.narrow(0, 0, std::min(n, size())), name_);
    }
    return *this;
}

// Comparison operations
pluss::Series Thread::eq(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return as_series().eq(other.as_series());
    }
    return pluss::zeros({size()}).to(torch::kBool);
}

pluss::Series Thread::ne(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return as_series().ne(other.as_series());
    }
    return pluss::ones({size()}).to(torch::kBool);
}

pluss::Series Thread::lt(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return as_series().lt(other.as_series());
    }
    return pluss::zeros({size()}).to(torch::kBool);
}

pluss::Series Thread::le(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return as_series().le(other.as_series());
    }
    return pluss::zeros({size()}).to(torch::kBool);
}

pluss::Series Thread::gt(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return as_series().gt(other.as_series());
    }
    return pluss::zeros({size()}).to(torch::kBool);
}

pluss::Series Thread::ge(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return as_series().ge(other.as_series());
    }
    return pluss::zeros({size()}).to(torch::kBool);
}

// Arithmetic operations
Thread Thread::add(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return Thread(as_series().add(other.as_series()), name_);
    }
    return *this;
}

Thread Thread::sub(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return Thread(as_series().sub(other.as_series()), name_);
    }
    return *this;
}

Thread Thread::mul(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return Thread(as_series().mul(other.as_series()), name_);
    }
    return *this;
}

Thread Thread::div(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return Thread(as_series().div(other.as_series()), name_);
    }
    return *this;
}

Thread Thread::mod(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return Thread(as_series().remainder(other.as_series()), name_);
    }
    return *this;
}

Thread Thread::pow(const Thread& other) const {
    if (is_series() && other.is_series()) {
        return Thread(as_series().pow(other.as_series()), name_);
    }
    return *this;
}

// Mathematical functions
Thread Thread::sqrt() const {
    if (is_series()) {
        return Thread(as_series().sqrt(), name_);
    }
    return *this;
}

Thread Thread::log() const {
    if (is_series()) {
        return Thread(as_series().log(), name_);
    }
    return *this;
}

Thread Thread::log10() const {
    if (is_series()) {
        return Thread(as_series().log10(), name_);
    }
    return *this;
}

Thread Thread::exp() const {
    if (is_series()) {
        return Thread(as_series().exp(), name_);
    }
    return *this;
}

Thread Thread::sin() const {
    if (is_series()) {
        return Thread(as_series().sin(), name_);
    }
    return *this;
}

Thread Thread::cos() const {
    if (is_series()) {
        return Thread(as_series().cos(), name_);
    }
    return *this;
}

Thread Thread::tan() const {
    if (is_series()) {
        return Thread(as_series().tan(), name_);
    }
    return *this;
}

// ========== AGGREGATION OPERATIONS ==========

double Thread::sum() const {
    if (is_series()) {
        return as_series().sum().item<double>();
    }
    return 0.0;
}

double Thread::mean() const {
    if (is_series()) {
        return as_series().mean().item<double>();
    }
    return 0.0;
}

double Thread::median() const {
    if (is_series()) {
        return as_series().median().item<double>();
    }
    return 0.0;
}

double Thread::std(int ddof) const {
    if (is_series()) {
        return as_series().std(ddof != 0).item<double>();
    }
    return 0.0;
}

double Thread::var(int ddof) const {
    if (is_series()) {
        return as_series().var(ddof != 0).item<double>();
    }
    return 0.0;
}

double Thread::min() const {
    if (is_series()) {
        return as_series().min().item<double>();
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double Thread::max() const {
    if (is_series()) {
        return as_series().max().item<double>();
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double Thread::prod() const {
    if (is_series()) {
        return as_series().prod().item<double>();
    }
    return 1.0;
}

double Thread::sem() const {
    if (is_series()) {
        const auto s = as_series();
        const int64_t n = s.numel();
        if (n <= 1) return std::numeric_limits<double>::quiet_NaN();
        return s.std(true).item<double>() / std::sqrt(static_cast<double>(n));
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double Thread::mad() const {
    if (is_series()) {
        const auto s = as_series();
        const double m = s.mean().item<double>();
        return (s - m).abs().mean().item<double>();
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double Thread::skew() const {
    if (is_series()) {
        const auto s = as_series();
        const int64_t n = s.numel();
        if (n < 3) return std::numeric_limits<double>::quiet_NaN();
        
        const double m = s.mean().item<double>();
        const double sigma = s.std(true).item<double>();
        if (sigma == 0.0) return 0.0;
        
        const auto centered = s - m;
        const double m3 = centered.pow(3).mean().item<double>();
        return m3 / std::pow(sigma, 3);
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double Thread::kurt() const {
    if (is_series()) {
        const auto s = as_series();
        const int64_t n = s.numel();
        if (n < 4) return std::numeric_limits<double>::quiet_NaN();
        
        const double m = s.mean().item<double>();
        const double sigma = s.std(true).item<double>();
        if (sigma == 0.0) return -3.0;  // Excess kurtosis for constant series
        
        const auto centered = s - m;
        const double m4 = centered.pow(4).mean().item<double>();
        return (m4 / std::pow(sigma, 4)) - 3.0;  // Excess kurtosis
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double Thread::quantile(double q) const {
    if (is_series()) {
        return as_series().quantile(q).item<double>();
    }
    return std::numeric_limits<double>::quiet_NaN();
}

int64_t Thread::count() const {
    if (is_series()) {
        // Count non-NaN values
        const auto s = as_series();
        return (s.data() == s.data()).sum().item<int64_t>();  // NaN != NaN
    }
    if (is_note()) {
        return as_note().shape()[0];
    }
    if (is_timeline()) {
        return size();
    }
    return 0;
}

int64_t Thread::nunique() const {
    if (is_series()) {
        const auto s = as_series();
        
        // Remove NaN values using Series API
        auto nan_mask = s.isnan();
        auto clean_mask = nan_mask.logical_not();
        auto clean = s.masked_select(clean_mask);
        
        if (clean.numel() == 0) return 0;
        
        // Use unordered_set for unique counting
        std::unordered_set<double> unique_vals;
        auto clean_contig = clean.contiguous();
        auto accessor = clean_contig.accessor<double, 1>();
        for (int64_t i = 0; i < clean.numel(); ++i) {
            unique_vals.insert(accessor[i]);
        }
        return static_cast<int64_t>(unique_vals.size());
    }
    if (is_note()) {
        const auto vec = as_note().vector();
        std::unordered_set<std::string> unique_vals;
        for (const auto& chord : vec) {
            unique_vals.insert(chord.str());
        }
        return static_cast<int64_t>(unique_vals.size());
    }
    return count();
}

// ========== INDEX LOCATION METHODS ==========

int64_t Thread::idxmax() const {
    if (is_series()) {
        return std::get<1>(as_series().max(0)).item<int64_t>();
    }
    return -1;
}

int64_t Thread::idxmin() const {
    if (is_series()) {
        return std::get<1>(as_series().min(0)).item<int64_t>();
    }
    return -1;
}

Thread Thread::mode() const {
    if (is_series()) {
        const auto s = as_series();
        
        // Remove NaN values using Series API
        auto nan_mask = s.isnan();
        auto clean_mask = nan_mask.logical_not();
        auto clean = s.masked_select(clean_mask);
        
        if (clean.numel() == 0) {
            return Thread(pluss::full({1}, std::numeric_limits<double>::quiet_NaN()), name_);
        }
        
        // Count frequencies
        std::unordered_map<double, int64_t> freq_map;
        auto clean_contig = clean.contiguous();
        auto accessor = clean_contig.accessor<double, 1>();
        for (int64_t i = 0; i < clean.numel(); ++i) {
            freq_map[accessor[i]]++;
        }
        
        // Find max frequency
        int64_t max_freq = 0;
        for (const auto& [val, freq] : freq_map) {
            max_freq = std::max(max_freq, freq);
        }
        
        // Collect all values with max frequency
        std::vector<double> modes;
        for (const auto& [val, freq] : freq_map) {
            if (freq == max_freq) {
                modes.push_back(val);
            }
        }
        
        // Sort modes for deterministic output
        std::sort(modes.begin(), modes.end());
        
        return Thread(pluss::from_blob(modes.data(), {static_cast<int64_t>(modes.size())}), name_);
    }
    
    if (is_note()) {
        const auto vec = as_note().vector();
        if (vec.empty()) {
            return Thread(pluss::make_note(), name_);
        }
        
        std::unordered_map<std::string, int64_t> freq_map;
        for (const auto& chord : vec) {
            freq_map[chord.str()]++;
        }
        
        int64_t max_freq = 0;
        for (const auto& [val, freq] : freq_map) {
            max_freq = std::max(max_freq, freq);
        }
        
        std::vector<std::string> modes;
        for (const auto& [val, freq] : freq_map) {
            if (freq == max_freq) {
                modes.push_back(val);
            }
        }
        
        std::sort(modes.begin(), modes.end());
        
        std::vector<pluss::Chord> mode_chords;
        for (const auto& m : modes) {
            mode_chords.emplace_back(m);
        }
        
        return Thread(pluss::make_note(mode_chords), name_);
    }
    
    return *this;
}

// ========== BOOLEAN REDUCTIONS ==========

bool Thread::all() const {
    if (is_series()) {
        return as_series().all().item<bool>();
    }
    return false;
}

bool Thread::any() const {
    if (is_series()) {
        return as_series().any().item<bool>();
    }
    return false;
}

// ========== SELECTION AND FILTERING ==========

Thread Thread::sample(int64_t n, bool replace) const {
    if (n <= 0 || size() == 0) return Thread(pluss::zeros({0}), name_);
    
    const int64_t sz = size();
    std::vector<int64_t> indices;
    
    if (replace) {
        // Sample with replacement
        indices.reserve(n);
        for (int64_t i = 0; i < n; ++i) {
            indices.push_back(std::rand() % sz);
        }
    } else {
        // Sample without replacement
        if (n > sz) n = sz;
        indices.reserve(sz);
        for (int64_t i = 0; i < sz; ++i) {
            indices.push_back(i);
        }
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(indices.begin(), indices.end(), gen);
        indices.resize(n);
    }
    
    return take(indices);
}

Thread Thread::truncate(int64_t before, int64_t after) const {
    const int64_t sz = size();
    if (before < 0) before = 0;
    if (after > sz) after = sz;
    if (before >= after) return Thread(pluss::zeros({0}), name_);
    
    return iloc(before, after);
}

Thread Thread::drop_duplicates() const {
    if (is_series()) {
        const auto s = as_series();
        auto t = s.data();
        
        std::unordered_set<double> seen;
        std::vector<int64_t> keep_indices;
        
        auto accessor = t.accessor<double, 1>();
        for (int64_t i = 0; i < t.numel(); ++i) {
            double val = accessor[i];
            // Handle NaN specially (each NaN is considered unique)
            if (std::isnan(val) || seen.find(val) == seen.end()) {
                seen.insert(val);
                keep_indices.push_back(i);
            }
        }
        
        return take(keep_indices);
    }
    
    if (is_note()) {
        const auto vec = as_note().vector();
        std::unordered_set<std::string> seen;
        std::vector<pluss::Chord> unique_chords;
        
        for (const auto& chord : vec) {
            const std::string s = chord.str();
            if (seen.find(s) == seen.end()) {
                seen.insert(s);
                unique_chords.push_back(chord);
            }
        }
        
        return Thread(pluss::make_note(unique_chords), name_);
    }
    
    return *this;
}

pluss::Series Thread::duplicated() const {
    if (is_series()) {
        const auto s = as_series();
        auto t = s.data();
        
        std::unordered_set<double> seen;
        std::vector<bool> is_dup;
        is_dup.reserve(t.numel());
        
        auto accessor = t.accessor<double, 1>();
        for (int64_t i = 0; i < t.numel(); ++i) {
            double val = accessor[i];
            if (std::isnan(val)) {
                // Each NaN is considered unique (not a duplicate)
                is_dup.push_back(false);
            } else if (seen.find(val) != seen.end()) {
                is_dup.push_back(true);
            } else {
                seen.insert(val);
                is_dup.push_back(false);
            }
        }
        
        // Convert bool vector to Series using pluss types
        // Use uint8 intermediate because bool accessor is deleted
        std::vector<uint8_t> is_dup_u8(is_dup.begin(), is_dup.end());
        auto result = pluss::from_blob(is_dup_u8.data(), {static_cast<int64_t>(is_dup_u8.size())}, pluss::SeriesOptions().dtype(at::kByte));
        return result.to(at::kBool);
    }
    
    // For non-Series, return all false
    return pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool));
}

pluss::Series Thread::isin(const std::vector<double>& values) const {
    if (is_series()) {
        const auto s = as_series();
        auto t = s.data();
        
        std::unordered_set<double> value_set(values.begin(), values.end());
        
        std::vector<bool> result_vec;
        result_vec.reserve(t.numel());
        auto accessor = t.accessor<double, 1>();
        
        for (int64_t i = 0; i < t.numel(); ++i) {
            result_vec.push_back(value_set.find(accessor[i]) != value_set.end());
        }
        
        // Use uint8 intermediate because bool accessor is deleted
        std::vector<uint8_t> result_u8(result_vec.begin(), result_vec.end());
        auto result = pluss::from_blob(result_u8.data(), {static_cast<int64_t>(result_u8.size())}, pluss::SeriesOptions().dtype(at::kByte));
        return result.to(at::kBool);
    }
    
    return pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool));
}

pluss::Series Thread::isin(const Thread& values) const {
    if (is_series() && values.is_series()) {
        const auto other_t = values.as_series().data();
        
        std::unordered_set<double> value_set;
        auto other_accessor = other_t.accessor<double, 1>();
        for (int64_t i = 0; i < other_t.numel(); ++i) {
            value_set.insert(other_accessor[i]);
        }
        
        const auto s = as_series();
        auto t = s.data();
        
        std::vector<bool> result_vec;
        result_vec.reserve(t.numel());
        auto accessor = t.accessor<double, 1>();
        
        for (int64_t i = 0; i < t.numel(); ++i) {
            result_vec.push_back(value_set.find(accessor[i]) != value_set.end());
        }
        
        // Use uint8 intermediate because bool accessor is deleted
        std::vector<uint8_t> result_u8(result_vec.begin(), result_vec.end());
        auto result = pluss::from_blob(result_u8.data(), {static_cast<int64_t>(result_u8.size())}, pluss::SeriesOptions().dtype(at::kByte));
        return result.to(at::kBool);
    }
    
    return pluss::zeros({size()}, pluss::SeriesOptions().dtype(at::kBool));
}

// ========== TRANSFORMATIONS ==========

Thread Thread::transform(const std::string& func) const {
    if (is_series()) {
        const auto s = as_series();
        
        if (func == "sqrt") return sqrt();
        if (func == "log") return log();
        if (func == "log10") return log10();
        if (func == "exp") return exp();
        if (func == "sin") return sin();
        if (func == "cos") return cos();
        if (func == "tan") return tan();
        if (func == "abs") return abs();
        if (func == "square") return Thread(s.pow(2), name_);
        if (func == "cube") return Thread(s.pow(3), name_);
        
        // Normalization functions
        if (func == "normalize" || func == "zscore") {
            const double m = s.mean().item<double>();
            const double sigma = s.std(true).item<double>();
            if (sigma == 0.0) return Thread(pluss::zeros_like(s), name_);
            return Thread((s - m) / sigma, name_);
        }
        
        if (func == "minmax") {
            const double min_val = s.min().item<double>();
            const double max_val = s.max().item<double>();
            if (max_val == min_val) return Thread(pluss::zeros_like(s), name_);
            return Thread((s - min_val) / (max_val - min_val), name_);
        }
    }
    
    return *this;
}

Thread Thread::apply(std::function<double(double)> func) const {
    if (is_series()) {
        const auto s = as_series();
        auto t = s.data();
        
        std::vector<double> result_vec;
        result_vec.reserve(t.numel());
        auto accessor = t.accessor<double, 1>();
        
        for (int64_t i = 0; i < t.numel(); ++i) {
            result_vec.push_back(func(accessor[i]));
        }
        
        return Thread(pluss::value(result_vec), name_);
    }
    
    return *this;
}
} // namespace pluss::table
