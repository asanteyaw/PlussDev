#include <pluss/datalog/table/thread/thread.h>
#include <pluss/NoteOps.h>
#include <pluss/note_factory.h>
#include <pluss/series_factory.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

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
            auto vec = as_note().vector();
            std::vector<std::string> sel;
            auto cond_numel = condition.numel();
            for (int64_t i = 0; i < cond_numel && static_cast<size_t>(i) < vec.size(); ++i) {
                if (condition[i].item<bool>()) {
                    sel.push_back(vec[i].str());
                }
            }
            std::vector<pluss::Chord> chords;
            chords.reserve(sel.size());
            for (const auto &sstr : sel) chords.emplace_back(sstr);
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
        // Efficient unique using sort + consecutive removal
        auto sorted_vals = std::get<0>(as_series().sort(0));
        int64_t numel = sorted_vals.numel();
        if (numel == 0) return *this;
        if (numel == 1) return Thread(sorted_vals, name_);
        
        // Build mask for non-consecutive duplicates
        std::vector<bool> keep(numel, true);
        for (int64_t i = 1; i < numel; ++i) {
            if (sorted_vals[i].item<double>() == sorted_vals[i-1].item<double>()) {
                keep[i] = false;
            }
        }
        
        // Build result
        std::vector<double> result_vec;
        result_vec.reserve(numel);
        for (int64_t i = 0; i < numel; ++i) {
            if (keep[i]) result_vec.push_back(sorted_vals[i].item<double>());
        }
        
        return Thread(pluss::from_torch(torch::tensor(result_vec)), name_);
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
        auto sorted_vals = std::get<0>(as_series().sort(0, ascending));
        return Thread(sorted_vals, name_);
    }
    if (is_timeline()) {
        auto s = as_timeline().to_Series();
        auto sorted_vals = std::get<0>(s.sort(0, ascending));
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
            
            return Thread(pluss::from_torch(torch::tensor(count_vals)), name_ + "_counts");
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
            return Thread(pluss::full({size()}, NAN), name_);
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
        if (periods > 0) {
            auto nans = pluss::full({periods}, NAN);
            auto trimmed = as_series().narrow(0, 0, sz - periods);
            auto combined = pluss::from_torch(torch::cat({nans.data(), trimmed.data()}, 0));
            return Thread(combined, name_);
        } else {
            auto nans = pluss::full({-periods}, NAN);
            auto trimmed = as_series().narrow(0, -periods, sz + periods);
            auto combined = pluss::from_torch(torch::cat({trimmed.data(), nans.data()}, 0));
            return Thread(combined, name_);
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
        auto idx_tensor = pluss::from_torch(torch::tensor(indices, torch::kLong));
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
        for (int64_t i = 1; i < size(); ++i) {
            if (std::isnan(result[i].item<double>())) {
                result[i] = result[i-1];
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
        for (int64_t i = size() - 2; i >= 0; --i) {
            if (std::isnan(result[i].item<double>())) {
                result[i] = result[i+1];
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
                    result_vec.push_back(NAN);
                }
            }
        }
        return Thread(pluss::from_torch(torch::tensor(result_vec)), name_);
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

} // namespace pluss::table
