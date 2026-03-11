#include "pluss/ai/nodej/split.h"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>
#include <unordered_map>
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

[[nodiscard]] std::vector<std::int64_t> make_indices(std::int64_t sample_count, const SplitOptions& options) {
  std::vector<std::int64_t> indices(static_cast<std::size_t>(sample_count));
  std::iota(indices.begin(), indices.end(), std::int64_t{0});
  if (options.shuffle) {
    std::mt19937_64 rng(compose_seed(options.random));
    std::shuffle(indices.begin(), indices.end(), rng);
  }
  return indices;
}

[[nodiscard]] std::int64_t holdout_test_count(std::int64_t sample_count, double holdout_fraction) {
  std::int64_t test_count = static_cast<std::int64_t>(std::llround(static_cast<double>(sample_count) * holdout_fraction));
  test_count = std::max<std::int64_t>(1, test_count);
  test_count = std::min<std::int64_t>(sample_count - 1, test_count);
  return test_count;
}

[[nodiscard]] IndexSplit build_holdout_from_indices(const std::vector<std::int64_t>& indices,
                                                    std::int64_t test_count) {
  std::vector<std::int64_t> test_indices(indices.begin(), indices.begin() + test_count);
  std::vector<std::int64_t> train_indices(indices.begin() + test_count, indices.end());
  std::sort(test_indices.begin(), test_indices.end());
  std::sort(train_indices.begin(), train_indices.end());
  return IndexSplit{
      .train_indices = make_index_series(train_indices),
      .test_indices = make_index_series(test_indices),
  };
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

[[nodiscard]] std::vector<std::vector<std::int64_t>> class_partitions(const std::vector<double>& labels,
                                                                      const SplitOptions& options) {
  std::unordered_map<std::uint64_t, std::size_t> class_to_bucket{};
  std::vector<std::vector<std::int64_t>> buckets{};
  buckets.reserve(labels.size());

  for (std::size_t i = 0; i < labels.size(); ++i) {
    double value = labels[i];
    if (value == 0.0) value = 0.0; // normalize -0.0/+0.0
    const std::uint64_t key = std::bit_cast<std::uint64_t>(value);
    auto it = class_to_bucket.find(key);
    if (it == class_to_bucket.end()) {
      const std::size_t bucket_id = buckets.size();
      class_to_bucket.emplace(key, bucket_id);
      buckets.push_back({});
      buckets.back().reserve(8);
      it = class_to_bucket.find(key);
    }
    buckets[it->second].push_back(static_cast<std::int64_t>(i));
  }

  if (options.shuffle) {
    std::mt19937_64 rng(compose_seed(options.random));
    for (auto& bucket : buckets) {
      std::shuffle(bucket.begin(), bucket.end(), rng);
    }
  }

  return buckets;
}

[[nodiscard]] std::vector<std::int64_t> stratified_test_counts(const std::vector<std::vector<std::int64_t>>& buckets,
                                                               std::int64_t target_total,
                                                               double holdout_fraction) {
  std::vector<std::int64_t> per_class_test(buckets.size(), 0);
  std::vector<double> residual(buckets.size(), 0.0);
  std::int64_t assigned = 0;

  for (std::size_t i = 0; i < buckets.size(); ++i) {
    const double exact = static_cast<double>(buckets[i].size()) * holdout_fraction;
    const std::int64_t base = static_cast<std::int64_t>(std::floor(exact));
    per_class_test[i] = std::min<std::int64_t>(base, static_cast<std::int64_t>(buckets[i].size()));
    residual[i] = exact - static_cast<double>(base);
    assigned += per_class_test[i];
  }

  while (assigned < target_total) {
    std::size_t best = buckets.size();
    double best_residual = -1.0;
    for (std::size_t i = 0; i < buckets.size(); ++i) {
      if (per_class_test[i] < static_cast<std::int64_t>(buckets[i].size()) && residual[i] > best_residual) {
        best = i;
        best_residual = residual[i];
      }
    }
    if (best == buckets.size()) break;
    ++per_class_test[best];
    ++assigned;
  }

  while (assigned > target_total) {
    std::size_t best = buckets.size();
    double best_residual = 2.0;
    for (std::size_t i = 0; i < buckets.size(); ++i) {
      if (per_class_test[i] > 0 && residual[i] < best_residual) {
        best = i;
        best_residual = residual[i];
      }
    }
    if (best == buckets.size()) break;
    --per_class_test[best];
    --assigned;
  }

  return per_class_test;
}

} // namespace

StatusOr<IndexSplit> holdout_split(std::int64_t sample_count, const SplitOptions& options) {
  Status args_status = validate_holdout_args(sample_count, options);
  if (!args_status.ok()) return StatusOr<IndexSplit>(std::move(args_status));

  std::vector<std::int64_t> indices = make_indices(sample_count, options);
  return StatusOr<IndexSplit>(build_holdout_from_indices(indices, holdout_test_count(sample_count, options.holdout_fraction)));
}

StatusOr<IndexSplit> strat_holdout_split(const Series& labels, const SplitOptions& options) {
  StatusOr<std::vector<double>> labels_or = flatten_label_values(labels);
  if (!labels_or.ok()) return StatusOr<IndexSplit>(labels_or.status());

  const std::vector<double>& label_values = labels_or.value();
  const std::int64_t sample_count = static_cast<std::int64_t>(label_values.size());

  Status args_status = validate_holdout_args(sample_count, options);
  if (!args_status.ok()) return StatusOr<IndexSplit>(std::move(args_status));

  std::vector<std::vector<std::int64_t>> buckets = class_partitions(label_values, options);
  const std::int64_t test_total = holdout_test_count(sample_count, options.holdout_fraction);
  const std::vector<std::int64_t> per_class_test = stratified_test_counts(buckets, test_total, options.holdout_fraction);

  std::vector<std::int64_t> test_indices{};
  test_indices.reserve(static_cast<std::size_t>(test_total));
  std::vector<std::int64_t> train_indices{};
  train_indices.reserve(static_cast<std::size_t>(sample_count - test_total));

  for (std::size_t i = 0; i < buckets.size(); ++i) {
    const auto& bucket = buckets[i];
    const std::int64_t cutoff = per_class_test[i];
    test_indices.insert(test_indices.end(), bucket.begin(), bucket.begin() + cutoff);
    train_indices.insert(train_indices.end(), bucket.begin() + cutoff, bucket.end());
  }

  if (test_indices.empty() || train_indices.empty()) {
    return StatusOr<IndexSplit>(Status::internal("strat_holdout_split produced an empty train or test partition."));
  }

  std::sort(test_indices.begin(), test_indices.end());
  std::sort(train_indices.begin(), train_indices.end());
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

StatusOr<SplitPlan> strat_fold_plan(const Series& labels, const SplitOptions& options) {
  StatusOr<std::vector<double>> labels_or = flatten_label_values(labels);
  if (!labels_or.ok()) return StatusOr<SplitPlan>(labels_or.status());

  const std::vector<double>& label_values = labels_or.value();
  const std::int64_t sample_count = static_cast<std::int64_t>(label_values.size());

  Status args_status = validate_fold_args(sample_count, options);
  if (!args_status.ok()) return StatusOr<SplitPlan>(std::move(args_status));

  const std::int64_t fold_count = options.folds;
  std::vector<std::vector<std::int64_t>> buckets = class_partitions(label_values, options);
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

} // namespace pluss::ai::nodej
