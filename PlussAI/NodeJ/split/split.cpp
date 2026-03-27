#include "pluss/ai/nodej/split.h"
#include "pluss/tils.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

[[nodiscard]] Config index_config() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

[[nodiscard]] Series make_index_series(const std::vector<std::int64_t>& indices) {
  return pluss::make_series(indices, {static_cast<std::int64_t>(indices.size())}, index_config());
}

[[nodiscard]] Status validate_holdout_args(std::int64_t sample_count, const SplitOptions& options) {
  if (sample_count < 2) {
    return Status::invalid_argument("holdout_split requires sample_count >= 2.");
  }
  if (!std::isfinite(options.holdout_fraction) || !(options.holdout_fraction > 0.0 && options.holdout_fraction < 1.0)) {
    return Status::invalid_argument("SplitOptions.holdout_fraction must be finite and in (0, 1).");
  }
  return Status::success();
}

[[nodiscard]] Status validate_fold_args(std::int64_t sample_count, const SplitOptions& options) {
  if (sample_count < 2) {
    return Status::invalid_argument("fold_plan requires sample_count >= 2.");
  }
  if (options.folds < 2) {
    return Status::invalid_argument("SplitOptions.folds must be >= 2.");
  }
  if (options.folds > sample_count) {
    return Status::invalid_argument("SplitOptions.folds must be <= sample_count.");
  }
  return Status::success();
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

[[nodiscard]] std::uint64_t compose_seed(const RandomState& random) {
  if (random.deterministic) return random.seed;

  const auto now_ticks = static_cast<std::uint64_t>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());
  std::uint64_t seed = splitmix64(now_ticks);
  if (random.seed != 0) seed ^= splitmix64(random.seed);

  std::random_device rd;
  seed ^= splitmix64((static_cast<std::uint64_t>(rd()) << 32) ^ static_cast<std::uint64_t>(rd()));
  return seed;
}

class NumpyRandomState final {
 public:
  explicit NumpyRandomState(std::uint32_t seed) : mt_(seed) {}

  [[nodiscard]] std::uint32_t rand_u32() { return mt_(); }

  [[nodiscard]] std::int64_t randint_exclusive(std::int64_t high_exclusive) {
    if (high_exclusive <= 0) return 0;
    return static_cast<std::int64_t>(rk_interval_(static_cast<std::uint32_t>(high_exclusive - 1)));
  }

 private:
  [[nodiscard]] std::uint32_t rk_interval_(std::uint32_t max_inclusive) {
    // Match NumPy RandomState randint path (randomkit rk_interval).
    std::uint32_t mask = max_inclusive;
    mask |= mask >> 1;
    mask |= mask >> 2;
    mask |= mask >> 4;
    mask |= mask >> 8;
    mask |= mask >> 16;
    for (;;) {
      const std::uint32_t value = rand_u32() & mask;
      if (value <= max_inclusive) return value;
    }
  }

  std::mt19937 mt_;
};

template <class T>
void shuffle_like_numpy(std::vector<T>& values, NumpyRandomState& rng) {
  if (values.size() < 2) return;
  for (std::size_t i = values.size(); i > 1; --i) {
    const std::size_t j = static_cast<std::size_t>(rng.randint_exclusive(static_cast<std::int64_t>(i)));
    std::swap(values[i - 1], values[j]);
  }
}

[[nodiscard]] std::vector<std::int64_t> make_indices(std::int64_t sample_count, const SplitOptions& options) {
  std::vector<std::int64_t> indices(static_cast<std::size_t>(sample_count));
  std::iota(indices.begin(), indices.end(), std::int64_t{0});
  if (options.shuffle) {
    NumpyRandomState rng(static_cast<std::uint32_t>(compose_seed(options.random)));
    shuffle_like_numpy(indices, rng);
  }
  return indices;
}

[[nodiscard]] std::int64_t holdout_test_count(std::int64_t sample_count, double holdout_fraction) {
  // sklearn-like default: for float test_size, use ceil(n_samples * test_size)
  // when train_size is omitted.
  std::int64_t test_count = static_cast<std::int64_t>(std::ceil(static_cast<double>(sample_count) * holdout_fraction));
  test_count = std::max<std::int64_t>(1, test_count);
  test_count = std::min<std::int64_t>(sample_count - 1, test_count);
  return test_count;
}

[[nodiscard]] pluss::loader::TimeSeriesSplitOptions to_time_series_options(const SplitOptions& options) {
  pluss::loader::TimeSeriesSplitOptions out{};
  out.n_splits = options.folds;
  out.gap = options.time_gap;
  if (options.time_test_size > 0) out.test_size = options.time_test_size;
  if (options.time_max_train_size > 0) out.max_train_size = options.time_max_train_size;
  return out;
}

[[nodiscard]] StatusOr<std::vector<double>> flatten_label_values(const Series& labels) {
  if (!labels.defined()) return StatusOr<std::vector<double>>(Status::invalid_argument("labels must be a defined Series."));
  if (labels.numel() < 2) return StatusOr<std::vector<double>>(Status::invalid_argument("labels must contain at least 2 observations."));

  const Series flat = labels.to(ScalarType::Float64).reshape({labels.numel()});
  if (flat.isnan().any().item<bool>()) {
    return StatusOr<std::vector<double>>(Status::invalid_argument("labels must not contain NaN."));
  }
  std::vector<double> values = flat.to_vec<double>();
  return StatusOr<std::vector<double>>(std::move(values));
}

[[nodiscard]] std::vector<std::vector<std::int64_t>> class_partitions(const std::vector<double>& labels) {
  // sklearn np.unique returns sorted classes; keep that ordering for parity.
  std::vector<double> unique_labels = labels;
  for (double& value : unique_labels) {
    if (value == 0.0) value = 0.0; // normalize -0.0/+0.0
  }
  std::sort(unique_labels.begin(), unique_labels.end());
  unique_labels.erase(std::unique(unique_labels.begin(), unique_labels.end()), unique_labels.end());

  std::map<double, std::size_t> class_to_bucket{};
  std::vector<std::vector<std::int64_t>> buckets(unique_labels.size());
  for (std::size_t i = 0; i < unique_labels.size(); ++i) {
    class_to_bucket.emplace(unique_labels[i], i);
  }

  for (std::size_t i = 0; i < labels.size(); ++i) {
    double value = labels[i];
    if (value == 0.0) value = 0.0;
    const auto it = class_to_bucket.find(value);
    PLUSS_CHECK(it != class_to_bucket.end(), "class_partitions: internal class lookup mismatch");
    buckets[it->second].push_back(static_cast<std::int64_t>(i));
  }
  return buckets;
}

[[nodiscard]] std::vector<std::int64_t> approximate_mode(const std::vector<std::int64_t>& class_counts,
                                                         std::int64_t draws,
                                                         NumpyRandomState& rng) {
  const std::size_t k = class_counts.size();
  std::vector<std::int64_t> out(k, 0);
  if (k == 0 || draws <= 0) return out;

  const std::int64_t total = std::accumulate(class_counts.begin(), class_counts.end(), std::int64_t{0});
  if (total <= 0) return out;

  std::vector<double> continuous(k, 0.0);
  std::vector<double> remainder(k, 0.0);
  std::int64_t assigned = 0;
  for (std::size_t i = 0; i < k; ++i) {
    continuous[i] = (static_cast<double>(class_counts[i]) / static_cast<double>(total)) * static_cast<double>(draws);
    const auto floored = static_cast<std::int64_t>(std::floor(continuous[i]));
    out[i] = std::clamp<std::int64_t>(floored, 0, class_counts[i]);
    remainder[i] = continuous[i] - static_cast<double>(out[i]);
    assigned += out[i];
  }

  std::int64_t need = draws - assigned;
  while (need > 0) {
    std::vector<double> values = remainder;
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
    std::reverse(values.begin(), values.end());
    for (double value : values) {
      std::vector<std::size_t> inds{};
      inds.reserve(k);
      for (std::size_t i = 0; i < k; ++i) {
        if (remainder[i] == value) inds.push_back(i);
      }
      if (inds.empty()) continue;
      const std::int64_t add_now = std::min<std::int64_t>(need, static_cast<std::int64_t>(inds.size()));
      std::vector<std::size_t> pool = inds;
      shuffle_like_numpy(pool, rng);
      for (std::int64_t j = 0; j < add_now; ++j) {
        ++out[pool[static_cast<std::size_t>(j)]];
      }
      need -= add_now;
      if (need == 0) break;
    }
  }

  return out;
}

[[nodiscard]] Status validate_stratify_counts(const std::vector<std::vector<std::int64_t>>& buckets,
                                              std::int64_t n_train,
                                              std::int64_t n_test) {
  if (buckets.empty()) {
    return Status::invalid_argument("strat_holdout_split requires at least one class.");
  }
  std::int64_t min_class = std::numeric_limits<std::int64_t>::max();
  for (const auto& bucket : buckets) {
    min_class = std::min<std::int64_t>(min_class, static_cast<std::int64_t>(bucket.size()));
  }
  if (min_class < 2) {
    return Status::invalid_argument("strat_holdout_split requires every class to have at least 2 observations.");
  }
  if (n_train < static_cast<std::int64_t>(buckets.size())) {
    return Status::invalid_argument("strat_holdout_split requires train size to be >= number of classes.");
  }
  if (n_test < static_cast<std::int64_t>(buckets.size())) {
    return Status::invalid_argument("strat_holdout_split requires test size to be >= number of classes.");
  }
  return Status::success();
}

} // namespace

StatusOr<IndexSplit> holdout_split(std::int64_t sample_count, const SplitOptions& options) {
  Status args_status = validate_holdout_args(sample_count, options);
  if (!args_status.ok()) return StatusOr<IndexSplit>(std::move(args_status));

  const std::int64_t test_count = holdout_test_count(sample_count, options.holdout_fraction);
  const std::int64_t train_count = sample_count - test_count;
  std::vector<std::int64_t> indices(static_cast<std::size_t>(sample_count));
  std::iota(indices.begin(), indices.end(), std::int64_t{0});
  if (options.shuffle) {
    NumpyRandomState rng(static_cast<std::uint32_t>(compose_seed(options.random)));
    shuffle_like_numpy(indices, rng);
  }

  std::vector<std::int64_t> train_indices{};
  std::vector<std::int64_t> test_indices{};
  train_indices.reserve(static_cast<std::size_t>(train_count));
  test_indices.reserve(static_cast<std::size_t>(test_count));
  if (options.shuffle) {
    test_indices.insert(test_indices.end(), indices.begin(), indices.begin() + test_count);
    train_indices.insert(train_indices.end(), indices.begin() + test_count, indices.begin() + test_count + train_count);
  } else {
    train_indices.insert(train_indices.end(), indices.begin(), indices.begin() + train_count);
    test_indices.insert(test_indices.end(), indices.begin() + train_count, indices.begin() + train_count + test_count);
  }

  return StatusOr<IndexSplit>(IndexSplit{
      .train_indices = make_index_series(train_indices),
      .test_indices = make_index_series(test_indices),
  });
}

StatusOr<IndexSplit> strat_holdout_split(const Series& labels, const SplitOptions& options) {
  StatusOr<std::vector<double>> labels_or = flatten_label_values(labels);
  if (!labels_or.ok()) return StatusOr<IndexSplit>(labels_or.status());

  const std::vector<double>& label_values = labels_or.value();
  const std::int64_t sample_count = static_cast<std::int64_t>(label_values.size());

  Status args_status = validate_holdout_args(sample_count, options);
  if (!args_status.ok()) return StatusOr<IndexSplit>(std::move(args_status));

  std::vector<std::vector<std::int64_t>> buckets = class_partitions(label_values);
  const std::int64_t test_total = holdout_test_count(sample_count, options.holdout_fraction);
  const std::int64_t train_total = sample_count - test_total;
  Status strat_ok = validate_stratify_counts(buckets, train_total, test_total);
  if (!strat_ok.ok()) return StatusOr<IndexSplit>(std::move(strat_ok));

  std::vector<std::int64_t> class_counts{};
  class_counts.reserve(buckets.size());
  for (const auto& bucket : buckets) {
    class_counts.push_back(static_cast<std::int64_t>(bucket.size()));
  }

  NumpyRandomState rng(static_cast<std::uint32_t>(compose_seed(options.random)));
  const std::vector<std::int64_t> per_class_train = approximate_mode(class_counts, train_total, rng);

  std::vector<std::int64_t> remaining_counts(class_counts.size(), 0);
  for (std::size_t i = 0; i < class_counts.size(); ++i) {
    remaining_counts[i] = class_counts[i] - per_class_train[i];
  }
  const std::vector<std::int64_t> per_class_test = approximate_mode(remaining_counts, test_total, rng);

  std::vector<std::int64_t> test_indices{};
  test_indices.reserve(static_cast<std::size_t>(test_total));
  std::vector<std::int64_t> train_indices{};
  train_indices.reserve(static_cast<std::size_t>(train_total));

  for (std::size_t i = 0; i < buckets.size(); ++i) {
    auto bucket = buckets[i];
    if (options.shuffle) {
      shuffle_like_numpy(bucket, rng);
    }
    const std::int64_t train_take = std::clamp<std::int64_t>(per_class_train[i], 0, static_cast<std::int64_t>(bucket.size()));
    const std::int64_t test_take = std::clamp<std::int64_t>(per_class_test[i], 0, static_cast<std::int64_t>(bucket.size()) - train_take);
    train_indices.insert(train_indices.end(), bucket.begin(), bucket.begin() + train_take);
    test_indices.insert(test_indices.end(), bucket.begin() + train_take, bucket.begin() + train_take + test_take);
  }

  if (test_indices.empty() || train_indices.empty()) {
    return StatusOr<IndexSplit>(Status::internal("strat_holdout_split produced an empty train or test partition."));
  }

  if (options.shuffle) {
    shuffle_like_numpy(train_indices, rng);
    shuffle_like_numpy(test_indices, rng);
  }
  return StatusOr<IndexSplit>(IndexSplit{
      .train_indices = make_index_series(train_indices),
      .test_indices = make_index_series(test_indices),
  });
}

StatusOr<SplitPlan> fold_plan(std::int64_t sample_count, const SplitOptions& options) {
  Status args_status = validate_fold_args(sample_count, options);
  if (!args_status.ok()) return StatusOr<SplitPlan>(std::move(args_status));

  const std::vector<std::int64_t> indices = make_indices(sample_count, options);
  const std::int64_t fold_count = options.folds;
  const std::int64_t base = sample_count / fold_count;
  const std::int64_t rem = sample_count % fold_count;

  SplitPlan plan{};
  plan.folds.reserve(static_cast<std::size_t>(fold_count));

  std::int64_t offset = 0;
  for (std::int64_t fold_id = 0; fold_id < fold_count; ++fold_id) {
    const std::int64_t test_size = base + (fold_id < rem ? 1 : 0);
    const std::int64_t end = offset + test_size;

    std::vector<std::int64_t> test_indices(indices.begin() + offset, indices.begin() + end);
    std::vector<std::int64_t> train_indices{};
    train_indices.reserve(static_cast<std::size_t>(sample_count - test_size));
    train_indices.insert(train_indices.end(), indices.begin(), indices.begin() + offset);
    train_indices.insert(train_indices.end(), indices.begin() + end, indices.end());

    std::sort(test_indices.begin(), test_indices.end());
    std::sort(train_indices.begin(), train_indices.end());

    plan.folds.push_back(IndexSplit{
        .train_indices = make_index_series(train_indices),
        .test_indices = make_index_series(test_indices),
    });
    offset = end;
  }

  return StatusOr<SplitPlan>(std::move(plan));
}

StatusOr<SplitPlan> time_series_fold_plan(std::int64_t sample_count, const SplitOptions& options) {
  if (sample_count < 2) {
    return StatusOr<SplitPlan>(Status::invalid_argument("time_series_fold_plan requires sample_count >= 2."));
  }
  if (options.shuffle) {
    return StatusOr<SplitPlan>(
        Status::invalid_argument("time_series_fold_plan requires SplitOptions.shuffle=false."));
  }

  const auto ts_opts = to_time_series_options(options);
  pluss::loader::TimeSeriesIndexSplit plan_indices{};
  try {
    plan_indices = pluss::loader::time_series_split_indices(sample_count, ts_opts);
  } catch (const pluss::codem::Error& e) {
    return StatusOr<SplitPlan>(Status::invalid_argument(e.what()));
  } catch (const std::exception& e) {
    return StatusOr<SplitPlan>(Status::invalid_argument(e.what()));
  }

  SplitPlan plan{};
  plan.folds.reserve(plan_indices.folds.size());
  for (const auto& fold : plan_indices.folds) {
    plan.folds.push_back(IndexSplit{
        .train_indices = make_index_series(fold.train_indices),
        .test_indices = make_index_series(fold.test_indices),
    });
  }
  return StatusOr<SplitPlan>(std::move(plan));
}

StatusOr<SplitPlan> strat_fold_plan(const Series& labels, const SplitOptions& options) {
  StatusOr<std::vector<double>> labels_or = flatten_label_values(labels);
  if (!labels_or.ok()) return StatusOr<SplitPlan>(labels_or.status());

  const std::vector<double>& label_values = labels_or.value();
  const std::int64_t sample_count = static_cast<std::int64_t>(label_values.size());

  Status args_status = validate_fold_args(sample_count, options);
  if (!args_status.ok()) return StatusOr<SplitPlan>(std::move(args_status));

  const std::int64_t fold_count = options.folds;
  std::vector<std::vector<std::int64_t>> buckets = class_partitions(label_values);
  std::vector<std::vector<std::int64_t>> test_by_fold(static_cast<std::size_t>(fold_count));

  for (const auto& bucket : buckets) {
    for (std::size_t i = 0; i < bucket.size(); ++i) {
      const std::int64_t fold_id = static_cast<std::int64_t>(i % static_cast<std::size_t>(fold_count));
      test_by_fold[static_cast<std::size_t>(fold_id)].push_back(bucket[i]);
    }
  }

  SplitPlan plan{};
  plan.folds.reserve(static_cast<std::size_t>(fold_count));

  for (std::int64_t fold_id = 0; fold_id < fold_count; ++fold_id) {
    std::vector<std::int64_t> test_indices = test_by_fold[static_cast<std::size_t>(fold_id)];
    std::sort(test_indices.begin(), test_indices.end());

    std::vector<bool> is_test(static_cast<std::size_t>(sample_count), false);
    for (std::int64_t idx : test_indices) {
      is_test[static_cast<std::size_t>(idx)] = true;
    }

    std::vector<std::int64_t> train_indices{};
    train_indices.reserve(static_cast<std::size_t>(sample_count - static_cast<std::int64_t>(test_indices.size())));
    for (std::int64_t i = 0; i < sample_count; ++i) {
      if (!is_test[static_cast<std::size_t>(i)]) train_indices.push_back(i);
    }

    plan.folds.push_back(IndexSplit{
        .train_indices = make_index_series(train_indices),
        .test_indices = make_index_series(test_indices),
    });
  }

  return StatusOr<SplitPlan>(std::move(plan));
}

StatusOr<FeatureBatch> slice_feature_batch(const FeatureBatch& in, const Series& indices) {
  Status check = validate_features(in);
  if (!check.ok()) return StatusOr<FeatureBatch>(std::move(check));
  if (!indices.defined()) {
    return StatusOr<FeatureBatch>(Status::invalid_argument("slice_feature_batch requires defined indices."));
  }

  const Series idx = indices.to(in.x.platform(), ScalarType::Int64).reshape({indices.numel()});
  FeatureBatch out = in;
  if (in.layout == ObservationLayout::Rows) {
    out.x = in.x.index_select(0, idx);
  } else {
    out.x = in.x.index_select(1, idx);
  }
  Status out_check = validate_features(out);
  if (!out_check.ok()) return StatusOr<FeatureBatch>(std::move(out_check));
  return StatusOr<FeatureBatch>(std::move(out));
}

StatusOr<LabeledBatch> slice_labeled_batch(const LabeledBatch& in, const Series& indices) {
  Status check = validate_labeled(in);
  if (!check.ok()) return StatusOr<LabeledBatch>(std::move(check));
  if (!indices.defined()) {
    return StatusOr<LabeledBatch>(Status::invalid_argument("slice_labeled_batch requires defined indices."));
  }

  StatusOr<FeatureBatch> features_or = slice_feature_batch(in.features, indices);
  if (!features_or.ok()) return StatusOr<LabeledBatch>(features_or.status());

  const Series y_idx = indices.to(in.y.platform(), ScalarType::Int64).reshape({indices.numel()});
  LabeledBatch out{};
  out.features = features_or.move_value();
  out.y = in.y.reshape({in.y.numel()}).index_select(0, y_idx);
  if (in.sample_weights.defined()) {
    const Series w_idx = indices.to(in.sample_weights.platform(), ScalarType::Int64).reshape({indices.numel()});
    out.sample_weights = in.sample_weights.reshape({in.sample_weights.numel()}).index_select(0, w_idx);
  }
  Status out_check = validate_labeled(out);
  if (!out_check.ok()) return StatusOr<LabeledBatch>(std::move(out_check));
  return StatusOr<LabeledBatch>(std::move(out));
}

} // namespace pluss::ai::nodej
