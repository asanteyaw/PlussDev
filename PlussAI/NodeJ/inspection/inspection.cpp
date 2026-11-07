#include "pluss/ai/nodej/inspection.h"

#include "pluss/codem/series_factory.h"
#include "pluss/tils/kit/transmit.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

[[nodiscard]] Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

// Matches the compose_seed pattern already used by split.cpp/tune.cpp/
// balance.cpp/prep.cpp/family_classifier.cpp/family_regressor.cpp for
// RandomState::deterministic == false: draw real entropy from the clock and
// std::random_device rather than deterministically hashing the given seed
// (which would make every non-deterministic call reproduce the same draw).
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

struct DenseRows {
  std::vector<double> x{};
  std::int64_t samples{0};
  std::int64_t features{0};
  Platform source_platform{pluss::pCPU};
};

[[nodiscard]] StatusOr<DenseRows> to_dense_rows_cpu(const FeatureBatch& batch, const char* where) {
  Status check = validate_features(batch);
  if (!check.ok()) return StatusOr<DenseRows>(std::move(check));
  if (batch.x.dim() != 2) {
    return StatusOr<DenseRows>(Status::shape_mismatch(std::string(where) + " requires rank-2 features."));
  }

  DenseRows out{};
  out.samples = batch.sample_count();
  out.features = batch.feature_count();
  out.source_platform = batch.x.platform();

  Series rows = batch.x.to(pluss::pCPU, ScalarType::Float64).contiguous();
  if (batch.layout == ObservationLayout::Columns) rows = rows.transpose(0, 1).contiguous();
  out.x = rows.host<double>().array().data;
  if (static_cast<std::int64_t>(out.x.size()) != out.samples * out.features) {
    return StatusOr<DenseRows>(Status::shape_mismatch(std::string(where) + " encountered inconsistent feature shape."));
  }
  return StatusOr<DenseRows>(std::move(out));
}

[[nodiscard]] FeatureBatch rows_to_batch(const std::vector<double>& rows,
                                         std::int64_t sample_count,
                                         std::int64_t feature_count,
                                         Platform platform) {
  return FeatureBatch{
      .x = pluss::make_series(rows, {sample_count, feature_count}, f64_cpu()).to(platform, ScalarType::Float64),
      .layout = ObservationLayout::Rows,
  };
}

[[nodiscard]] StatusOr<double> single_metric(const Series& metrics, const char* where) {
  const Series flat = metrics.to(pluss::pCPU, ScalarType::Float64).reshape({metrics.numel()});
  if (flat.numel() != 1) {
    return StatusOr<double>(Status::shape_mismatch(std::string(where) + " expected one metric value."));
  }
  const double value = flat.host<double>().front();
  if (!std::isfinite(value)) {
    return StatusOr<double>(Status::numerical_failure(std::string(where) + " produced non-finite metric value."));
  }
  return StatusOr<double>(value);
}

} // namespace

namespace {

// One (feature, all repeats) unit of work: copy the eval matrix once for this
// feature, then repeatedly reshuffle-in-place and re-score, matching
// scikit-learn's _calculate_permutation_scores (a single copy per column, one
// evolving shuffle across repeats -- not a fresh copy per repeat).
struct PermuteFeatureResult {
  std::vector<double> drops{};  // one entry per repeat; empty on error
  bool ok = true;
  Status status = Status::success();
};

[[nodiscard]] PermuteFeatureResult permute_one_feature(const EstimatorBase& model,
                                                        const DenseRows& dense_eval,
                                                        const LabeledBatch& eval,
                                                        const AssessOptions& assess_opts,
                                                        double baseline,
                                                        bool prefers_max,
                                                        std::int64_t repeats,
                                                        std::uint64_t seed,
                                                        std::int64_t f) {
  const std::int64_t samples = dense_eval.samples;
  const std::int64_t features = dense_eval.features;
  std::vector<double> permuted = dense_eval.x;  // one full copy for this feature
  std::vector<double> column(static_cast<std::size_t>(samples));
  std::vector<std::int64_t> order(static_cast<std::size_t>(samples));
  std::mt19937_64 rng(seed);  // same seed per feature, mirrors sklearn's per-column reuse

  PermuteFeatureResult out{};
  out.drops.resize(static_cast<std::size_t>(repeats));
  for (std::int64_t r = 0; r < repeats; ++r) {
    // Snapshot the column's current values before overwriting (they may
    // already be shuffled from a previous repeat) -- matches numpy fancy
    // assignment's read-then-write semantics, and keeps this a valid uniform
    // permutation of the original column on every repeat.
    for (std::int64_t i = 0; i < samples; ++i) {
      column[static_cast<std::size_t>(i)] = permuted[static_cast<std::size_t>(i * features + f)];
    }
    std::iota(order.begin(), order.end(), std::int64_t{0});
    std::shuffle(order.begin(), order.end(), rng);
    for (std::int64_t i = 0; i < samples; ++i) {
      permuted[static_cast<std::size_t>(i * features + f)] = column[static_cast<std::size_t>(order[static_cast<std::size_t>(i)])];
    }

    const LabeledBatch probe{
        .features = rows_to_batch(permuted, samples, features, eval.features.x.platform()),
        .y = eval.y,
        .sample_weights = eval.sample_weights,
    };
    StatusOr<Series> m_or = model.assess(probe, assess_opts);
    if (!m_or.ok()) {
      out.ok = false;
      out.status = m_or.status();
      return out;
    }
    StatusOr<double> v_or = single_metric(m_or.value(), "permutation_impact repeat");
    if (!v_or.ok()) {
      out.ok = false;
      out.status = v_or.status();
      return out;
    }
    const double permuted_score = v_or.move_value();
    out.drops[static_cast<std::size_t>(r)] = prefers_max ? (baseline - permuted_score) : (permuted_score - baseline);
  }
  return out;
}

} // namespace

StatusOr<PermuteImpactReport> permutation_impact(const ModelSpec& spec,
                                                 const LabeledBatch& train,
                                                 const LabeledBatch& eval,
                                                 MetricKind metric,
                                                 std::int64_t repeats,
                                                 RandomState random) {
  if (repeats <= 0) {
    return StatusOr<PermuteImpactReport>(Status::invalid_argument("permutation_impact requires repeats > 0."));
  }
  Status train_status = validate_labeled(train);
  if (!train_status.ok()) return StatusOr<PermuteImpactReport>(std::move(train_status));
  Status eval_status = validate_labeled(eval);
  if (!eval_status.ok()) return StatusOr<PermuteImpactReport>(std::move(eval_status));
  if (train.features.feature_count() != eval.features.feature_count()) {
    return StatusOr<PermuteImpactReport>(Status::shape_mismatch("permutation_impact train/eval feature-count mismatch."));
  }

  StatusOr<DenseRows> dense_eval_or = to_dense_rows_cpu(eval.features, "permutation_impact");
  if (!dense_eval_or.ok()) return StatusOr<PermuteImpactReport>(dense_eval_or.status());
  const DenseRows dense_eval = dense_eval_or.move_value();

  const AssessOptions assess_opts{
      .metrics = {metric},
  };

  // One estimator, held by its EstimatorBase interface, so the classification
  // and regression paths below share the same fit-once/probe-many-times loop
  // instead of carrying two copies of it.
  std::unique_ptr<EstimatorBase> model;
  if (spec.task == TaskKind::Classification) {
    StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_classifier(spec);
    if (!model_or.ok()) return StatusOr<PermuteImpactReport>(model_or.status());
    model = model_or.move_value();
  } else {
    StatusOr<std::unique_ptr<RegressorBase>> model_or = make_regressor(spec);
    if (!model_or.ok()) return StatusOr<PermuteImpactReport>(model_or.status());
    model = model_or.move_value();
  }
  Status fit = model->learn(train);
  if (!fit.ok()) return StatusOr<PermuteImpactReport>(std::move(fit));

  StatusOr<Series> baseline_or_series = model->assess(eval, assess_opts);
  if (!baseline_or_series.ok()) return StatusOr<PermuteImpactReport>(baseline_or_series.status());
  StatusOr<double> baseline_or = single_metric(baseline_or_series.value(), "permutation_impact baseline");
  if (!baseline_or.ok()) return StatusOr<PermuteImpactReport>(baseline_or.status());
  const double baseline = baseline_or.move_value();
  const bool prefers_max = metric_prefers_max(metric);
  const std::uint64_t seed = compose_seed(random);

  const std::int64_t features = dense_eval.features;
  std::vector<std::int64_t> feature_indices(static_cast<std::size_t>(features));
  std::iota(feature_indices.begin(), feature_indices.end(), std::int64_t{0});

  // Every feature's work is independent given the one already-fitted model
  // (assess() is const, read-only inference) -- run them across
  // hardware_concurrency() workers instead of one feature at a time.
  const auto compute_feature = [&](std::int64_t f) -> PermuteFeatureResult {
    return permute_one_feature(*model, dense_eval, eval, assess_opts, baseline, prefers_max, repeats, seed, f);
  };
  pluss::tils::transmit_options topts{};
  topts.grain_size = 1;
  topts.exec = features > 1 ? pluss::tils::transmit_exec::par : pluss::tils::transmit_exec::seq;
  const std::vector<PermuteFeatureResult> results = pluss::tils::transmit(compute_feature, feature_indices, topts);

  std::vector<double> raw(static_cast<std::size_t>(repeats * features), 0.0);
  std::vector<double> mean(static_cast<std::size_t>(features), 0.0);
  std::vector<double> stdev(static_cast<std::size_t>(features), 0.0);
  for (std::int64_t f = 0; f < features; ++f) {
    const PermuteFeatureResult& fr = results[static_cast<std::size_t>(f)];
    if (!fr.ok) return StatusOr<PermuteImpactReport>(fr.status);
    double sum = 0.0;
    for (std::int64_t r = 0; r < repeats; ++r) {
      raw[static_cast<std::size_t>(r * features + f)] = fr.drops[static_cast<std::size_t>(r)];
      sum += fr.drops[static_cast<std::size_t>(r)];
    }
    mean[static_cast<std::size_t>(f)] = sum / static_cast<double>(repeats);
    double var = 0.0;
    for (std::int64_t r = 0; r < repeats; ++r) {
      const double d = fr.drops[static_cast<std::size_t>(r)] - mean[static_cast<std::size_t>(f)];
      var += d * d;
    }
    stdev[static_cast<std::size_t>(f)] = std::sqrt(var / static_cast<double>(repeats));
  }

  return StatusOr<PermuteImpactReport>(PermuteImpactReport{
      .mean_drop = pluss::make_series(mean, {features}, f64_cpu()),
      .std_drop = pluss::make_series(stdev, {features}, f64_cpu()),
      .per_repeat_drops = pluss::make_series(raw, {repeats, features}, f64_cpu()),
      .metric = metric,
  });
}

StatusOr<DependenceCurve> dependence_curve(const ModelSpec& spec,
                                           const LabeledBatch& train,
                                           const FeatureBatch& eval,
                                           std::int64_t feature_index,
                                           std::int64_t grid_points) {
  if (grid_points <= 1) {
    return StatusOr<DependenceCurve>(Status::invalid_argument("dependence_curve requires grid_points > 1."));
  }
  Status train_status = validate_labeled(train);
  if (!train_status.ok()) return StatusOr<DependenceCurve>(std::move(train_status));
  Status eval_status = validate_features(eval);
  if (!eval_status.ok()) return StatusOr<DependenceCurve>(std::move(eval_status));

  StatusOr<DenseRows> dense_or = to_dense_rows_cpu(eval, "dependence_curve");
  if (!dense_or.ok()) return StatusOr<DependenceCurve>(dense_or.status());
  DenseRows dense = dense_or.move_value();
  if (feature_index < 0 || feature_index >= dense.features) {
    return StatusOr<DependenceCurve>(Status::shape_mismatch("dependence_curve feature index out of range."));
  }

  double lo = std::numeric_limits<double>::infinity();
  double hi = -std::numeric_limits<double>::infinity();
  for (std::int64_t i = 0; i < dense.samples; ++i) {
    const double v = dense.x[static_cast<std::size_t>(i * dense.features + feature_index)];
    lo = std::min(lo, v);
    hi = std::max(hi, v);
  }
  if (!std::isfinite(lo) || !std::isfinite(hi)) {
    return StatusOr<DependenceCurve>(Status::numerical_failure("dependence_curve encountered non-finite feature values."));
  }

  std::vector<double> grid(static_cast<std::size_t>(grid_points), lo);
  if (hi > lo) {
    const double step = (hi - lo) / static_cast<double>(grid_points - 1);
    for (std::int64_t i = 0; i < grid_points; ++i) {
      grid[static_cast<std::size_t>(i)] = lo + static_cast<double>(i) * step;
    }
  }

  std::vector<double> means(static_cast<std::size_t>(grid_points), 0.0);
  if (spec.task == TaskKind::Classification) {
    StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_classifier(spec);
    if (!model_or.ok()) return StatusOr<DependenceCurve>(model_or.status());
    std::unique_ptr<ClassifierBase> model = model_or.move_value();
    Status fit = model->learn(train);
    if (!fit.ok()) return StatusOr<DependenceCurve>(std::move(fit));

    for (std::int64_t gi = 0; gi < grid_points; ++gi) {
      std::vector<double> probe = dense.x;
      const double v = grid[static_cast<std::size_t>(gi)];
      for (std::int64_t i = 0; i < dense.samples; ++i) {
        probe[static_cast<std::size_t>(i * dense.features + feature_index)] = v;
      }
      const FeatureBatch fb = rows_to_batch(probe, dense.samples, dense.features, eval.x.platform());
      StatusOr<Series> scores_or = model->infer_scores(fb);
      if (!scores_or.ok()) return StatusOr<DependenceCurve>(scores_or.status());
      if (scores_or.value().numel() != dense.samples) {
        return StatusOr<DependenceCurve>(Status::invalid_argument(
            "dependence_curve requires a classifier that produces exactly one score per row "
            "(binary classification); this ModelSpec's infer_scores produced multiple columns "
            "per row, which has no single well-defined mean prediction."));
      }
      const Series probs = scores_or.value().sigmoid().to(pluss::pCPU, ScalarType::Float64).reshape({dense.samples});
      means[static_cast<std::size_t>(gi)] = probs.mean().item<double>();
    }
  } else {
    StatusOr<std::unique_ptr<RegressorBase>> model_or = make_regressor(spec);
    if (!model_or.ok()) return StatusOr<DependenceCurve>(model_or.status());
    std::unique_ptr<RegressorBase> model = model_or.move_value();
    Status fit = model->learn(train);
    if (!fit.ok()) return StatusOr<DependenceCurve>(std::move(fit));

    for (std::int64_t gi = 0; gi < grid_points; ++gi) {
      std::vector<double> probe = dense.x;
      const double v = grid[static_cast<std::size_t>(gi)];
      for (std::int64_t i = 0; i < dense.samples; ++i) {
        probe[static_cast<std::size_t>(i * dense.features + feature_index)] = v;
      }
      const FeatureBatch fb = rows_to_batch(probe, dense.samples, dense.features, eval.x.platform());
      StatusOr<Series> pred_or = model->infer(fb);
      if (!pred_or.ok()) return StatusOr<DependenceCurve>(pred_or.status());
      const Series pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).reshape({dense.samples});
      means[static_cast<std::size_t>(gi)] = pred.mean().item<double>();
    }
  }

  return StatusOr<DependenceCurve>(DependenceCurve{
      .feature_index = feature_index,
      .grid = pluss::make_series(grid, {grid_points}, f64_cpu()),
      .mean_prediction = pluss::make_series(means, {grid_points}, f64_cpu()),
  });
}

StatusOr<DependenceSurface> dependence_surface(const ModelSpec& spec,
                                               const LabeledBatch& train,
                                               const FeatureBatch& eval,
                                               std::int64_t feature_a,
                                               std::int64_t feature_b,
                                               std::int64_t grid_points_a,
                                               std::int64_t grid_points_b) {
  if (grid_points_a <= 1 || grid_points_b <= 1) {
    return StatusOr<DependenceSurface>(Status::invalid_argument("dependence_surface requires grid points > 1."));
  }
  if (feature_a == feature_b) {
    return StatusOr<DependenceSurface>(Status::invalid_argument("dependence_surface requires distinct feature indices."));
  }
  Status train_status = validate_labeled(train);
  if (!train_status.ok()) return StatusOr<DependenceSurface>(std::move(train_status));
  Status eval_status = validate_features(eval);
  if (!eval_status.ok()) return StatusOr<DependenceSurface>(std::move(eval_status));

  StatusOr<DenseRows> dense_or = to_dense_rows_cpu(eval, "dependence_surface");
  if (!dense_or.ok()) return StatusOr<DependenceSurface>(dense_or.status());
  DenseRows dense = dense_or.move_value();
  if (feature_a < 0 || feature_a >= dense.features || feature_b < 0 || feature_b >= dense.features) {
    return StatusOr<DependenceSurface>(Status::shape_mismatch("dependence_surface feature index out of range."));
  }

  auto make_grid = [&](std::int64_t feature_index, std::int64_t points) -> std::vector<double> {
    double lo = std::numeric_limits<double>::infinity();
    double hi = -std::numeric_limits<double>::infinity();
    for (std::int64_t i = 0; i < dense.samples; ++i) {
      const double v = dense.x[static_cast<std::size_t>(i * dense.features + feature_index)];
      lo = std::min(lo, v);
      hi = std::max(hi, v);
    }
    std::vector<double> grid(static_cast<std::size_t>(points), lo);
    if (hi > lo) {
      const double step = (hi - lo) / static_cast<double>(points - 1);
      for (std::int64_t i = 0; i < points; ++i) grid[static_cast<std::size_t>(i)] = lo + static_cast<double>(i) * step;
    }
    return grid;
  };

  const std::vector<double> grid_a = make_grid(feature_a, grid_points_a);
  const std::vector<double> grid_b = make_grid(feature_b, grid_points_b);
  std::vector<double> means(static_cast<std::size_t>(grid_points_a * grid_points_b), 0.0);

  // Fit once, then sweep.
  if (spec.task == TaskKind::Classification) {
    StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_classifier(spec);
    if (!model_or.ok()) return StatusOr<DependenceSurface>(model_or.status());
    std::unique_ptr<ClassifierBase> model = model_or.move_value();
    Status fit = model->learn(train);
    if (!fit.ok()) return StatusOr<DependenceSurface>(std::move(fit));
    for (std::int64_t ia = 0; ia < grid_points_a; ++ia) {
      for (std::int64_t ib = 0; ib < grid_points_b; ++ib) {
        std::vector<double> probe = dense.x;
        const double va = grid_a[static_cast<std::size_t>(ia)];
        const double vb = grid_b[static_cast<std::size_t>(ib)];
        for (std::int64_t i = 0; i < dense.samples; ++i) {
          probe[static_cast<std::size_t>(i * dense.features + feature_a)] = va;
          probe[static_cast<std::size_t>(i * dense.features + feature_b)] = vb;
        }
        const FeatureBatch fb = rows_to_batch(probe, dense.samples, dense.features, eval.x.platform());
        StatusOr<Series> scores_or = model->infer_scores(fb);
        if (!scores_or.ok()) return StatusOr<DependenceSurface>(scores_or.status());
        if (scores_or.value().numel() != dense.samples) {
          return StatusOr<DependenceSurface>(Status::invalid_argument(
              "dependence_surface requires a classifier that produces exactly one score per row "
              "(binary classification); this ModelSpec's infer_scores produced multiple columns "
              "per row, which has no single well-defined mean prediction."));
        }
        means[static_cast<std::size_t>(ia * grid_points_b + ib)] =
            scores_or.value().sigmoid().to(pluss::pCPU, ScalarType::Float64).mean().item<double>();
      }
    }
  } else {
    StatusOr<std::unique_ptr<RegressorBase>> model_or = make_regressor(spec);
    if (!model_or.ok()) return StatusOr<DependenceSurface>(model_or.status());
    std::unique_ptr<RegressorBase> model = model_or.move_value();
    Status fit = model->learn(train);
    if (!fit.ok()) return StatusOr<DependenceSurface>(std::move(fit));
    for (std::int64_t ia = 0; ia < grid_points_a; ++ia) {
      for (std::int64_t ib = 0; ib < grid_points_b; ++ib) {
        std::vector<double> probe = dense.x;
        const double va = grid_a[static_cast<std::size_t>(ia)];
        const double vb = grid_b[static_cast<std::size_t>(ib)];
        for (std::int64_t i = 0; i < dense.samples; ++i) {
          probe[static_cast<std::size_t>(i * dense.features + feature_a)] = va;
          probe[static_cast<std::size_t>(i * dense.features + feature_b)] = vb;
        }
        const FeatureBatch fb = rows_to_batch(probe, dense.samples, dense.features, eval.x.platform());
        StatusOr<Series> pred_or = model->infer(fb);
        if (!pred_or.ok()) return StatusOr<DependenceSurface>(pred_or.status());
        means[static_cast<std::size_t>(ia * grid_points_b + ib)] =
            pred_or.value().to(pluss::pCPU, ScalarType::Float64).mean().item<double>();
      }
    }
  }

  return StatusOr<DependenceSurface>(DependenceSurface{
      .feature_a = feature_a,
      .feature_b = feature_b,
      .grid_a = pluss::make_series(grid_a, {grid_points_a}, f64_cpu()),
      .grid_b = pluss::make_series(grid_b, {grid_points_b}, f64_cpu()),
      .mean_prediction = pluss::make_series(means, {grid_points_a, grid_points_b}, f64_cpu()),
  });
}

} // namespace pluss::ai::nodej
