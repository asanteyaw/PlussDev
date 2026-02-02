#include <pluss/datalog/table/atlas/atlas.h>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <random>
#include <unordered_set>

namespace pluss::table {

// =============== Constructors ===============

Atlas::Atlas(const std::vector<std::shared_ptr<DataFrame>>& frames,
             const std::vector<std::string>& depth_names)
    : frames_(frames), depth_names_(depth_names) {
    
    if (!depth_names.empty() && depth_names.size() != frames.size()) {
        throw std::invalid_argument("Atlas: depth_names size must match frames size");
    }
    
    // Generate default depth names if not provided
    if (depth_names_.empty() && !frames_.empty()) {
        depth_names_.reserve(frames_.size());
        for (size_t i = 0; i < frames_.size(); ++i) {
            depth_names_.push_back(generate_depth_name(static_cast<int64_t>(i)));
        }
    }
    
    rebuild_depth_index();
}

Atlas::Atlas(const std::unordered_map<std::string, std::shared_ptr<DataFrame>>& frames) {
    frames_.reserve(frames.size());
    depth_names_.reserve(frames.size());
    
    for (const auto& [name, df] : frames) {
        frames_.push_back(df);
        depth_names_.push_back(name);
    }
    
    rebuild_depth_index();
}

Atlas::Atlas(const Atlas& other)
    : frames_(other.frames_),
      depth_names_(other.depth_names_),
      depth_name_to_index_(other.depth_name_to_index_) {}

Atlas& Atlas::operator=(const Atlas& other) {
    if (this != &other) {
        frames_ = other.frames_;
        depth_names_ = other.depth_names_;
        depth_name_to_index_ = other.depth_name_to_index_;
    }
    return *this;
}

// =============== Access and Inspection ===============

std::shared_ptr<DataFrame> Atlas::at(int64_t depth) const {
    validate_depth_bounds(depth);
    return frames_[static_cast<size_t>(depth)];
}

std::shared_ptr<DataFrame> Atlas::at(const std::string& depth_name) const {
    auto it = depth_name_to_index_.find(depth_name);
    if (it == depth_name_to_index_.end()) {
        throw std::out_of_range("Atlas: depth name '" + depth_name + "' not found");
    }
    return frames_[static_cast<size_t>(it->second)];
}

std::shared_ptr<DataFrame> Atlas::operator[](int64_t depth) const {
    return at(depth);
}

std::shared_ptr<DataFrame> Atlas::operator[](const std::string& depth_name) const {
    return at(depth_name);
}

int64_t Atlas::ndepth() const noexcept {
    return static_cast<int64_t>(frames_.size());
}

int64_t Atlas::nrows() const noexcept {
    if (frames_.empty()) return 0;
    auto shape = frames_[0]->shape();
    return shape.first;
}

int64_t Atlas::ncols() const noexcept {
    if (frames_.empty()) return 0;
    auto shape = frames_[0]->shape();
    return shape.second;
}

std::tuple<int64_t, int64_t, int64_t> Atlas::shape() const noexcept {
    return {ndepth(), nrows(), ncols()};
}

int64_t Atlas::size() const noexcept {
    return ndepth() * nrows() * ncols();
}

bool Atlas::empty() const noexcept {
    return frames_.empty();
}

const std::vector<std::string>& Atlas::depth_names() const noexcept {
    return depth_names_;
}

std::vector<std::string> Atlas::column_names() const {
    if (frames_.empty()) return {};
    return frames_[0]->columns();
}

// =============== Indexing and Selection ===============

std::shared_ptr<Atlas> Atlas::select_depths(const std::vector<int64_t>& depths) const {
    std::vector<std::shared_ptr<DataFrame>> selected_frames;
    std::vector<std::string> selected_names;
    
    selected_frames.reserve(depths.size());
    selected_names.reserve(depths.size());
    
    for (int64_t d : depths) {
        validate_depth_bounds(d);
        selected_frames.push_back(frames_[static_cast<size_t>(d)]);
        selected_names.push_back(depth_names_[static_cast<size_t>(d)]);
    }
    
    return std::make_shared<Atlas>(selected_frames, selected_names);
}

std::shared_ptr<Atlas> Atlas::select_depths(const std::vector<std::string>& depth_names) const {
    std::vector<std::shared_ptr<DataFrame>> selected_frames;
    std::vector<std::string> selected_names;
    
    selected_frames.reserve(depth_names.size());
    selected_names.reserve(depth_names.size());
    
    for (const auto& name : depth_names) {
        auto it = depth_name_to_index_.find(name);
        if (it == depth_name_to_index_.end()) {
            throw std::out_of_range("Atlas: depth name '" + name + "' not found");
        }
        selected_frames.push_back(frames_[static_cast<size_t>(it->second)]);
        selected_names.push_back(name);
    }
    
    return std::make_shared<Atlas>(selected_frames, selected_names);
}

std::shared_ptr<Atlas> Atlas::iloc(int64_t start, int64_t end) const {
    const int64_t n = ndepth();
    
    // Handle negative indices
    if (start < 0) start += n;
    if (end < 0) end += n;
    
    // Clamp to valid range
    start = std::max(int64_t(0), std::min(start, n));
    end = std::max(int64_t(0), std::min(end, n));
    
    if (start >= end) {
        return std::make_shared<Atlas>();
    }
    
    std::vector<std::shared_ptr<DataFrame>> selected_frames(
        frames_.begin() + start, frames_.begin() + end);
    std::vector<std::string> selected_names(
        depth_names_.begin() + start, depth_names_.begin() + end);
    
    return std::make_shared<Atlas>(selected_frames, selected_names);
}

std::shared_ptr<Atlas> Atlas::loc(const std::vector<std::string>& depth_names) const {
    return select_depths(depth_names);
}

std::shared_ptr<Atlas> Atlas::select_columns(const std::vector<std::string>& columns) const {
    return apply_to_all_frames([&columns](const std::shared_ptr<DataFrame>& df) {
        return df->filter(columns);
    });
}

std::shared_ptr<Atlas> Atlas::head(int64_t n) const {
    return iloc(0, std::min(n, ndepth()));
}

std::shared_ptr<Atlas> Atlas::tail(int64_t n) const {
    const int64_t nd = ndepth();
    return iloc(std::max(int64_t(0), nd - n), nd);
}

std::shared_ptr<Atlas> Atlas::sample(int64_t n, bool replace, int64_t random_state) const {
    const int64_t nd = ndepth();
    if (n > nd && !replace) {
        throw std::invalid_argument("Atlas::sample: Cannot take a larger sample than population when replace=false");
    }
    
    // Create indices vector
    std::vector<int64_t> indices(static_cast<size_t>(nd));
    std::iota(indices.begin(), indices.end(), 0);
    
    // Shuffle
    if (random_state >= 0) {
        std::mt19937 gen(static_cast<unsigned>(random_state));
        std::shuffle(indices.begin(), indices.end(), gen);
    } else {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(indices.begin(), indices.end(), gen);
    }
    
    // Take first n
    if (!replace) {
        indices.resize(static_cast<size_t>(std::min(n, nd)));
    } else {
        // For replace=true, sample with replacement
        std::vector<int64_t> sampled_indices;
        sampled_indices.reserve(static_cast<size_t>(n));
        
        if (random_state >= 0) {
            std::mt19937 gen(static_cast<unsigned>(random_state));
            std::uniform_int_distribution<int64_t> dist(0, nd - 1);
            for (int64_t i = 0; i < n; ++i) {
                sampled_indices.push_back(dist(gen));
            }
        } else {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int64_t> dist(0, nd - 1);
            for (int64_t i = 0; i < n; ++i) {
                sampled_indices.push_back(dist(gen));
            }
        }
        indices = std::move(sampled_indices);
    }
    
    return take(indices);
}

std::shared_ptr<Atlas> Atlas::take(const std::vector<int64_t>& indices) const {
    std::vector<std::shared_ptr<DataFrame>> selected_frames;
    std::vector<std::string> selected_names;
    
    selected_frames.reserve(indices.size());
    selected_names.reserve(indices.size());
    
    for (int64_t idx : indices) {
        validate_depth_bounds(idx);
        selected_frames.push_back(frames_[static_cast<size_t>(idx)]);
        selected_names.push_back(depth_names_[static_cast<size_t>(idx)]);
    }
    
    return std::make_shared<Atlas>(selected_frames, selected_names);
}

std::shared_ptr<Atlas> Atlas::truncate(int64_t before, int64_t after) const {
    int64_t start = (before == -1) ? 0 : before;
    int64_t end = (after == -1) ? ndepth() : after + 1;
    return iloc(start, end);
}

// =============== Sorting and Ordering ===============

std::shared_ptr<Atlas> Atlas::sort_values(const std::string& by, bool ascending, const std::string& agg) const {
    // Aggregate each DataFrame to get single value for sorting
    std::vector<std::pair<double, size_t>> values_indices;
    values_indices.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        double val = 0.0;
        if (agg == "mean") {
            val = frames_[i]->mean(by);
        } else if (agg == "sum") {
            val = frames_[i]->sum(by);
        } else if (agg == "median") {
            val = frames_[i]->median(by);
        } else if (agg == "min") {
            val = frames_[i]->min(by);
        } else if (agg == "max") {
            val = frames_[i]->max(by);
        } else {
            throw std::invalid_argument("Atlas::sort_values: unknown aggregation '" + agg + "'");
        }
        values_indices.emplace_back(val, i);
    }
    
    // Sort
    if (ascending) {
        std::sort(values_indices.begin(), values_indices.end());
    } else {
        std::sort(values_indices.begin(), values_indices.end(), std::greater<>());
    }
    
    // Build result
    std::vector<std::shared_ptr<DataFrame>> sorted_frames;
    std::vector<std::string> sorted_names;
    sorted_frames.reserve(frames_.size());
    sorted_names.reserve(frames_.size());
    
    for (const auto& [val, idx] : values_indices) {
        sorted_frames.push_back(frames_[idx]);
        sorted_names.push_back(depth_names_[idx]);
    }
    
    return std::make_shared<Atlas>(sorted_frames, sorted_names);
}

std::shared_ptr<Atlas> Atlas::sort_values(const std::vector<std::string>& by, bool ascending, const std::string& agg) const {
    if (by.empty()) return std::make_shared<Atlas>(*this);
    // For simplicity, sort by first column
    return sort_values(by[0], ascending, agg);
}

std::shared_ptr<Atlas> Atlas::sort_index(bool ascending) const {
    std::vector<std::pair<std::string, size_t>> names_indices;
    names_indices.reserve(depth_names_.size());
    
    for (size_t i = 0; i < depth_names_.size(); ++i) {
        names_indices.emplace_back(depth_names_[i], i);
    }
    
    if (ascending) {
        std::sort(names_indices.begin(), names_indices.end());
    } else {
        std::sort(names_indices.begin(), names_indices.end(), std::greater<>());
    }
    
    std::vector<std::shared_ptr<DataFrame>> sorted_frames;
    std::vector<std::string> sorted_names;
    sorted_frames.reserve(frames_.size());
    sorted_names.reserve(frames_.size());
    
    for (const auto& [name, idx] : names_indices) {
        sorted_frames.push_back(frames_[idx]);
        sorted_names.push_back(name);
    }
    
    return std::make_shared<Atlas>(sorted_frames, sorted_names);
}

std::shared_ptr<Atlas> Atlas::reindex(const std::vector<std::string>& new_depth_names) const {
    return std::make_shared<Atlas>(frames_, new_depth_names);
}

std::shared_ptr<Atlas> Atlas::reindex_like(const std::shared_ptr<Atlas>& other) const {
    return reindex(other->depth_names());
}

std::shared_ptr<Atlas> Atlas::set_depth_names(const std::vector<std::string>& names) const {
    return std::make_shared<Atlas>(frames_, names);
}

std::shared_ptr<Atlas> Atlas::reset_depth_names() const {
    std::vector<std::string> empty_names;
    return std::make_shared<Atlas>(frames_, empty_names);
}

// =============== Filtering and Selection ===============

std::shared_ptr<Atlas> Atlas::drop(const std::vector<std::string>& columns) const {
    return apply_to_all_frames([&columns](const std::shared_ptr<DataFrame>& df) {
        return df->drop(columns);
    });
}

std::shared_ptr<Atlas> Atlas::drop_depths(const std::vector<int64_t>& depths) const {
    std::unordered_set<int64_t> to_drop(depths.begin(), depths.end());
    std::vector<std::shared_ptr<DataFrame>> kept_frames;
    std::vector<std::string> kept_names;
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        if (to_drop.find(static_cast<int64_t>(i)) == to_drop.end()) {
            kept_frames.push_back(frames_[i]);
            kept_names.push_back(depth_names_[i]);
        }
    }
    
    return std::make_shared<Atlas>(kept_frames, kept_names);
}

std::shared_ptr<Atlas> Atlas::drop_depths(const std::vector<std::string>& depth_names) const {
    std::unordered_set<std::string> to_drop(depth_names.begin(), depth_names.end());
    std::vector<std::shared_ptr<DataFrame>> kept_frames;
    std::vector<std::string> kept_names;
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        if (to_drop.find(depth_names_[i]) == to_drop.end()) {
            kept_frames.push_back(frames_[i]);
            kept_names.push_back(depth_names_[i]);
        }
    }
    
    return std::make_shared<Atlas>(kept_frames, kept_names);
}

std::shared_ptr<Atlas> Atlas::drop_duplicates(const std::vector<std::string>& subset) const {
    return apply_to_all_frames([&subset](const std::shared_ptr<DataFrame>& df) {
        return df->drop_duplicates(subset);
    });
}

std::shared_ptr<Atlas> Atlas::filter(const std::vector<std::string>& items) const {
    return apply_to_all_frames([&items](const std::shared_ptr<DataFrame>& df) {
        return df->filter(items);
    });
}

// =============== Missing Data Handling ===============

std::shared_ptr<Atlas> Atlas::dropna(const std::string& how, int64_t thresh) const {
    return apply_to_all_frames([&how, thresh](const std::shared_ptr<DataFrame>& df) {
        return df->dropna(how, thresh);
    });
}

std::shared_ptr<Atlas> Atlas::fillna(double value) const {
    return apply_to_all_frames([value](const std::shared_ptr<DataFrame>& df) {
        return df->fillna(value);
    });
}

std::shared_ptr<Atlas> Atlas::fillna(const std::unordered_map<std::string, double>& values) const {
    return apply_to_all_frames([&values](const std::shared_ptr<DataFrame>& df) {
        return df->fillna(values);
    });
}

std::shared_ptr<Atlas> Atlas::ffill() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->ffill();
    });
}

std::shared_ptr<Atlas> Atlas::bfill() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->bfill();
    });
}

std::shared_ptr<Atlas> Atlas::interpolate(const std::string& method) const {
    return apply_to_all_frames([&method](const std::shared_ptr<DataFrame>& df) {
        return df->interpolate(method);
    });
}

std::shared_ptr<Atlas> Atlas::isna() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->isna();
    });
}

std::shared_ptr<Atlas> Atlas::notna() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->notna();
    });
}

// =============== Transformations ===============

std::shared_ptr<Atlas> Atlas::apply(
    std::function<std::shared_ptr<DataFrame>(const std::shared_ptr<DataFrame>&)> func) const {
    return apply_to_all_frames(func);
}

std::shared_ptr<Atlas> Atlas::transform(const std::string& func) const {
    return apply_to_all_frames([&func](const std::shared_ptr<DataFrame>& df) {
        return df->transform(func);
    });
}

std::shared_ptr<Atlas> Atlas::abs() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->abs();
    });
}

std::shared_ptr<Atlas> Atlas::round(int decimals) const {
    return apply_to_all_frames([decimals](const std::shared_ptr<DataFrame>& df) {
        return df->round(decimals);
    });
}

std::shared_ptr<Atlas> Atlas::clip(double lower, double upper) const {
    return apply_to_all_frames([lower, upper](const std::shared_ptr<DataFrame>& df) {
        return df->clip(lower, upper);
    });
}

std::shared_ptr<Atlas> Atlas::diff(int64_t periods) const {
    return apply_to_all_frames([periods](const std::shared_ptr<DataFrame>& df) {
        return df->diff(periods);
    });
}

std::shared_ptr<Atlas> Atlas::shift(int64_t periods) const {
    return apply_to_all_frames([periods](const std::shared_ptr<DataFrame>& df) {
        return df->shift(periods);
    });
}

std::shared_ptr<Atlas> Atlas::pct_change(int64_t periods) const {
    return apply_to_all_frames([periods](const std::shared_ptr<DataFrame>& df) {
        return df->pct_change(periods);
    });
}

std::shared_ptr<Atlas> Atlas::sqrt() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->sqrt();
    });
}

std::shared_ptr<Atlas> Atlas::log() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->log();
    });
}

std::shared_ptr<Atlas> Atlas::log10() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->log10();
    });
}

std::shared_ptr<Atlas> Atlas::exp() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->exp();
    });
}

std::shared_ptr<Atlas> Atlas::sin() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->sin();
    });
}

std::shared_ptr<Atlas> Atlas::cos() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->cos();
    });
}

std::shared_ptr<Atlas> Atlas::tan() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->tan();
    });
}

// =============== Cumulative Operations ===============

std::shared_ptr<Atlas> Atlas::cumsum_depth() const {
    if (frames_.empty()) return std::make_shared<Atlas>();
    
    std::vector<std::shared_ptr<DataFrame>> cumulative_frames;
    cumulative_frames.reserve(frames_.size());
    
    auto running_sum = frames_[0];
    cumulative_frames.push_back(running_sum);
    
    for (size_t i = 1; i < frames_.size(); ++i) {
        running_sum = running_sum->add(frames_[i]);
        cumulative_frames.push_back(running_sum);
    }
    
    return std::make_shared<Atlas>(cumulative_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::cumprod_depth() const {
    if (frames_.empty()) return std::make_shared<Atlas>();
    
    std::vector<std::shared_ptr<DataFrame>> cumulative_frames;
    cumulative_frames.reserve(frames_.size());
    
    auto running_prod = frames_[0];
    cumulative_frames.push_back(running_prod);
    
    for (size_t i = 1; i < frames_.size(); ++i) {
        running_prod = running_prod->mul(frames_[i]);
        cumulative_frames.push_back(running_prod);
    }
    
    return std::make_shared<Atlas>(cumulative_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::cummax_depth() const {
    if (frames_.empty()) return std::make_shared<Atlas>();
    
    std::vector<std::shared_ptr<DataFrame>> cumulative_frames;
    cumulative_frames.reserve(frames_.size());
    
    // For cummax, we need element-wise max
    auto running_max = frames_[0];
    cumulative_frames.push_back(running_max);
    
    for (size_t i = 1; i < frames_.size(); ++i) {
        // Element-wise maximum (needs implementation in DataFrame)
        auto comparison = running_max->gt(frames_[i]);
        running_max = running_max->where(comparison, NAN);  // This would need proper implementation
        cumulative_frames.push_back(running_max);
    }
    
    return std::make_shared<Atlas>(cumulative_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::cummin_depth() const {
    if (frames_.empty()) return std::make_shared<Atlas>();
    
    std::vector<std::shared_ptr<DataFrame>> cumulative_frames;
    cumulative_frames.reserve(frames_.size());
    
    auto running_min = frames_[0];
    cumulative_frames.push_back(running_min);
    
    for (size_t i = 1; i < frames_.size(); ++i) {
        // Element-wise minimum
        auto comparison = running_min->lt(frames_[i]);
        running_min = running_min->where(comparison, NAN);
        cumulative_frames.push_back(running_min);
    }
    
    return std::make_shared<Atlas>(cumulative_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::cumsum() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->cumsum();
    });
}

std::shared_ptr<Atlas> Atlas::cumprod() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->cumprod();
    });
}

std::shared_ptr<Atlas> Atlas::cummax() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->cummax();
    });
}

std::shared_ptr<Atlas> Atlas::cummin() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->cummin();
    });
}

// =============== Aggregations ===============

std::shared_ptr<DataFrame> Atlas::mean_depth() const {
    return reduce_across_depths([](const std::vector<std::shared_ptr<DataFrame>>& dfs) {
        if (dfs.empty()) return std::shared_ptr<DataFrame>();
        
        // Sum all DataFrames
        auto result = dfs[0];
        for (size_t i = 1; i < dfs.size(); ++i) {
            result = result->add(dfs[i]);
        }
        
        // Divide by count
        return result->div(std::make_shared<DataFrame>());  // Needs scalar division
    });
}

std::shared_ptr<DataFrame> Atlas::sum_depth() const {
    return reduce_across_depths([](const std::vector<std::shared_ptr<DataFrame>>& dfs) {
        if (dfs.empty()) return std::shared_ptr<DataFrame>();
        
        auto result = dfs[0];
        for (size_t i = 1; i < dfs.size(); ++i) {
            result = result->add(dfs[i]);
        }
        return result;
    });
}

std::shared_ptr<DataFrame> Atlas::median_depth() const {
    // Median across depths requires stacking and computing median
    // For now, return approximate (mean of min and max)
    auto min_df = min_depth();
    auto max_df = max_depth();
    return min_df->add(max_df)->div(std::make_shared<DataFrame>());  // Needs scalar div
}

std::shared_ptr<DataFrame> Atlas::std_depth(int ddof) const {
    (void)ddof;  // Placeholder implementation
    // Standard deviation across depth dimension
    auto mean_df = mean_depth();
    
    if (frames_.empty()) return mean_df;
    
    // Compute variance
    auto var_sum = frames_[0];  // Placeholder
    // Full implementation would compute (x - mean)^2 for each frame
    
    return var_sum;  // Would need sqrt
}

std::shared_ptr<DataFrame> Atlas::var_depth(int ddof) const {
    (void)ddof;  // Placeholder implementation
    auto mean_df = mean_depth();
    // Compute variance across depths
    return mean_df;  // Placeholder
}

std::shared_ptr<DataFrame> Atlas::min_depth() const {
    return reduce_across_depths([](const std::vector<std::shared_ptr<DataFrame>>& dfs) {
        if (dfs.empty()) return std::shared_ptr<DataFrame>();
        
        auto result = dfs[0];
        for (size_t i = 1; i < dfs.size(); ++i) {
            // Element-wise min (would need proper implementation)
            // TODO: implement element-wise min
        }
        return result;
    });
}

std::shared_ptr<DataFrame> Atlas::max_depth() const {
    return reduce_across_depths([](const std::vector<std::shared_ptr<DataFrame>>& dfs) {
        if (dfs.empty()) return std::shared_ptr<DataFrame>();
        
        auto result = dfs[0];
        for (size_t i = 1; i < dfs.size(); ++i) {
            // Element-wise max (would need proper implementation)
            // TODO: implement element-wise max
        }
        return result;
    });
}

std::shared_ptr<DataFrame> Atlas::prod_depth() const {
    return reduce_across_depths([](const std::vector<std::shared_ptr<DataFrame>>& dfs) {
        if (dfs.empty()) return std::shared_ptr<DataFrame>();
        
        auto result = dfs[0];
        for (size_t i = 1; i < dfs.size(); ++i) {
            result = result->mul(dfs[i]);
        }
        return result;
    });
}

std::shared_ptr<DataFrame> Atlas::quantile_depth(double q) const {
    (void)q;  // Placeholder implementation
    // Would need to implement quantile across depth dimension
    return mean_depth();  // Placeholder
}

// Within-DataFrame aggregations
std::shared_ptr<Atlas> Atlas::mean() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->mean();
    });
}

std::shared_ptr<Atlas> Atlas::sum() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->sum();
    });
}

std::shared_ptr<Atlas> Atlas::median() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->median();
    });
}

std::shared_ptr<Atlas> Atlas::std(int ddof) const {
    return apply_to_all_frames([ddof](const std::shared_ptr<DataFrame>& df) {
        return df->std(ddof);
    });
}

std::shared_ptr<Atlas> Atlas::var(int ddof) const {
    return apply_to_all_frames([ddof](const std::shared_ptr<DataFrame>& df) {
        return df->var(ddof);
    });
}

std::shared_ptr<Atlas> Atlas::min() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->min();
    });
}

std::shared_ptr<Atlas> Atlas::max() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->max();
    });
}

std::shared_ptr<Atlas> Atlas::prod() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->prod();
    });
}

std::shared_ptr<Atlas> Atlas::count() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->count();
    });
}

std::shared_ptr<Atlas> Atlas::nunique() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->nunique();
    });
}

std::shared_ptr<Atlas> Atlas::quantile(double q) const {
    return apply_to_all_frames([q](const std::shared_ptr<DataFrame>& df) {
        return df->quantile(q);
    });
}

// =============== Statistical Operations ===============

std::shared_ptr<DataFrame> Atlas::describe_depth(const std::vector<double>& percentiles) const {
    (void)percentiles;  // Placeholder implementation
    // Statistical summary across depth dimension
    return mean_depth();  // Placeholder
}

std::shared_ptr<Atlas> Atlas::describe(const std::vector<double>& percentiles) const {
    return apply_to_all_frames([&percentiles](const std::shared_ptr<DataFrame>& df) {
        return df->describe(percentiles);
    });
}

std::shared_ptr<Atlas> Atlas::corr(const std::string& method) const {
    return apply_to_all_frames([&method](const std::shared_ptr<DataFrame>& df) {
        return df->corr(method);
    });
}

std::shared_ptr<Atlas> Atlas::cov(int ddof) const {
    return apply_to_all_frames([ddof](const std::shared_ptr<DataFrame>& df) {
        return df->cov(ddof);
    });
}

std::shared_ptr<Atlas> Atlas::rank(const std::string& method) const {
    return apply_to_all_frames([&method](const std::shared_ptr<DataFrame>& df) {
        return df->rank(method);
    });
}

// =============== Comparison Operations ===============

std::shared_ptr<Atlas> Atlas::eq(const std::shared_ptr<Atlas>& other) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::eq: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->eq(other->frames_[i]));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::ne(const std::shared_ptr<Atlas>& other) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::ne: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->ne(other->frames_[i]));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::lt(const std::shared_ptr<Atlas>& other) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::lt: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->lt(other->frames_[i]));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::le(const std::shared_ptr<Atlas>& other) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::le: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->le(other->frames_[i]));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::gt(const std::shared_ptr<Atlas>& other) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::gt: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->gt(other->frames_[i]));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::ge(const std::shared_ptr<Atlas>& other) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::ge: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->ge(other->frames_[i]));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

// Scalar comparisons
std::shared_ptr<Atlas> Atlas::eq(double value) const {
    (void)value;  // Placeholder - needs DataFrame::eq(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        // Would need DataFrame::eq(double)
        return df;  // Placeholder
    });
}

std::shared_ptr<Atlas> Atlas::ne(double value) const {
    (void)value;  // Placeholder - needs DataFrame::ne(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df;  // Placeholder
    });
}

std::shared_ptr<Atlas> Atlas::lt(double value) const {
    (void)value;  // Placeholder - needs DataFrame::lt(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df;  // Placeholder
    });
}

std::shared_ptr<Atlas> Atlas::le(double value) const {
    (void)value;  // Placeholder - needs DataFrame::le(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df;  // Placeholder
    });
}

std::shared_ptr<Atlas> Atlas::gt(double value) const {
    (void)value;  // Placeholder - needs DataFrame::gt(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df;  // Placeholder
    });
}

std::shared_ptr<Atlas> Atlas::ge(double value) const {
    (void)value;  // Placeholder - needs DataFrame::ge(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df;  // Placeholder
    });
}

// =============== Arithmetic Operations ===============

std::shared_ptr<Atlas> Atlas::add(const std::shared_ptr<Atlas>& other, double fill_value) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::add: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->add(other->frames_[i], fill_value));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::sub(const std::shared_ptr<Atlas>& other, double fill_value) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::sub: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->sub(other->frames_[i], fill_value));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::mul(const std::shared_ptr<Atlas>& other, double fill_value) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::mul: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->mul(other->frames_[i], fill_value));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::div(const std::shared_ptr<Atlas>& other, double fill_value) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::div: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->div(other->frames_[i], fill_value));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::mod(const std::shared_ptr<Atlas>& other, double fill_value) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::mod: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->mod(other->frames_[i], fill_value));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::pow(const std::shared_ptr<Atlas>& other, double fill_value) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::pow: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        result_frames.push_back(frames_[i]->pow(other->frames_[i], fill_value));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

// Scalar arithmetic (placeholder - would need DataFrame scalar operations)
std::shared_ptr<Atlas> Atlas::add(double value) const {
    (void)value;  // Placeholder - needs DataFrame::add(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df;  // Would call df->add(value)
    });
}

std::shared_ptr<Atlas> Atlas::sub(double value) const {
    (void)value;  // Placeholder - needs DataFrame::sub(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df;  // Would call df->sub(value)
    });
}

std::shared_ptr<Atlas> Atlas::mul(double value) const {
    (void)value;  // Placeholder - needs DataFrame::mul(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df;  // Would call df->mul(value)
    });
}

std::shared_ptr<Atlas> Atlas::div(double value) const {
    (void)value;  // Placeholder - needs DataFrame::div(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df;  // Would call df->div(value)
    });
}

std::shared_ptr<Atlas> Atlas::mod(double value) const {
    (void)value;  // Placeholder - needs DataFrame::mod(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df;  // Would call df->mod(value)
    });
}

std::shared_ptr<Atlas> Atlas::pow(double value) const {
    (void)value;  // Placeholder - needs DataFrame::pow(double) implementation
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df;  // Would call df->pow(value)
    });
}

// Operator overloads
std::shared_ptr<Atlas> Atlas::operator+(const std::shared_ptr<Atlas>& other) const {
    return add(other);
}

std::shared_ptr<Atlas> Atlas::operator-(const std::shared_ptr<Atlas>& other) const {
    return sub(other);
}

std::shared_ptr<Atlas> Atlas::operator*(const std::shared_ptr<Atlas>& other) const {
    return mul(other);
}

std::shared_ptr<Atlas> Atlas::operator/(const std::shared_ptr<Atlas>& other) const {
    return div(other);
}

std::shared_ptr<Atlas> Atlas::operator+(double value) const {
    return add(value);
}

std::shared_ptr<Atlas> Atlas::operator-(double value) const {
    return sub(value);
}

std::shared_ptr<Atlas> Atlas::operator*(double value) const {
    return mul(value);
}

std::shared_ptr<Atlas> Atlas::operator/(double value) const {
    return div(value);
}

// =============== Renaming ===============

std::shared_ptr<Atlas> Atlas::rename_columns(const std::unordered_map<std::string, std::string>& columns) const {
    return apply_to_all_frames([&columns](const std::shared_ptr<DataFrame>& df) {
        return df->rename(columns);
    });
}

std::shared_ptr<Atlas> Atlas::rename_depths(const std::unordered_map<std::string, std::string>& depths) const {
    std::vector<std::string> new_names = depth_names_;
    
    for (size_t i = 0; i < new_names.size(); ++i) {
        auto it = depths.find(new_names[i]);
        if (it != depths.end()) {
            new_names[i] = it->second;
        }
    }
    
    return std::make_shared<Atlas>(frames_, new_names);
}

std::shared_ptr<Atlas> Atlas::add_prefix(const std::string& prefix) const {
    return apply_to_all_frames([&prefix](const std::shared_ptr<DataFrame>& df) {
        return df->add_prefix(prefix);
    });
}

std::shared_ptr<Atlas> Atlas::add_suffix(const std::string& suffix) const {
    return apply_to_all_frames([&suffix](const std::shared_ptr<DataFrame>& df) {
        return df->add_suffix(suffix);
    });
}

// =============== Combining and Merging ===============

std::shared_ptr<Atlas> Atlas::concat(const std::vector<std::shared_ptr<Atlas>>& atlases, bool ignore_depth_names) {
    std::vector<std::shared_ptr<DataFrame>> all_frames;
    std::vector<std::string> all_names;
    
    for (const auto& atlas : atlases) {
        all_frames.insert(all_frames.end(), atlas->frames_.begin(), atlas->frames_.end());
        if (!ignore_depth_names) {
            all_names.insert(all_names.end(), atlas->depth_names_.begin(), atlas->depth_names_.end());
        }
    }
    
    if (ignore_depth_names) {
        all_names.clear();
    }
    
    return std::make_shared<Atlas>(all_frames, all_names);
}

std::shared_ptr<Atlas> Atlas::append(const std::shared_ptr<Atlas>& other, bool ignore_depth_names) const {
    return concat({std::make_shared<Atlas>(*this), other}, ignore_depth_names);
}

std::shared_ptr<Atlas> Atlas::append_depth(const std::shared_ptr<DataFrame>& frame, const std::string& depth_name) const {
    auto new_frames = frames_;
    auto new_names = depth_names_;
    
    new_frames.push_back(frame);
    new_names.push_back(depth_name.empty() ? generate_depth_name(ndepth()) : depth_name);
    
    return std::make_shared<Atlas>(new_frames, new_names);
}

std::shared_ptr<Atlas> Atlas::merge(const std::shared_ptr<Atlas>& other, const std::string& on, const std::string& how) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::merge: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> merged_frames;
    merged_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        merged_frames.push_back(frames_[i]->merge(other->frames_[i], on, how));
    }
    
    return std::make_shared<Atlas>(merged_frames, depth_names_);
}

std::shared_ptr<Atlas> Atlas::join(const std::shared_ptr<Atlas>& other, const std::string& how) const {
    if (ndepth() != other->ndepth()) {
        throw std::invalid_argument("Atlas::join: depth dimensions must match");
    }
    
    std::vector<std::shared_ptr<DataFrame>> joined_frames;
    joined_frames.reserve(frames_.size());
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        joined_frames.push_back(frames_[i]->join(other->frames_[i], how));
    }
    
    return std::make_shared<Atlas>(joined_frames, depth_names_);
}

// =============== Reshaping ===============

std::shared_ptr<DataFrame> Atlas::stack_depths() const {
    // Would stack all DataFrames vertically with depth indicator column
    if (frames_.empty()) return std::make_shared<DataFrame>();
    
    // Placeholder - would concatenate all frames
    return frames_[0];
}

std::shared_ptr<Atlas> Atlas::unstack_column(const std::shared_ptr<DataFrame>& df, const std::string& depth_column) {
    (void)df;  // Placeholder implementation
    (void)depth_column;  // Placeholder implementation
    // Would split DataFrame into multiple DataFrames based on depth_column values
    // Placeholder
    return std::make_shared<Atlas>();
}

std::shared_ptr<DataFrame> Atlas::squeeze() const {
    if (ndepth() == 1) return frames_[0];
    throw std::runtime_error("Atlas::squeeze: cannot squeeze Atlas with multiple depths");
}

// =============== Boolean Reductions ===============

bool Atlas::all() const {
    for (const auto& frame : frames_) {
        // Would need DataFrame::all() -> bool
        // if (!frame->all()) return false;
    }
    return true;
}

bool Atlas::any() const {
    for (const auto& frame : frames_) {
        // Would need DataFrame::any() -> bool
        // if (frame->any()) return true;
    }
    return false;
}

std::shared_ptr<Atlas> Atlas::all_within() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->all();
    });
}

std::shared_ptr<Atlas> Atlas::any_within() const {
    return apply_to_all_frames([](const std::shared_ptr<DataFrame>& df) {
        return df->any();
    });
}

std::shared_ptr<DataFrame> Atlas::all_depth() const {
    // Logical AND across all depths
    if (frames_.empty()) return std::make_shared<DataFrame>();
    
    auto result = frames_[0];
    // Would need element-wise AND operations
    return result;
}

std::shared_ptr<DataFrame> Atlas::any_depth() const {
    // Logical OR across all depths
    if (frames_.empty()) return std::make_shared<DataFrame>();
    
    auto result = frames_[0];
    // Would need element-wise OR operations
    return result;
}

// =============== Utility Methods ===============

bool Atlas::equals(const std::shared_ptr<Atlas>& other) const {
    if (ndepth() != other->ndepth()) return false;
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        if (!frames_[i]->equals(other->frames_[i])) return false;
    }
    
    return depth_names_ == other->depth_names_;
}

std::shared_ptr<Atlas> Atlas::copy() const {
    return std::make_shared<Atlas>(*this);
}

std::shared_ptr<Atlas> Atlas::assign(const std::unordered_map<std::string, std::shared_ptr<DataFrame>>& kwargs) const {
    auto result = std::make_shared<Atlas>(*this);
    
    for (const auto& [name, df] : kwargs) {
        auto it = result->depth_name_to_index_.find(name);
        if (it != result->depth_name_to_index_.end()) {
            result->frames_[static_cast<size_t>(it->second)] = df;
        } else {
            result->frames_.push_back(df);
            result->depth_names_.push_back(name);
            result->rebuild_depth_index();
        }
    }
    
    return result;
}

bool Atlas::has_consistent_structure() const {
    if (frames_.empty()) return true;
    
    auto [ref_rows, ref_cols] = frames_[0]->shape();
    auto ref_columns = frames_[0]->columns();
    
    for (size_t i = 1; i < frames_.size(); ++i) {
        auto [rows, cols] = frames_[i]->shape();
        if (rows != ref_rows || cols != ref_cols) return false;
        
        auto columns = frames_[i]->columns();
        if (columns != ref_columns) return false;
    }
    
    return true;
}

void Atlas::validate_structure() const {
    if (!has_consistent_structure()) {
        throw std::runtime_error("Atlas: DataFrames have inconsistent structure");
    }
}

// =============== I/O Operations ===============

std::string Atlas::to_json(const std::string& orient) const {
    std::ostringstream oss;
    oss << "{\"depths\": [";
    
    for (size_t i = 0; i < frames_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "{"
            << "\"name\": \"" << depth_names_[i] << "\", "
            << "\"data\": " << frames_[i]->to_json(orient)
            << "}";
    }
    
    oss << "]}";
    return oss.str();
}

std::string Atlas::to_string() const {
    std::ostringstream oss;
    oss << "Atlas (depth=" << ndepth() << ", rows=" << nrows() << ", cols=" << ncols() << ")\n";
    oss << "Depths: [";
    for (size_t i = 0; i < depth_names_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << depth_names_[i];
    }
    oss << "]\n";
    return oss.str();
}

void Atlas::to_directory(const std::string& dir_path) const {
    (void)dir_path;  // Placeholder implementation
    // Would save each DataFrame to separate file in directory
    // Implementation depends on file I/O utilities
}

std::shared_ptr<Atlas> Atlas::from_directory(const std::string& dir_path, const std::string& pattern) {
    (void)dir_path;  // Placeholder implementation
    (void)pattern;  // Placeholder implementation
    // Would load DataFrames from directory
    // Implementation depends on file I/O utilities
    return std::make_shared<Atlas>();
}

std::shared_ptr<Atlas> Atlas::from_frames(
    const std::vector<std::shared_ptr<DataFrame>>& frames,
    const std::vector<std::string>& depth_names) {
    return std::make_shared<Atlas>(frames, depth_names);
}

std::shared_ptr<Atlas> Atlas::from_frames(
    const std::unordered_map<std::string, std::shared_ptr<DataFrame>>& frames) {
    return std::make_shared<Atlas>(frames);
}

// =============== Private Helper Methods ===============

void Atlas::rebuild_depth_index() {
    depth_name_to_index_.clear();
    for (size_t i = 0; i < depth_names_.size(); ++i) {
        depth_name_to_index_[depth_names_[i]] = static_cast<int64_t>(i);
    }
}

void Atlas::validate_depth_bounds(int64_t depth) const {
    if (depth < 0 || depth >= ndepth()) {
        throw std::out_of_range("Atlas: depth " + std::to_string(depth) + " out of range [0, " + std::to_string(ndepth()) + ")");
    }
}

std::string Atlas::generate_depth_name(int64_t index) const {
    return "depth_" + std::to_string(index);
}

template<typename Func>
std::shared_ptr<Atlas> Atlas::apply_to_all_frames(Func&& func) const {
    std::vector<std::shared_ptr<DataFrame>> result_frames;
    result_frames.reserve(frames_.size());
    
    for (const auto& frame : frames_) {
        result_frames.push_back(func(frame));
    }
    
    return std::make_shared<Atlas>(result_frames, depth_names_);
}

template<typename Func>
std::shared_ptr<DataFrame> Atlas::reduce_across_depths(Func&& func) const {
    return func(frames_);
}

} // namespace pluss::table
