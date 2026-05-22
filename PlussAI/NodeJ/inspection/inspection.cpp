#include "pluss/ai/nodej/inspection.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
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
  out.x = rows.to_vec<double>();
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
  const double value = flat.item<double>();
  if (!std::isfinite(value)) {
    return StatusOr<double>(Status::numerical_failure(std::string(where) + " produced non-finite metric value."));
  }
  return StatusOr<double>(value);
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
  DenseRows dense_eval = dense_eval_or.move_value();

  const AssessOptions assess_opts{
      .metrics = {metric},
  };

  double baseline = std::numeric_limits<double>::quiet_NaN();
  if (spec.task == TaskKind::Classification) {
    StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_classifier(spec);
    if (!model_or.ok()) return StatusOr<PermuteImpactReport>(model_or.status());
    std::unique_ptr<ClassifierBase> model = model_or.move_value();
    Status fit = model->learn(train);
    if (!fit.ok()) return StatusOr<PermuteImpactReport>(std::move(fit));
    StatusOr<Series> metric_or = model->assess(eval, assess_opts);
    if (!metric_or.ok()) return StatusOr<PermuteImpactReport>(metric_or.status());
    StatusOr<double> base_or = single_metric(metric_or.value(), "permutation_impact baseline");
    if (!base_or.ok()) return StatusOr<PermuteImpactReport>(base_or.status());
    baseline = base_or.move_value();

    const std::uint64_t seed = random.deterministic ? random.seed : splitmix64(random.seed ^ 0x8f9f0bdeULL);
    std::mt19937_64 rng(seed);
    std::vector<double> raw(static_cast<std::size_t>(repeats * dense_eval.features), 0.0);
    for (std::int64_t r = 0; r < repeats; ++r) {
      for (std::int64_t f = 0; f < dense_eval.features; ++f) {
        std::vector<double> permuted = dense_eval.x;
        std::vector<std::int64_t> order(static_cast<std::size_t>(dense_eval.samples), 0);
        std::iota(order.begin(), order.end(), static_cast<std::int64_t>(0));
        std::shuffle(order.begin(), order.end(), rng);
        for (std::int64_t i = 0; i < dense_eval.samples; ++i) {
          permuted[static_cast<std::size_t>(i * dense_eval.features + f)] =
              dense_eval.x[static_cast<std::size_t>(order[static_cast<std::size_t>(i)] * dense_eval.features + f)];
        }
        const LabeledBatch probe{
            .features = rows_to_batch(permuted, dense_eval.samples, dense_eval.features, eval.features.x.platform()),
            .y = eval.y,
            .sample_weights = eval.sample_weights,
        };
        StatusOr<Series> m_or = model->assess(probe, assess_opts);
        if (!m_or.ok()) return StatusOr<PermuteImpactReport>(m_or.status());
        StatusOr<double> v_or = single_metric(m_or.value(), "permutation_impact classification repeat");
        if (!v_or.ok()) return StatusOr<PermuteImpactReport>(v_or.status());
        raw[static_cast<std::size_t>(r * dense_eval.features + f)] = baseline - v_or.move_value();
      }
    }

    std::vector<double> mean(static_cast<std::size_t>(dense_eval.features), 0.0);
    std::vector<double> stdev(static_cast<std::size_t>(dense_eval.features), 0.0);
    for (std::int64_t f = 0; f < dense_eval.features; ++f) {
      for (std::int64_t r = 0; r < repeats; ++r) {
        mean[static_cast<std::size_t>(f)] += raw[static_cast<std::size_t>(r * dense_eval.features + f)];
      }
      mean[static_cast<std::size_t>(f)] /= static_cast<double>(repeats);
      for (std::int64_t r = 0; r < repeats; ++r) {
        const double d = raw[static_cast<std::size_t>(r * dense_eval.features + f)] - mean[static_cast<std::size_t>(f)];
        stdev[static_cast<std::size_t>(f)] += d * d;
      }
      stdev[static_cast<std::size_t>(f)] = std::sqrt(stdev[static_cast<std::size_t>(f)] / static_cast<double>(repeats));
    }

    return StatusOr<PermuteImpactReport>(PermuteImpactReport{
        .mean_drop = pluss::make_series(mean, {dense_eval.features}, f64_cpu()),
        .std_drop = pluss::make_series(stdev, {dense_eval.features}, f64_cpu()),
        .per_repeat_drops = pluss::make_series(raw, {repeats, dense_eval.features}, f64_cpu()),
        .metric = metric,
    });
  }

  StatusOr<std::unique_ptr<RegressorBase>> model_or = make_regressor(spec);
  if (!model_or.ok()) return StatusOr<PermuteImpactReport>(model_or.status());
  std::unique_ptr<RegressorBase> model = model_or.move_value();
  Status fit = model->learn(train);
  if (!fit.ok()) return StatusOr<PermuteImpactReport>(std::move(fit));
  StatusOr<Series> metric_or = model->assess(eval, assess_opts);
  if (!metric_or.ok()) return StatusOr<PermuteImpactReport>(metric_or.status());
  StatusOr<double> base_or = single_metric(metric_or.value(), "permutation_impact baseline");
  if (!base_or.ok()) return StatusOr<PermuteImpactReport>(base_or.status());
  baseline = base_or.move_value();

  const std::uint64_t seed = random.deterministic ? random.seed : splitmix64(random.seed ^ 0x8f9f0bdeULL);
  std::mt19937_64 rng(seed);
  std::vector<double> raw(static_cast<std::size_t>(repeats * dense_eval.features), 0.0);
  for (std::int64_t r = 0; r < repeats; ++r) {
    for (std::int64_t f = 0; f < dense_eval.features; ++f) {
      std::vector<double> permuted = dense_eval.x;
      std::vector<std::int64_t> order(static_cast<std::size_t>(dense_eval.samples), 0);
      std::iota(order.begin(), order.end(), static_cast<std::int64_t>(0));
      std::shuffle(order.begin(), order.end(), rng);
      for (std::int64_t i = 0; i < dense_eval.samples; ++i) {
        permuted[static_cast<std::size_t>(i * dense_eval.features + f)] =
            dense_eval.x[static_cast<std::size_t>(order[static_cast<std::size_t>(i)] * dense_eval.features + f)];
      }
      const LabeledBatch probe{
          .features = rows_to_batch(permuted, dense_eval.samples, dense_eval.features, eval.features.x.platform()),
          .y = eval.y,
          .sample_weights = eval.sample_weights,
      };
      StatusOr<Series> m_or = model->assess(probe, assess_opts);
      if (!m_or.ok()) return StatusOr<PermuteImpactReport>(m_or.status());
      StatusOr<double> v_or = single_metric(m_or.value(), "permutation_impact regression repeat");
      if (!v_or.ok()) return StatusOr<PermuteImpactReport>(v_or.status());
      raw[static_cast<std::size_t>(r * dense_eval.features + f)] = v_or.move_value() - baseline;
    }
  }

  std::vector<double> mean(static_cast<std::size_t>(dense_eval.features), 0.0);
  std::vector<double> stdev(static_cast<std::size_t>(dense_eval.features), 0.0);
  for (std::int64_t f = 0; f < dense_eval.features; ++f) {
    for (std::int64_t r = 0; r < repeats; ++r) {
      mean[static_cast<std::size_t>(f)] += raw[static_cast<std::size_t>(r * dense_eval.features + f)];
    }
    mean[static_cast<std::size_t>(f)] /= static_cast<double>(repeats);
    for (std::int64_t r = 0; r < repeats; ++r) {
      const double d = raw[static_cast<std::size_t>(r * dense_eval.features + f)] - mean[static_cast<std::size_t>(f)];
      stdev[static_cast<std::size_t>(f)] += d * d;
    }
    stdev[static_cast<std::size_t>(f)] = std::sqrt(stdev[static_cast<std::size_t>(f)] / static_cast<double>(repeats));
  }

  return StatusOr<PermuteImpactReport>(PermuteImpactReport{
      .mean_drop = pluss::make_series(mean, {dense_eval.features}, f64_cpu()),
      .std_drop = pluss::make_series(stdev, {dense_eval.features}, f64_cpu()),
      .per_repeat_drops = pluss::make_series(raw, {repeats, dense_eval.features}, f64_cpu()),
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
