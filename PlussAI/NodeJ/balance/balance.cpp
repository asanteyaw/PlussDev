#include "pluss/ai/nodej/balance.h"
#include "pluss/ai/nodej/spatial_index.h"
#include "pluss/ai/nodej/split.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

using pluss::Config;
using pluss::ScalarType;

[[nodiscard]] Config f64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(platform);
}

[[nodiscard]] Config i64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
}

[[nodiscard]] bool all_finite(const Series& x) {
  return x.isfinite().all().item<bool>();
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

[[nodiscard]] std::uint64_t compose_seed(const RandomState& random) {
  if (random.deterministic) return random.seed;
  std::random_device rd;
  std::uint64_t seed = (static_cast<std::uint64_t>(rd()) << 32U) ^ static_cast<std::uint64_t>(rd());
  if (random.seed != 0) seed ^= splitmix64(random.seed);
  return splitmix64(seed);
}

[[nodiscard]] Status validate_sample_weights(const Series& weights,
                                             std::int64_t sample_count,
                                             const char* where,
                                             const char* label) {
  if (!weights.defined()) return Status::success();
  if (weights.numel() != sample_count) {
    return Status::shape_mismatch(std::string(where) + " " + label + " length must match sample count.");
  }
  if (!all_finite(weights)) {
    return Status::invalid_argument(std::string(where) + " " + label + " must be finite.");
  }
  if (weights.le(0.0).any().item<bool>()) {
    return Status::invalid_argument(std::string(where) + " " + label + " must be strictly positive.");
  }
  return Status::success();
}

[[nodiscard]] Series per_sample_from_class_weights(const Series& labels,
                                                   const Series& class_labels,
                                                   const Series& class_weights) {
  const std::int64_t n = labels.numel();
  const std::int64_t k = class_labels.numel();
  const Series membership = labels.reshape({n, 1}).eq(class_labels.reshape({1, k})).to(ScalarType::Float64);
  return membership.matmul(class_weights.reshape({k, 1})).reshape({n});
}

[[nodiscard]] StatusOr<Series> resolve_target_counts(const ClassMix& mix,
                                                     const BalanceTarget& target,
                                                     const char* where) {
  const std::int64_t k = mix.class_count;
  const Platform platform = mix.labels.platform();
  const Series counts = mix.counts.to(platform, ScalarType::Float64).reshape({k});
  if (target.kind == BalanceTargetKind::Keep) {
    return StatusOr<Series>(counts);
  }
  if (target.kind == BalanceTargetKind::Equalize) {
    const Series max_count = counts.max().reshape({1});
    return StatusOr<Series>(max_count.expand(std::vector<std::int64_t>{k}).reshape({k}));
  }
  if (target.kind == BalanceTargetKind::Counts) {
    if (!target.counts.defined()) {
      return StatusOr<Series>(Status::invalid_argument(std::string(where) + " BalanceTarget.counts must be defined."));
    }
    Series target_counts = target.counts.to(platform, ScalarType::Float64).reshape({target.counts.numel()});
    if (target_counts.numel() != k) {
      return StatusOr<Series>(Status::shape_mismatch(
          std::string(where) + " BalanceTarget.counts length must match class count."));
    }
    if (!all_finite(target_counts) || target_counts.lt(1.0).any().item<bool>()) {
      return StatusOr<Series>(Status::invalid_argument(
          std::string(where) + " BalanceTarget.counts values must be finite and >= 1."));
    }
    return StatusOr<Series>(target_counts.round().reshape({k}));
  }
  if (target.kind == BalanceTargetKind::Ratio) {
    if (k != 2) {
      return StatusOr<Series>(
          Status::invalid_argument(std::string(where) + " BalanceTargetKind::Ratio currently requires exactly two classes."));
    }
    if (!std::isfinite(target.ratio) || target.ratio <= 0.0 || target.ratio > 1.0) {
      return StatusOr<Series>(
          Status::invalid_argument(std::string(where) + " BalanceTarget.ratio must be finite and in (0, 1]."));
    }
    const double c0 = counts.index_select(0, pluss::series::arange(0, 1, i64_on(platform))).reshape({1}).host<double>().front();
    const double c1 = counts.index_select(0, pluss::series::arange(1, 2, i64_on(platform))).reshape({1}).host<double>().front();
    const double minority = std::min(c0, c1);
    const double majority = std::max(c0, c1);
    const double desired_minority = std::max(minority, std::ceil(target.ratio * majority));
    const std::vector<double> out = c0 <= c1
                                      ? std::vector<double>{desired_minority, majority}
                                      : std::vector<double>{majority, desired_minority};
    return StatusOr<Series>(pluss::make_series(out, {2}, f64_on(platform)));
  }
  return StatusOr<Series>(Status::invalid_argument(std::string(where) + " received unsupported BalanceTargetKind."));
}

[[nodiscard]] Series append_indices(Series out, const Series& part) {
  if (!part.defined() || part.numel() == 0) return out;
  if (!out.defined()) return part.reshape({part.numel()});
  return out.reshape({out.numel()}).unite({part.reshape({part.numel()})}, 0);
}

[[nodiscard]] Series append_f64(Series out, const Series& part) {
  if (!part.defined() || part.numel() == 0) return out;
  if (!out.defined()) return part.reshape({part.numel()});
  return out.reshape({out.numel()}).unite({part.reshape({part.numel()})}, 0);
}

[[nodiscard]] StatusOr<Series> select_class_indices(const Series& all_indices,
                                                    const Series& labels,
                                                    const Series& class_labels,
                                                    std::int64_t class_index) {
  const Platform platform = labels.platform();
  const Series pos = pluss::series::arange(class_index, class_index + 1, i64_on(platform));
  const Series class_value = class_labels.index_select(0, pos).reshape({1});
  const Series mask = labels.eq(class_value);
  Series selected = all_indices.masked_select(mask).to(platform, ScalarType::Int64);
  return StatusOr<Series>(selected.reshape({selected.numel()}));
}

[[nodiscard]] StatusOr<Series> select_class_indices_from_base(const Series& base_indices,
                                                              const Series& base_labels,
                                                              const Series& class_labels,
                                                              std::int64_t class_index) {
  const Platform platform = base_labels.platform();
  const Series pos = pluss::series::arange(class_index, class_index + 1, i64_on(platform));
  const Series class_value = class_labels.index_select(0, pos).reshape({1});
  const Series mask = base_labels.eq(class_value);
  Series selected = base_indices.to(platform, ScalarType::Int64).masked_select(mask).to(platform, ScalarType::Int64);
  return StatusOr<Series>(selected.reshape({selected.numel()}));
}

[[nodiscard]] StatusOr<Series> dense_feature_rows(const FeatureBatch& features, const char* where) {
  Status check = validate_features(features);
  if (!check.ok()) return StatusOr<Series>(std::move(check));
  if (features.x.dim() != 2) {
    return StatusOr<Series>(
        Status::not_implemented(std::string(where) + " BlendNeighbors currently requires rank-2 feature matrices."));
  }
  Series x = features.x.to(ScalarType::Float64);
  if (features.layout == ObservationLayout::Columns) {
    x = x.transpose(0, 1);
  }
  return StatusOr<Series>(x.reshape({features.sample_count(), features.feature_count()}));
}

[[nodiscard]] Series pairwise_distance2(const Series& a_rows, const Series& b_rows) {
  const Series dot = a_rows.matmul(b_rows.transpose(0, 1));
  const Series a_norm = a_rows.square().sum(1, true);
  const Series b_norm = b_rows.square().sum(1, true).transpose(0, 1);
  return a_norm.add(b_norm).sub(dot.mul(2.0)).relu();
}

[[nodiscard]] Series self_masked_distance2(const Series& feature_rows) {
  const std::int64_t n = feature_rows.sizes()[0];
  const Platform platform = feature_rows.platform();
  const Series dist2 = pairwise_distance2(feature_rows, feature_rows);
  const Series idx = pluss::series::arange(0, n, i64_on(platform));
  const Series eye = idx.reshape({n, 1}).eq(idx.reshape({1, n}));
  const Series large = dist2.max().reshape({1, 1}).add(1.0).mul(static_cast<double>(n + 1));
  return eye.where(large.expand(std::vector<std::int64_t>{n, n}), dist2).reshape({n, n});
}

[[nodiscard]] Series targetable_clean_mask(const Series& labels,
                                           const ClassMix& mix,
                                           bool protect_small_classes) {
  const std::int64_t n = labels.numel();
  const std::int64_t k = mix.class_count;
  if (!protect_small_classes) return pluss::series::ones({n}, Config{}.with_dtype(ScalarType::Bool).with_platform(labels.platform()));
  const Series counts = mix.counts.to(labels.platform(), ScalarType::Float64).reshape({k});
  const Series min_count = counts.min().reshape({1});
  const Series class_targetable = counts.gt(min_count).to(ScalarType::Float64).reshape({k});
  const Series membership = labels.reshape({n, 1}).eq(mix.labels.reshape({1, k})).to(ScalarType::Float64);
  return membership.matmul(class_targetable.reshape({k, 1})).reshape({n}).gt(0.0);
}

[[nodiscard]] StatusOr<Series> cleaned_keep_indices(const Series& feature_rows,
                                                    const Series& labels,
                                                    const BalanceTarget& target,
                                                    const ClassMix& mix,
                                                    const char* where) {
  const std::int64_t n = labels.numel();
  const Platform platform = labels.platform();
  const Series all_indices = pluss::series::arange(0, n, i64_on(platform));
  if (target.clean == BalanceCleanKind::None) {
    return StatusOr<Series>(all_indices.reshape({n}));
  }
  if (n <= 1) {
    return StatusOr<Series>(all_indices.reshape({n}));
  }
  if (target.clean_neighbors <= 0) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " BalanceTarget.clean_neighbors must be positive."));
  }

  const Series y = labels.to(platform, ScalarType::Float64).reshape({n});
  const Series removable = targetable_clean_mask(y, mix, target.protect_small_classes).reshape({n});
  const Series dist2 = self_masked_distance2(feature_rows.to(feature_rows.platform(), ScalarType::Float64));
  const Series sort_idx = dist2.argsort(1, false, true);

  Series remove_mask = pluss::series::zeros({n}, Config{}.with_dtype(ScalarType::Bool).with_platform(platform));
  if (target.clean == BalanceCleanKind::BoundaryPairs
      || target.clean == BalanceCleanKind::BoundaryPairsAndNeighborAgreement) {
    const Series first_col = pluss::series::arange(0, 1, i64_on(sort_idx.platform()));
    const Series nn = sort_idx.index_select(1, first_col).reshape({n}).to(platform, ScalarType::Int64);
    const Series nn_of_nn = nn.index_select(0, nn).reshape({n});
    const Series nn_label = y.index_select(0, nn).reshape({n});
    const Series reciprocal = nn_of_nn.eq(all_indices);
    const Series different = nn_label.ne(y);
    remove_mask = remove_mask.logical_or(removable.logical_and(reciprocal).logical_and(different));
  }

  if (target.clean == BalanceCleanKind::NeighborAgreement
      || target.clean == BalanceCleanKind::BoundaryPairsAndNeighborAgreement) {
    const std::int64_t neighbor_count = std::max<std::int64_t>(1, std::min<std::int64_t>(target.clean_neighbors, n - 1));
    const Series neighbor_cols = pluss::series::arange(0, neighbor_count, i64_on(sort_idx.platform()));
    const Series neighbor_idx = sort_idx.index_select(1, neighbor_cols).reshape({n, neighbor_count}).to(platform, ScalarType::Int64);
    const Series y_expand = y.reshape({1, n}).expand(std::vector<std::int64_t>{n, n});
    const Series neighbor_labels = y_expand.gather(1, neighbor_idx).reshape({n, neighbor_count});
    const Series same = neighbor_labels.eq(y.reshape({n, 1})).to(ScalarType::Float64);
    Series agrees = pluss::series::undefined();
    if (target.clean_vote == BalanceCleanVote::AnyDisagreement) {
      agrees = same.min(1, false).reshape({n}).gt(0.0);
    } else if (target.clean_vote == BalanceCleanVote::MajorityDisagreement) {
      agrees = same.sum(1, false).reshape({n}).ge(static_cast<double>(neighbor_count) * 0.5);
    } else {
      return StatusOr<Series>(Status::invalid_argument(std::string(where) + " received unsupported BalanceCleanVote."));
    }
    remove_mask = remove_mask.logical_or(removable.logical_and(agrees.logical_not()));
  }

  Series keep = all_indices.masked_select(remove_mask.logical_not()).to(platform, ScalarType::Int64);
  keep = keep.reshape({keep.numel()});
  if (keep.numel() <= 0) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " cleaning removed every row."));
  }
  const Series kept_labels = y.index_select(0, keep).reshape({keep.numel()});
  StatusOr<ClassMix> kept_mix_or = class_mix(kept_labels);
  if (!kept_mix_or.ok()) return StatusOr<Series>(kept_mix_or.status());
  if (kept_mix_or.value().class_count != mix.class_count) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " cleaning removed an entire class."));
  }
  return StatusOr<Series>(keep);
}

} // namespace

StatusOr<ClassMix> class_mix(const Series& labels) {
  constexpr const char* where = "class_mix";
  if (!labels.defined()) {
    return StatusOr<ClassMix>(Status::invalid_argument(std::string(where) + " requires defined labels."));
  }
  const std::int64_t n = labels.numel();
  if (n <= 0) {
    return StatusOr<ClassMix>(Status::invalid_argument(std::string(where) + " requires at least one label."));
  }

  Series y = labels.to(ScalarType::Float64).reshape({n});
  if (!all_finite(y)) {
    return StatusOr<ClassMix>(Status::invalid_argument(std::string(where) + " labels must be finite."));
  }

  Series classes = y.unique(true).sort(0, false, false);
  const std::int64_t k = classes.numel();
  if (k <= 0) {
    return StatusOr<ClassMix>(Status::invalid_argument(std::string(where) + " requires at least one class."));
  }

  const Series membership = y.reshape({n, 1}).eq(classes.reshape({1, k})).to(ScalarType::Float64);
  const Series counts = membership.sum(0, false).reshape({k}).to(y.platform(), ScalarType::Float64);
  if (counts.le(0.0).any().item<bool>()) {
    return StatusOr<ClassMix>(Status::internal(std::string(where) + " produced an empty class bucket."));
  }

  const Series total = counts.sum().reshape({1});
  const Series k_scalar = pluss::series::ones({1}, f64_on(y.platform())).mul(static_cast<double>(k));
  ClassMix out{};
  out.labels = classes.to(y.platform(), ScalarType::Float64).reshape({k});
  out.counts = counts;
  out.proportions = counts.div(total.clamp_min(1.0)).reshape({k});
  out.inverse_frequency = total.div(k_scalar.mul(counts).clamp_min(1.0)).reshape({k});
  out.sample_count = n;
  out.class_count = k;
  const double min_count = counts.min().reshape({1}).host<double>().front();
  const double max_count = counts.max().reshape({1}).host<double>().front();
  out.imbalance_ratio = (min_count > 0.0 && std::isfinite(min_count) && std::isfinite(max_count))
                            ? max_count / min_count
                            : 1.0;
  return StatusOr<ClassMix>(std::move(out));
}

StatusOr<WeightPlan> resolve_weight_plan(const Series& labels,
                                         const Series& batch_sample_weights,
                                         const TrainOptions& options) {
  constexpr const char* where = "resolve_weight_plan";
  StatusOr<ClassMix> mix_or = class_mix(labels);
  if (!mix_or.ok()) return StatusOr<WeightPlan>(mix_or.status());
  ClassMix mix = mix_or.move_value();
  const std::int64_t n = mix.sample_count;
  const std::int64_t k = mix.class_count;
  const Platform platform = mix.labels.platform();

  Series weights = pluss::series::ones({n}, f64_on(platform));

  Status sample_status = validate_sample_weights(batch_sample_weights, n, where, "batch.sample_weights");
  if (!sample_status.ok()) return StatusOr<WeightPlan>(std::move(sample_status));
  if (batch_sample_weights.defined()) {
    weights = weights.mul(batch_sample_weights.to(platform, ScalarType::Float64).reshape({n}));
  }

  sample_status = validate_sample_weights(options.sample_weights, n, where, "TrainOptions.sample_weights");
  if (!sample_status.ok()) return StatusOr<WeightPlan>(std::move(sample_status));
  if (options.sample_weights.defined()) {
    weights = weights.mul(options.sample_weights.to(platform, ScalarType::Float64).reshape({n}));
  }
  const Series resolved_sample_weights = weights.reshape({n});

  Series class_weights = pluss::series::ones({k}, f64_on(platform));
  if (options.class_weight == ClassWeightMode::Balanced) {
    class_weights = mix.inverse_frequency.to(platform, ScalarType::Float64).reshape({k});
  } else if (options.class_weight == ClassWeightMode::Custom) {
    if (!options.class_weights.defined()) {
      return StatusOr<WeightPlan>(Status::invalid_argument(
          std::string(where) + " TrainOptions.class_weights must be provided for ClassWeightMode::Custom."));
    }
    class_weights = options.class_weights.to(platform, ScalarType::Float64).reshape({options.class_weights.numel()});
    if (class_weights.numel() != k) {
      return StatusOr<WeightPlan>(Status::shape_mismatch(
          std::string(where) + " TrainOptions.class_weights length must match detected class count."));
    }
    if (!all_finite(class_weights) || class_weights.le(0.0).any().item<bool>()) {
      return StatusOr<WeightPlan>(Status::invalid_argument(
          std::string(where) + " TrainOptions.class_weights values must be finite and > 0."));
    }
  }

  weights = weights.mul(per_sample_from_class_weights(labels.to(platform, ScalarType::Float64).reshape({n}),
                                                      mix.labels,
                                                      class_weights));

  if (!all_finite(weights)) {
    return StatusOr<WeightPlan>(Status::numerical_failure(std::string(where) + " produced non-finite training weights."));
  }
  if (weights.le(0.0).all().item<bool>()) {
    return StatusOr<WeightPlan>(Status::invalid_argument(std::string(where) + " produced zero total training weight."));
  }

  const double mean_weight = weights.mean().reshape({1}).host<double>().front();
  if (!(std::isfinite(mean_weight) && mean_weight > 0.0)) {
    return StatusOr<WeightPlan>(Status::numerical_failure(std::string(where) + " produced invalid mean training weight."));
  }
  weights = weights.div(mean_weight).reshape({n});

  WeightPlan out{};
  out.weights = weights.to(platform, ScalarType::Float64).reshape({n});
  out.class_weights = class_weights.to(platform, ScalarType::Float64).reshape({k});
  out.sample_weights = resolved_sample_weights.to(platform, ScalarType::Float64).reshape({n});
  out.mix = std::move(mix);
  out.mean_weight = mean_weight;
  return StatusOr<WeightPlan>(std::move(out));
}

StatusOr<Series> resolve_training_weights(const Series& labels,
                                          const Series& batch_sample_weights,
                                          const TrainOptions& options) {
  StatusOr<WeightPlan> plan_or = resolve_weight_plan(labels, batch_sample_weights, options);
  if (!plan_or.ok()) return StatusOr<Series>(plan_or.status());
  return StatusOr<Series>(plan_or.value().weights);
}

StatusOr<BalancePlan> plan_balance(const Series& labels, const BalanceTarget& target, RandomState random) {
  constexpr const char* where = "plan_balance";
  if (target.clean != BalanceCleanKind::None) {
    return StatusOr<BalancePlan>(
        Status::invalid_argument(std::string(where) + " cleaning requires plan_balance(LabeledBatch, ...)."));
  }
  if (target.growth == BalanceGrowthKind::BlendNeighbors) {
    return StatusOr<BalancePlan>(
        Status::invalid_argument(std::string(where) + " BlendNeighbors requires plan_balance(LabeledBatch, ...)."));
  }
  StatusOr<ClassMix> before_or = class_mix(labels);
  if (!before_or.ok()) return StatusOr<BalancePlan>(before_or.status());
  ClassMix before = before_or.move_value();
  const std::int64_t n = before.sample_count;
  const std::int64_t k = before.class_count;
  const Platform platform = before.labels.platform();

  StatusOr<Series> target_counts_or = resolve_target_counts(before, target, where);
  if (!target_counts_or.ok()) return StatusOr<BalancePlan>(target_counts_or.status());
  const Series target_counts = target_counts_or.move_value().to(platform, ScalarType::Float64).reshape({k});

  const Series y = labels.to(platform, ScalarType::Float64).reshape({n});
  const Series all_indices = pluss::series::arange(0, n, i64_on(platform));
  const std::vector<double> current = before.counts.to(pluss::pCPU, ScalarType::Float64).reshape({k}).host<double>();
  const std::vector<double> desired = target_counts.to(pluss::pCPU, ScalarType::Float64).reshape({k}).host<double>();
  const std::uint64_t seed = compose_seed(random);

  Series planned = pluss::series::undefined();
  bool uses_replacement = false;
  for (std::int64_t ci = 0; ci < k; ++ci) {
    StatusOr<Series> class_idx_or = select_class_indices(all_indices, y, before.labels, ci);
    if (!class_idx_or.ok()) return StatusOr<BalancePlan>(class_idx_or.status());
    Series class_indices = class_idx_or.move_value();
    class_indices = class_indices.reshape({class_indices.numel()});
    const std::int64_t have = class_indices.numel();
    const std::int64_t want = static_cast<std::int64_t>(std::llround(desired[static_cast<std::size_t>(ci)]));
    if (want <= 0) {
      return StatusOr<BalancePlan>(Status::invalid_argument(std::string(where) + " target counts must be positive."));
    }

    Series selected = pluss::series::undefined();
    const std::uint64_t class_seed = splitmix64(seed + static_cast<std::uint64_t>(ci) * 0x9e3779b97f4a7c15ULL);
    if (want < have) {
      const Series perm = pluss::series::randperm(have, i64_on(platform), class_seed).reshape({have});
      const Series take = perm.index_select(0, pluss::series::arange(0, want, i64_on(platform))).reshape({want});
      selected = class_indices.index_select(0, take).reshape({want});
    } else if (want == have) {
      selected = class_indices.reshape({have});
    } else {
      const Series extra_pos = pluss::series::randint(0, have, {want - have}, i64_on(platform), class_seed).reshape({want - have});
      const Series extra = class_indices.index_select(0, extra_pos).reshape({want - have});
      selected = class_indices.reshape({have}).unite({extra}, 0).reshape({want});
      uses_replacement = true;
    }
    planned = append_indices(std::move(planned), selected);
  }

  if (!planned.defined() || planned.numel() <= 0) {
    return StatusOr<BalancePlan>(Status::internal(std::string(where) + " produced no row indices."));
  }
  const Series final_perm = pluss::series::randperm(planned.numel(), i64_on(platform), splitmix64(seed ^ 0x51ed2705ULL));
  planned = planned.index_select(0, final_perm).reshape({planned.numel()});

  const Series after_labels = y.index_select(0, planned.to(platform, ScalarType::Int64)).reshape({planned.numel()});
  StatusOr<ClassMix> after_or = class_mix(after_labels);
  if (!after_or.ok()) return StatusOr<BalancePlan>(after_or.status());

  BalancePlan out{};
  out.indices = planned.to(platform, ScalarType::Int64).reshape({planned.numel()});
  out.target_counts = target_counts.reshape({k});
  out.before = std::move(before);
  out.after = after_or.move_value();
  out.uses_replacement = uses_replacement;
  return StatusOr<BalancePlan>(std::move(out));
}

StatusOr<BalancePlan> plan_balance(const LabeledBatch& batch, const BalanceTarget& target, RandomState random) {
  constexpr const char* where = "plan_balance";
  Status check = validate_labeled(batch);
  if (!check.ok()) return StatusOr<BalancePlan>(std::move(check));
  if (target.clean == BalanceCleanKind::None
      && (target.growth == BalanceGrowthKind::ReuseRows || target.kind == BalanceTargetKind::Keep)) {
    return plan_balance(batch.y, target, random);
  }
  if (target.growth != BalanceGrowthKind::ReuseRows && target.growth != BalanceGrowthKind::BlendNeighbors) {
    return StatusOr<BalancePlan>(Status::invalid_argument(std::string(where) + " received unsupported BalanceGrowthKind."));
  }
  if (target.growth == BalanceGrowthKind::BlendNeighbors && target.neighbors <= 0) {
    return StatusOr<BalancePlan>(Status::invalid_argument(std::string(where) + " BalanceTarget.neighbors must be positive."));
  }

  StatusOr<Series> rows_or = dense_feature_rows(batch.features, where);
  if (!rows_or.ok()) return StatusOr<BalancePlan>(rows_or.status());
  const Series feature_rows = rows_or.move_value();

  StatusOr<ClassMix> original_before_or = class_mix(batch.y);
  if (!original_before_or.ok()) return StatusOr<BalancePlan>(original_before_or.status());
  ClassMix original_before = original_before_or.move_value();

  const Platform platform = original_before.labels.platform();
  const std::int64_t n = original_before.sample_count;
  const Series y = batch.y.to(platform, ScalarType::Float64).reshape({n});

  StatusOr<Series> clean_keep_or = cleaned_keep_indices(feature_rows, y, target, original_before, where);
  if (!clean_keep_or.ok()) return StatusOr<BalancePlan>(clean_keep_or.status());
  Series base_indices = clean_keep_or.move_value().to(platform, ScalarType::Int64);
  base_indices = base_indices.reshape({base_indices.numel()});
  const Series base_labels = y.index_select(0, base_indices).reshape({base_indices.numel()});

  StatusOr<ClassMix> before_or = class_mix(base_labels);
  if (!before_or.ok()) return StatusOr<BalancePlan>(before_or.status());
  ClassMix before = before_or.move_value();
  const std::int64_t k = before.class_count;

  StatusOr<Series> target_counts_or = resolve_target_counts(before, target, where);
  if (!target_counts_or.ok()) return StatusOr<BalancePlan>(target_counts_or.status());
  const Series target_counts = target_counts_or.move_value().to(platform, ScalarType::Float64).reshape({k});

  const std::vector<double> desired = target_counts.to(pluss::pCPU, ScalarType::Float64).reshape({k}).host<double>();
  const std::uint64_t seed = compose_seed(random);

  Series retained = pluss::series::undefined();
  Series synth_base = pluss::series::undefined();
  Series synth_neighbor = pluss::series::undefined();
  Series synth_steps = pluss::series::undefined();
  Series synth_labels = pluss::series::undefined();
  bool uses_replacement = false;

  for (std::int64_t ci = 0; ci < k; ++ci) {
    StatusOr<Series> class_idx_or = select_class_indices_from_base(base_indices, base_labels, before.labels, ci);
    if (!class_idx_or.ok()) return StatusOr<BalancePlan>(class_idx_or.status());
    Series class_indices = class_idx_or.move_value();
    class_indices = class_indices.reshape({class_indices.numel()});
    const std::int64_t have = class_indices.numel();
    const std::int64_t want = static_cast<std::int64_t>(std::llround(desired[static_cast<std::size_t>(ci)]));
    if (want <= 0) {
      return StatusOr<BalancePlan>(Status::invalid_argument(std::string(where) + " target counts must be positive."));
    }

    const std::uint64_t class_seed = splitmix64(seed + static_cast<std::uint64_t>(ci) * 0x9e3779b97f4a7c15ULL);
    if (want < have) {
      const Series perm = pluss::series::randperm(have, i64_on(platform), class_seed).reshape({have});
      const Series take = perm.index_select(0, pluss::series::arange(0, want, i64_on(platform))).reshape({want});
      retained = append_indices(std::move(retained), class_indices.index_select(0, take).reshape({want}));
      continue;
    }

    retained = append_indices(std::move(retained), class_indices.reshape({have}));
    const std::int64_t extra = want - have;
    if (extra <= 0) continue;

    if (target.growth == BalanceGrowthKind::ReuseRows || have < 2) {
      const Series extra_pos = pluss::series::randint(0, have, {extra}, i64_on(platform), class_seed).reshape({extra});
      retained = append_indices(std::move(retained), class_indices.index_select(0, extra_pos).reshape({extra}));
      uses_replacement = true;
      continue;
    }

    const std::int64_t neighbor_count = std::max<std::int64_t>(1, std::min<std::int64_t>(target.neighbors, have - 1));
    const Series class_x = feature_rows.index_select(0, class_indices.to(feature_rows.platform(), ScalarType::Int64))
                               .reshape({have, batch.features.feature_count()});

    // SpatialIndex (a real KD-tree) replaces the dense pairwise_distance2 +
    // argsort self-search on CPU, where it's an exact drop-in and typically
    // faster for anything but tiny classes -- it's a host-memory recursive
    // structure, so GPU keeps the existing Series/matmul path unchanged.
    // Bonus: query_self_k excludes each point's own index directly, rather
    // than assuming column 0 of the sorted distance matrix is self (which
    // isn't guaranteed when duplicate points tie at distance 0).
    Series neighbor_pos = pluss::series::undefined();
    if (class_x.platform() == pluss::pCPU) {
      StatusOr<SpatialIndex> index_or = SpatialIndex::build(class_x);
      if (!index_or.ok()) return StatusOr<BalancePlan>(index_or.status());
      const SpatialIndex index = index_or.move_value();
      StatusOr<KNearestResult> knn_or = index.query_self_k(neighbor_count);
      if (!knn_or.ok()) return StatusOr<BalancePlan>(knn_or.status());
      neighbor_pos = knn_or.move_value().indices;
    } else {
      const Series dist2 = pairwise_distance2(class_x, class_x);
      const Series sort_idx = dist2.argsort(1, false, true);
      const Series neighbor_cols = pluss::series::arange(1, neighbor_count + 1, i64_on(sort_idx.platform()));
      neighbor_pos = sort_idx.index_select(1, neighbor_cols).reshape({have, neighbor_count});
    }

    const Series base_pos = pluss::series::randint(0, have, {extra}, i64_on(platform), splitmix64(class_seed ^ 0xfeed1234ULL))
                                .reshape({extra});
    const Series neighbor_choice =
        pluss::series::randint(0, neighbor_count, {extra}, i64_on(platform), splitmix64(class_seed ^ 0xa11ce5eedULL))
            .reshape({extra, 1});
    const Series neighbor_for_base = neighbor_pos.index_select(0, base_pos.to(neighbor_pos.platform(), ScalarType::Int64))
                                         .gather(1, neighbor_choice.to(neighbor_pos.platform(), ScalarType::Int64))
                                         .reshape({extra});
    const Series base_indices = class_indices.index_select(0, base_pos.to(platform, ScalarType::Int64)).reshape({extra});
    const Series neighbor_indices = class_indices.index_select(0, neighbor_for_base.to(platform, ScalarType::Int64))
                                        .reshape({extra});
    const Series steps = pluss::series::rand({extra}, f64_on(platform), splitmix64(class_seed ^ 0x5eededULL)).reshape({extra});
    const Series label_pos = pluss::series::arange(ci, ci + 1, i64_on(platform));
    const double label_value = before.labels.index_select(0, label_pos).reshape({1}).host<double>().front();
    const Series labels_extra = pluss::series::ones({extra}, f64_on(platform)).mul(label_value).reshape({extra});

    synth_base = append_indices(std::move(synth_base), base_indices);
    synth_neighbor = append_indices(std::move(synth_neighbor), neighbor_indices);
    synth_steps = append_f64(std::move(synth_steps), steps);
    synth_labels = append_f64(std::move(synth_labels), labels_extra);
  }

  if (!retained.defined() || retained.numel() <= 0) {
    return StatusOr<BalancePlan>(Status::internal(std::string(where) + " produced no retained row indices."));
  }

  Series after_labels = y.index_select(0, retained.to(platform, ScalarType::Int64)).reshape({retained.numel()});
  if (synth_labels.defined() && synth_labels.numel() > 0) {
    after_labels = after_labels.unite({synth_labels.to(platform, ScalarType::Float64).reshape({synth_labels.numel()})}, 0);
  }
  StatusOr<ClassMix> after_or = class_mix(after_labels);
  if (!after_or.ok()) return StatusOr<BalancePlan>(after_or.status());

  BalancePlan out{};
  out.indices = retained.to(platform, ScalarType::Int64).reshape({retained.numel()});
  out.target_counts = target_counts.reshape({k});
  out.synthetic_base_indices = synth_base.defined() ? synth_base.to(platform, ScalarType::Int64).reshape({synth_base.numel()}) : pluss::series::undefined();
  out.synthetic_neighbor_indices =
      synth_neighbor.defined() ? synth_neighbor.to(platform, ScalarType::Int64).reshape({synth_neighbor.numel()}) : pluss::series::undefined();
  out.synthetic_steps = synth_steps.defined() ? synth_steps.to(platform, ScalarType::Float64).reshape({synth_steps.numel()}) : pluss::series::undefined();
  out.synthetic_labels = synth_labels.defined() ? synth_labels.to(platform, ScalarType::Float64).reshape({synth_labels.numel()}) : pluss::series::undefined();
  if (target.clean != BalanceCleanKind::None) {
    const Series all_indices = pluss::series::arange(0, n, i64_on(platform));
    const Series kept_mask = all_indices.reshape({n, 1}).eq(base_indices.reshape({1, base_indices.numel()})).any(1, false);
    Series removed = all_indices.masked_select(kept_mask.logical_not()).to(platform, ScalarType::Int64);
    out.removed_indices = removed.reshape({removed.numel()});
  }
  out.before = std::move(original_before);
  out.after = after_or.move_value();
  out.uses_replacement = uses_replacement;
  out.uses_synthesis = out.synthetic_base_indices.defined() && out.synthetic_base_indices.numel() > 0;
  out.uses_cleaning = target.clean != BalanceCleanKind::None;
  return StatusOr<BalancePlan>(std::move(out));
}

StatusOr<LabeledBatch> balance_rows(const LabeledBatch& batch, const BalanceTarget& target, RandomState random) {
  Status check = validate_labeled(batch);
  if (!check.ok()) return StatusOr<LabeledBatch>(std::move(check));
  StatusOr<BalancePlan> plan_or = plan_balance(batch, target, random);
  if (!plan_or.ok()) return StatusOr<LabeledBatch>(plan_or.status());
  const BalancePlan& plan = plan_or.value();
  StatusOr<LabeledBatch> base_or = slice_labeled_batch(batch, plan.indices);
  if (!base_or.ok()) return StatusOr<LabeledBatch>(base_or.status());
  LabeledBatch out = base_or.move_value();

  if (!plan.uses_synthesis) return StatusOr<LabeledBatch>(std::move(out));
  if (!plan.synthetic_labels.defined()
      || !plan.synthetic_base_indices.defined()
      || !plan.synthetic_neighbor_indices.defined()
      || !plan.synthetic_steps.defined()) {
    return StatusOr<LabeledBatch>(Status::internal("balance_rows synthetic balance plan is incomplete."));
  }

  const std::int64_t synth_count = plan.synthetic_labels.numel();
  StatusOr<Series> rows_or = dense_feature_rows(batch.features, "balance_rows");
  if (!rows_or.ok()) return StatusOr<LabeledBatch>(rows_or.status());
  const Series feature_rows = rows_or.move_value();
  const std::int64_t p = batch.features.feature_count();
  const Series base_idx = plan.synthetic_base_indices.to(feature_rows.platform(), ScalarType::Int64).reshape({synth_count});
  const Series neighbor_idx = plan.synthetic_neighbor_indices.to(feature_rows.platform(), ScalarType::Int64).reshape({synth_count});
  const Series steps = plan.synthetic_steps.to(feature_rows.platform(), ScalarType::Float64).reshape({synth_count, 1});
  const Series base_x = feature_rows.index_select(0, base_idx).reshape({synth_count, p});
  const Series neighbor_x = feature_rows.index_select(0, neighbor_idx).reshape({synth_count, p});
  const Series synth_x_rows = base_x.add(neighbor_x.sub(base_x).mul(steps)).reshape({synth_count, p});

  if (batch.features.layout == ObservationLayout::Rows) {
    out.features.x = out.features.x.to(synth_x_rows.platform(), ScalarType::Float64).unite({synth_x_rows}, 0);
  } else {
    out.features.x = out.features.x.to(synth_x_rows.platform(), ScalarType::Float64).unite({synth_x_rows.transpose(0, 1)}, 1);
  }
  out.y = out.y.to(plan.synthetic_labels.platform(), plan.synthetic_labels.scalar_type())
              .reshape({out.y.numel()})
              .unite({plan.synthetic_labels.reshape({synth_count})}, 0)
              .to(batch.y.scalar_type());

  if (batch.sample_weights.defined()) {
    const Series all_w = batch.sample_weights.to(feature_rows.platform(), ScalarType::Float64).reshape({batch.sample_weights.numel()});
    const Series wb = all_w.index_select(0, base_idx).reshape({synth_count});
    const Series wn = all_w.index_select(0, neighbor_idx).reshape({synth_count});
    const Series synth_w = wb.add(wn).mul(0.5).reshape({synth_count});
    out.sample_weights = out.sample_weights.to(feature_rows.platform(), ScalarType::Float64)
                             .reshape({out.sample_weights.numel()})
                             .unite({synth_w}, 0);
  }

  Status out_check = validate_labeled(out);
  if (!out_check.ok()) return StatusOr<LabeledBatch>(std::move(out_check));
  return StatusOr<LabeledBatch>(std::move(out));
}

} // namespace pluss::ai::nodej
