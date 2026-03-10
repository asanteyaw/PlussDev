#include "pluss/ai/nodej/prep.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::ai::nodej::prep {
namespace {

struct DenseMatrix {
  std::vector<double> values{};
  std::int64_t sample_count = 0;
  std::int64_t feature_count = 0;
  ObservationLayout layout = ObservationLayout::Rows;
  Platform original_platform = pluss::pCPU;
  ScalarType original_dtype = ScalarType::Float64;
};

[[nodiscard]] Config prep_work_config() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

[[nodiscard]] std::size_t offset(const DenseMatrix& matrix, std::int64_t sample_idx, std::int64_t feature_idx) {
  if (matrix.layout == ObservationLayout::Rows) {
    return static_cast<std::size_t>(sample_idx * matrix.feature_count + feature_idx);
  }
  return static_cast<std::size_t>(feature_idx * matrix.sample_count + sample_idx);
}

[[nodiscard]] StatusOr<DenseMatrix> to_dense(const FeatureBatch& batch, const char* where) {
  Status check = validate_features(batch);
  if (!check.ok()) return StatusOr<DenseMatrix>(std::move(check));
  if (batch.x.dim() != 2) {
    return StatusOr<DenseMatrix>(Status::shape_mismatch(std::string(where) + " requires rank-2 feature matrices."));
  }

  DenseMatrix matrix{};
  matrix.layout = batch.layout;
  matrix.sample_count = batch.sample_count();
  matrix.feature_count = batch.feature_count();
  matrix.original_platform = batch.x.platform();
  matrix.original_dtype = batch.x.scalar_type();

  const Series x64 = batch.x.to(pluss::pCPU, ScalarType::Float64);
  matrix.values = x64.to_vec<double>();
  if (static_cast<std::int64_t>(matrix.values.size()) != matrix.sample_count * matrix.feature_count) {
    return StatusOr<DenseMatrix>(
        Status::shape_mismatch(std::string(where) + " expects contiguous 2D feature storage with matching element count."));
  }
  return StatusOr<DenseMatrix>(std::move(matrix));
}

[[nodiscard]] Series from_dense(const DenseMatrix& matrix, std::vector<double> values, std::int64_t out_feature_count) {
  std::vector<std::int64_t> shape =
      matrix.layout == ObservationLayout::Rows
          ? std::vector<std::int64_t>{matrix.sample_count, out_feature_count}
          : std::vector<std::int64_t>{out_feature_count, matrix.sample_count};
  Series out = pluss::make_series(std::move(values), shape, prep_work_config());
  return out.to(matrix.original_platform, matrix.original_dtype);
}

[[nodiscard]] Series from_dense_like(const DenseMatrix& matrix, std::vector<double> values) {
  return from_dense(matrix, std::move(values), matrix.feature_count);
}

[[nodiscard]] std::vector<double> feature_non_missing(const DenseMatrix& matrix, std::int64_t feature_idx) {
  std::vector<double> values{};
  values.reserve(static_cast<std::size_t>(matrix.sample_count));
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    const double v = matrix.values[offset(matrix, i, feature_idx)];
    if (!std::isnan(v)) values.push_back(v);
  }
  return values;
}

[[nodiscard]] double quantile(std::vector<double> values, double q) {
  std::sort(values.begin(), values.end());
  if (values.size() == 1) return values.front();

  const double pos = q * static_cast<double>(values.size() - 1);
  const auto lo = static_cast<std::size_t>(std::floor(pos));
  const auto hi = static_cast<std::size_t>(std::ceil(pos));
  if (lo == hi) return values[lo];
  const double t = pos - static_cast<double>(lo);
  return values[lo] * (1.0 - t) + values[hi] * t;
}

[[nodiscard]] double most_frequent(const std::vector<double>& values, double fallback) {
  if (values.empty()) return fallback;

  std::unordered_map<std::uint64_t, std::int64_t> counts{};
  counts.reserve(values.size());

  for (double value : values) {
    if (value == 0.0) value = 0.0; // normalize signed zero
    const std::uint64_t key = std::bit_cast<std::uint64_t>(value);
    ++counts[key];
  }

  std::int64_t best_count = -1;
  double best_value = fallback;
  for (const auto& [key, count] : counts) {
    const double value = std::bit_cast<double>(key);
    if (count > best_count || (count == best_count && value < best_value)) {
      best_count = count;
      best_value = value;
    }
  }
  return best_value;
}

[[nodiscard]] Status validate_range(double low, double high, const char* where) {
  if (!std::isfinite(low) || !std::isfinite(high) || !(high > low)) {
    return Status::invalid_argument(std::string(where) + " requires finite bounds with upper > lower.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_quantiles(double low, double high, const char* where) {
  if (!std::isfinite(low) || !std::isfinite(high) || !(low >= 0.0 && high <= 1.0 && low < high)) {
    return Status::invalid_argument(std::string(where) + " requires quantiles in [0,1] with lower < upper.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_floor(double value, const char* where) {
  if (!std::isfinite(value) || !(value > 0.0)) {
    return Status::invalid_argument(std::string(where) + " requires a positive finite floor.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_max_categories(std::int64_t value, const char* where) {
  if (value < 0) {
    return Status::invalid_argument(std::string(where) + " requires max_categories >= 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_unit_interval(double value, const char* where) {
  if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
    return Status::invalid_argument(std::string(where) + " requires a finite value in [0,1].");
  }
  return Status::success();
}

[[nodiscard]] std::uint64_t key_for_value(double value) {
  if (value == 0.0) value = 0.0; // normalize signed zero
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t out_offset(ObservationLayout layout,
                                     std::int64_t sample_count,
                                     std::int64_t encoded_features,
                                     std::int64_t sample_idx,
                                     std::int64_t encoded_feature_idx) {
  if (layout == ObservationLayout::Rows) {
    return static_cast<std::size_t>(sample_idx * encoded_features + encoded_feature_idx);
  }
  return static_cast<std::size_t>(encoded_feature_idx * sample_count + sample_idx);
}

[[nodiscard]] Status validate_trained_input(bool trained,
                                            ObservationLayout expected_layout,
                                            std::int64_t expected_features,
                                            const FeatureBatch& batch,
                                            const char* where) {
  if (!trained) return Status::precondition(std::string(where) + " requires learn() before map().");
  Status check = validate_features(batch);
  if (!check.ok()) return check;
  if (batch.layout != expected_layout) {
    return Status::shape_mismatch(std::string(where) + " layout mismatch with fitted transformer.");
  }
  if (batch.feature_count() != expected_features) {
    return Status::shape_mismatch(std::string(where) + " feature count mismatch with fitted transformer.");
  }
  return Status::success();
}

} // namespace

StandardScaler::StandardScaler(StandardScalerOptions options) : options_(options) {}

bool StandardScaler::trained() const noexcept { return trained_; }

Status StandardScaler::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status floor_status = validate_floor(options_.variance_floor, "StandardScaler");
  if (!floor_status.ok()) return floor_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "StandardScaler::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  center_.assign(static_cast<std::size_t>(feature_count_), 0.0);
  scale_.assign(static_cast<std::size_t>(feature_count_), 1.0);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    double sum = 0.0;
    std::int64_t count = 0;
    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const double v = matrix.values[offset(matrix, i, f)];
      if (std::isnan(v)) continue;
      sum += v;
      ++count;
    }

    double mean = 0.0;
    double stddev = 1.0;
    if (count > 0) {
      mean = sum / static_cast<double>(count);
      double var = 0.0;
      for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
        const double v = matrix.values[offset(matrix, i, f)];
        if (std::isnan(v)) continue;
        const double d = v - mean;
        var += d * d;
      }
      var /= static_cast<double>(count);
      stddev = std::sqrt(var);
      if (!(stddev >= options_.variance_floor)) stddev = 1.0;
    }

    center_[static_cast<std::size_t>(f)] = options_.with_centering ? mean : 0.0;
    scale_[static_cast<std::size_t>(f)] = options_.with_scaling ? stddev : 1.0;
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> StandardScaler::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "StandardScaler::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "StandardScaler::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out = matrix.values;
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      const double v = out[idx];
      if (std::isnan(v)) continue;
      out[idx] = (v - center_[static_cast<std::size_t>(f)]) / scale_[static_cast<std::size_t>(f)];
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> StandardScaler::learn_map(const FeatureBatch& batch,
                                           const TrainOptions& learn_options,
                                           const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

MinMaxScaler::MinMaxScaler(MinMaxScalerOptions options) : options_(options) {}

bool MinMaxScaler::trained() const noexcept { return trained_; }

Status MinMaxScaler::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status range_status = validate_range(options_.lower, options_.upper, "MinMaxScaler");
  if (!range_status.ok()) return range_status;
  Status floor_status = validate_floor(options_.range_floor, "MinMaxScaler");
  if (!floor_status.ok()) return floor_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "MinMaxScaler::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  data_min_.assign(static_cast<std::size_t>(feature_count_), 0.0);
  data_max_.assign(static_cast<std::size_t>(feature_count_), 1.0);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    double minv = std::numeric_limits<double>::infinity();
    double maxv = -std::numeric_limits<double>::infinity();
    bool seen = false;
    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const double v = matrix.values[offset(matrix, i, f)];
      if (std::isnan(v)) continue;
      minv = std::min(minv, v);
      maxv = std::max(maxv, v);
      seen = true;
    }
    if (!seen) {
      minv = 0.0;
      maxv = 1.0;
    }
    data_min_[static_cast<std::size_t>(f)] = minv;
    data_max_[static_cast<std::size_t>(f)] = maxv;
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> MinMaxScaler::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "MinMaxScaler::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "MinMaxScaler::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out = matrix.values;
  const double out_span = options_.upper - options_.lower;
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      const double v = out[idx];
      if (std::isnan(v)) continue;
      const double minv = data_min_[static_cast<std::size_t>(f)];
      const double maxv = data_max_[static_cast<std::size_t>(f)];
      const double den = maxv - minv;
      double mapped = options_.lower;
      if (std::abs(den) >= options_.range_floor) {
        mapped = ((v - minv) / den) * out_span + options_.lower;
      }
      if (options_.clip) {
        mapped = std::clamp(mapped, options_.lower, options_.upper);
      }
      out[idx] = mapped;
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> MinMaxScaler::learn_map(const FeatureBatch& batch,
                                         const TrainOptions& learn_options,
                                         const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

RobustScaler::RobustScaler(RobustScalerOptions options) : options_(options) {}

bool RobustScaler::trained() const noexcept { return trained_; }

Status RobustScaler::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status q_status = validate_quantiles(options_.lower_quantile, options_.upper_quantile, "RobustScaler");
  if (!q_status.ok()) return q_status;
  Status floor_status = validate_floor(options_.range_floor, "RobustScaler");
  if (!floor_status.ok()) return floor_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "RobustScaler::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  center_.assign(static_cast<std::size_t>(feature_count_), 0.0);
  scale_.assign(static_cast<std::size_t>(feature_count_), 1.0);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    std::vector<double> values = feature_non_missing(matrix, f);
    double med = 0.0;
    double iqr = 1.0;
    if (!values.empty()) {
      med = quantile(values, 0.5);
      const double q_low = quantile(values, options_.lower_quantile);
      const double q_high = quantile(values, options_.upper_quantile);
      iqr = q_high - q_low;
      if (!(std::abs(iqr) >= options_.range_floor)) iqr = 1.0;
    }

    center_[static_cast<std::size_t>(f)] = options_.with_centering ? med : 0.0;
    scale_[static_cast<std::size_t>(f)] = options_.with_scaling ? iqr : 1.0;
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> RobustScaler::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "RobustScaler::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "RobustScaler::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out = matrix.values;
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      const double v = out[idx];
      if (std::isnan(v)) continue;
      out[idx] = (v - center_[static_cast<std::size_t>(f)]) / scale_[static_cast<std::size_t>(f)];
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> RobustScaler::learn_map(const FeatureBatch& batch,
                                         const TrainOptions& learn_options,
                                         const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

SimpleImputer::SimpleImputer(SimpleImputerOptions options) : options_(options) {}

bool SimpleImputer::trained() const noexcept { return trained_; }

Status SimpleImputer::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "SimpleImputer::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  fill_values_.assign(static_cast<std::size_t>(feature_count_), options_.constant_value);

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    std::vector<double> values = feature_non_missing(matrix, f);
    double fill = options_.empty_feature_fallback;
    switch (options_.strategy) {
      case SimpleImputerStrategy::Mean: {
        if (!values.empty()) {
          const double sum = std::accumulate(values.begin(), values.end(), 0.0);
          fill = sum / static_cast<double>(values.size());
        }
        break;
      }
      case SimpleImputerStrategy::Median: {
        if (!values.empty()) fill = quantile(std::move(values), 0.5);
        break;
      }
      case SimpleImputerStrategy::MostFrequent: {
        fill = most_frequent(values, options_.empty_feature_fallback);
        break;
      }
      case SimpleImputerStrategy::Constant: {
        fill = options_.constant_value;
        break;
      }
    }
    fill_values_[static_cast<std::size_t>(f)] = fill;
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> SimpleImputer::map(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "SimpleImputer::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "SimpleImputer::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out = matrix.values;
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t idx = offset(matrix, i, f);
      if (std::isnan(out[idx])) out[idx] = fill_values_[static_cast<std::size_t>(f)];
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> SimpleImputer::learn_map(const FeatureBatch& batch,
                                          const TrainOptions& learn_options,
                                          const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

OneHotEncoder::OneHotEncoder(OneHotEncoderOptions options) : options_(options) {}

bool OneHotEncoder::trained() const noexcept { return trained_; }

Status OneHotEncoder::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status max_cat_status = validate_max_categories(options_.max_categories, "OneHotEncoder");
  if (!max_cat_status.ok()) return max_cat_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "OneHotEncoder::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  category_keys_.assign(static_cast<std::size_t>(feature_count_), {});
  feature_offsets_.assign(static_cast<std::size_t>(feature_count_), 0);
  feature_widths_.assign(static_cast<std::size_t>(feature_count_), 0);
  encoded_feature_count_ = 0;

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    std::unordered_map<std::uint64_t, std::int64_t> counts{};
    counts.reserve(static_cast<std::size_t>(matrix.sample_count));

    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const double v = matrix.values[offset(matrix, i, f)];
      if (std::isnan(v)) continue;
      ++counts[key_for_value(v)];
    }

    std::vector<std::pair<std::uint64_t, std::int64_t>> ranked(counts.begin(), counts.end());
    const std::size_t cap = options_.max_categories > 0 ? static_cast<std::size_t>(options_.max_categories) : ranked.size();

    bool active = true;
    if (ranked.size() > cap) {
      switch (options_.overflow) {
        case CategoryOverflowPolicy::Error: {
          return Status::invalid_argument("OneHotEncoder observed feature cardinality above max_categories.");
        }
        case CategoryOverflowPolicy::SkipFeature: {
          active = false;
          ranked.clear();
          break;
        }
        case CategoryOverflowPolicy::KeepMostFrequent: {
          std::stable_sort(ranked.begin(),
                           ranked.end(),
                           [](const auto& lhs, const auto& rhs) {
                             if (lhs.second != rhs.second) return lhs.second > rhs.second;
                             return std::bit_cast<double>(lhs.first) < std::bit_cast<double>(rhs.first);
                           });
          ranked.resize(cap);
          break;
        }
      }
    }

    std::vector<std::uint64_t> keys{};
    keys.reserve(ranked.size());
    for (const auto& [key, _count] : ranked) keys.push_back(key);
    std::sort(keys.begin(), keys.end(), [](std::uint64_t a, std::uint64_t b) {
      return std::bit_cast<double>(a) < std::bit_cast<double>(b);
    });

    if (!active) keys.clear();
    category_keys_[static_cast<std::size_t>(f)] = std::move(keys);

    std::int64_t width = static_cast<std::int64_t>(category_keys_[static_cast<std::size_t>(f)].size());
    if (active && options_.include_unknown_bucket) ++width;
    feature_offsets_[static_cast<std::size_t>(f)] = encoded_feature_count_;
    feature_widths_[static_cast<std::size_t>(f)] = width;
    encoded_feature_count_ += width;
  }

  if (encoded_feature_count_ <= 0) {
    return Status::invalid_argument("OneHotEncoder produced zero output features; adjust category options.");
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> OneHotEncoder::map(const FeatureBatch& batch, const InferOptions& options) const {
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "OneHotEncoder::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "OneHotEncoder::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out(static_cast<std::size_t>(matrix.sample_count * encoded_feature_count_), 0.0);
  for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
    const auto& keys = category_keys_[static_cast<std::size_t>(f)];
    const std::int64_t start = feature_offsets_[static_cast<std::size_t>(f)];
    const std::int64_t width = feature_widths_[static_cast<std::size_t>(f)];
    const std::int64_t unknown_idx = options_.include_unknown_bucket ? start + static_cast<std::int64_t>(keys.size()) : -1;
    if (width <= 0) continue;

    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const double v = matrix.values[offset(matrix, i, f)];
      bool unknown = std::isnan(v);
      std::int64_t local_idx = -1;
      if (!unknown) {
        const std::uint64_t key = key_for_value(v);
        auto it = std::find(keys.begin(), keys.end(), key);
        if (it == keys.end()) {
          unknown = true;
        } else {
          local_idx = static_cast<std::int64_t>(std::distance(keys.begin(), it));
        }
      }

      if (unknown) {
        switch (options.unknown_categorical) {
          case UnknownCategoryPolicy::Error: {
            return StatusOr<Series>(Status::invalid_argument("OneHotEncoder saw unknown category during map()."));
          }
          case UnknownCategoryPolicy::MissingBucket: {
            if (unknown_idx >= 0) {
              out[out_offset(matrix.layout, matrix.sample_count, encoded_feature_count_, i, unknown_idx)] = 1.0;
            }
            break;
          }
          case UnknownCategoryPolicy::Ignore: {
            break;
          }
        }
        continue;
      }

      const std::int64_t encoded_idx = start + local_idx;
      out[out_offset(matrix.layout, matrix.sample_count, encoded_feature_count_, i, encoded_idx)] = 1.0;
    }
  }

  return StatusOr<Series>(from_dense(matrix, std::move(out), encoded_feature_count_));
}

StatusOr<Series> OneHotEncoder::learn_map(const FeatureBatch& batch,
                                          const TrainOptions& learn_options,
                                          const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

FrequencyEncoder::FrequencyEncoder(FrequencyEncoderOptions options) : options_(options) {}

bool FrequencyEncoder::trained() const noexcept { return trained_; }

Status FrequencyEncoder::learn(const FeatureBatch& batch, const TrainOptions& options) {
  (void)options;
  Status max_cat_status = validate_max_categories(options_.max_categories, "FrequencyEncoder");
  if (!max_cat_status.ok()) return max_cat_status;
  Status unknown_status = validate_unit_interval(options_.unknown_frequency, "FrequencyEncoder");
  if (!unknown_status.ok()) return unknown_status;

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "FrequencyEncoder::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseMatrix matrix = matrix_or.move_value();

  layout_ = matrix.layout;
  feature_count_ = matrix.feature_count;
  category_keys_.assign(static_cast<std::size_t>(feature_count_), {});
  category_freqs_.assign(static_cast<std::size_t>(feature_count_), {});

  for (std::int64_t f = 0; f < feature_count_; ++f) {
    std::unordered_map<std::uint64_t, std::int64_t> counts{};
    counts.reserve(static_cast<std::size_t>(matrix.sample_count));

    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      const double v = matrix.values[offset(matrix, i, f)];
      if (std::isnan(v)) continue;
      ++counts[key_for_value(v)];
    }

    std::vector<std::pair<std::uint64_t, std::int64_t>> ranked(counts.begin(), counts.end());
    const std::size_t cap = options_.max_categories > 0 ? static_cast<std::size_t>(options_.max_categories) : ranked.size();
    if (ranked.size() > cap) {
      switch (options_.overflow) {
        case CategoryOverflowPolicy::Error: {
          return Status::invalid_argument("FrequencyEncoder observed feature cardinality above max_categories.");
        }
        case CategoryOverflowPolicy::SkipFeature: {
          ranked.clear();
          break;
        }
        case CategoryOverflowPolicy::KeepMostFrequent: {
          std::stable_sort(ranked.begin(),
                           ranked.end(),
                           [](const auto& lhs, const auto& rhs) {
                             if (lhs.second != rhs.second) return lhs.second > rhs.second;
                             return std::bit_cast<double>(lhs.first) < std::bit_cast<double>(rhs.first);
                           });
          ranked.resize(cap);
          break;
        }
      }
    }

    const double total = static_cast<double>(std::accumulate(
        ranked.begin(),
        ranked.end(),
        std::int64_t{0},
        [](std::int64_t acc, const std::pair<std::uint64_t, std::int64_t>& item) { return acc + item.second; }));

    std::sort(ranked.begin(), ranked.end(), [](const auto& lhs, const auto& rhs) {
      return std::bit_cast<double>(lhs.first) < std::bit_cast<double>(rhs.first);
    });

    auto& keys = category_keys_[static_cast<std::size_t>(f)];
    auto& freqs = category_freqs_[static_cast<std::size_t>(f)];
    keys.reserve(ranked.size());
    freqs.reserve(ranked.size());
    for (const auto& [key, count] : ranked) {
      keys.push_back(key);
      freqs.push_back(total > 0.0 ? static_cast<double>(count) / total : 0.0);
    }
  }

  trained_ = true;
  return Status::success();
}

StatusOr<Series> FrequencyEncoder::map(const FeatureBatch& batch, const InferOptions& options) const {
  Status ready = validate_trained_input(trained_, layout_, feature_count_, batch, "FrequencyEncoder::map");
  if (!ready.ok()) return StatusOr<Series>(std::move(ready));

  StatusOr<DenseMatrix> matrix_or = to_dense(batch, "FrequencyEncoder::map");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseMatrix matrix = matrix_or.move_value();

  std::vector<double> out(static_cast<std::size_t>(matrix.sample_count * matrix.feature_count), 0.0);
  for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
    for (std::int64_t f = 0; f < matrix.feature_count; ++f) {
      const std::size_t out_idx = offset(matrix, i, f);
      const auto& keys = category_keys_[static_cast<std::size_t>(f)];
      const auto& freqs = category_freqs_[static_cast<std::size_t>(f)];

      const double v = matrix.values[offset(matrix, i, f)];
      bool unknown = std::isnan(v);
      double mapped = 0.0;
      if (!unknown) {
        const std::uint64_t key = key_for_value(v);
        auto it = std::find(keys.begin(), keys.end(), key);
        if (it == keys.end()) {
          unknown = true;
        } else {
          const std::size_t idx = static_cast<std::size_t>(std::distance(keys.begin(), it));
          mapped = freqs[idx];
        }
      }

      if (unknown) {
        switch (options.unknown_categorical) {
          case UnknownCategoryPolicy::Error: {
            return StatusOr<Series>(Status::invalid_argument("FrequencyEncoder saw unknown category during map()."));
          }
          case UnknownCategoryPolicy::MissingBucket: {
            mapped = options_.unknown_frequency;
            break;
          }
          case UnknownCategoryPolicy::Ignore: {
            mapped = 0.0;
            break;
          }
        }
      }
      out[out_idx] = mapped;
    }
  }
  return StatusOr<Series>(from_dense_like(matrix, std::move(out)));
}

StatusOr<Series> FrequencyEncoder::learn_map(const FeatureBatch& batch,
                                             const TrainOptions& learn_options,
                                             const InferOptions& infer_options) {
  Status s = learn(batch, learn_options);
  if (!s.ok()) return StatusOr<Series>(std::move(s));
  return map(batch, infer_options);
}

} // namespace pluss::ai::nodej::prep
