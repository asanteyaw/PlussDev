#include "pluss/ai/nodej/feature_select.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

[[nodiscard]] Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

[[nodiscard]] Config i64_cpu() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

struct DenseRows {
  std::vector<double> x{};
  std::int64_t samples{0};
  std::int64_t features{0};
  ObservationLayout source_layout{ObservationLayout::Rows};
  Platform source_platform{pluss::pCPU};
};

[[nodiscard]] StatusOr<DenseRows> to_dense_rows_cpu(const FeatureBatch& batch, const char* where) {
  Status check = validate_features(batch);
  if (!check.ok()) return StatusOr<DenseRows>(std::move(check));
  if (batch.x.dim() != 2) {
    return StatusOr<DenseRows>(Status::shape_mismatch(std::string(where) + " requires rank-2 features."));
  }

  DenseRows out{};
  out.source_layout = batch.layout;
  out.source_platform = batch.x.platform();
  out.samples = batch.sample_count();
  out.features = batch.feature_count();

  Series rows = batch.x.to(pluss::pCPU, ScalarType::Float64).contiguous();
  if (batch.layout == ObservationLayout::Columns) {
    rows = rows.transpose(0, 1).contiguous();
  }
  out.x = rows.to_vec<double>();
  if (static_cast<std::int64_t>(out.x.size()) != out.samples * out.features) {
    return StatusOr<DenseRows>(Status::shape_mismatch(std::string(where) + " encountered inconsistent feature shape."));
  }
  return StatusOr<DenseRows>(std::move(out));
}

[[nodiscard]] StatusOr<std::vector<double>> flat_labels(const LabeledBatch& batch, const char* where) {
  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return StatusOr<std::vector<double>>(std::move(data_status));

  const Series y_flat = batch.y.to(pluss::pCPU, ScalarType::Float64).reshape({batch.y.numel()}).contiguous();
  if (y_flat.isnan().any().item<bool>()) {
    return StatusOr<std::vector<double>>(Status::invalid_argument(std::string(where) + " requires finite labels."));
  }
  return StatusOr<std::vector<double>>(y_flat.to_vec<double>());
}

[[nodiscard]] std::vector<double> normalized_unique(std::vector<double> values) {
  for (double& v : values) {
    if (v == 0.0) v = 0.0;
  }
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

[[nodiscard]] StatusOr<std::vector<double>> class_f_scores(const DenseRows& dense,
                                                           const std::vector<double>& y,
                                                           bool absolute_score,
                                                           const char* where) {
  if (static_cast<std::int64_t>(y.size()) != dense.samples) {
    return StatusOr<std::vector<double>>(Status::shape_mismatch(std::string(where) + " labels length must match sample count."));
  }
  std::vector<double> classes = normalized_unique(y);
  if (classes.size() < 2) {
    return StatusOr<std::vector<double>>(Status::invalid_argument(std::string(where) + " requires at least two classes."));
  }
  std::map<double, std::int64_t> class_to_id{};
  for (std::size_t i = 0; i < classes.size(); ++i) class_to_id[classes[i]] = static_cast<std::int64_t>(i);
  std::vector<std::int64_t> cls(static_cast<std::size_t>(dense.samples), 0);
  for (std::int64_t i = 0; i < dense.samples; ++i) {
    double v = y[static_cast<std::size_t>(i)];
    if (v == 0.0) v = 0.0;
    cls[static_cast<std::size_t>(i)] = class_to_id[v];
  }

  std::vector<double> scores(static_cast<std::size_t>(dense.features), 0.0);
  constexpr double eps = 1e-15;
  for (std::int64_t f = 0; f < dense.features; ++f) {
    double overall_mean = 0.0;
    for (std::int64_t i = 0; i < dense.samples; ++i) {
      overall_mean += dense.x[static_cast<std::size_t>(i * dense.features + f)];
    }
    overall_mean /= static_cast<double>(dense.samples);

    std::vector<double> class_sum(classes.size(), 0.0);
    std::vector<double> class_count(classes.size(), 0.0);
    for (std::int64_t i = 0; i < dense.samples; ++i) {
      const double xv = dense.x[static_cast<std::size_t>(i * dense.features + f)];
      const std::size_t c = static_cast<std::size_t>(cls[static_cast<std::size_t>(i)]);
      class_sum[c] += xv;
      class_count[c] += 1.0;
    }

    std::vector<double> class_mean(classes.size(), 0.0);
    for (std::size_t c = 0; c < classes.size(); ++c) {
      class_mean[c] = class_count[c] > 0.0 ? (class_sum[c] / class_count[c]) : overall_mean;
    }

    double ss_between = 0.0;
    double ss_within = 0.0;
    for (std::size_t c = 0; c < classes.size(); ++c) {
      const double d = class_mean[c] - overall_mean;
      ss_between += class_count[c] * d * d;
    }
    for (std::int64_t i = 0; i < dense.samples; ++i) {
      const std::size_t c = static_cast<std::size_t>(cls[static_cast<std::size_t>(i)]);
      const double d = dense.x[static_cast<std::size_t>(i * dense.features + f)] - class_mean[c];
      ss_within += d * d;
    }

    const double df_between = static_cast<double>(classes.size() - 1);
    const double df_within = static_cast<double>(std::max<std::int64_t>(1, dense.samples - static_cast<std::int64_t>(classes.size())));
    const double ms_between = ss_between / std::max(df_between, 1.0);
    const double ms_within = ss_within / std::max(df_within, 1.0);
    const double score = ms_between / (ms_within + eps);
    scores[static_cast<std::size_t>(f)] = absolute_score ? std::abs(score) : score;
  }
  return StatusOr<std::vector<double>>(std::move(scores));
}

[[nodiscard]] StatusOr<std::vector<double>> regression_f_scores(const DenseRows& dense,
                                                                const std::vector<double>& y,
                                                                bool absolute_score,
                                                                const char* where) {
  if (static_cast<std::int64_t>(y.size()) != dense.samples) {
    return StatusOr<std::vector<double>>(Status::shape_mismatch(std::string(where) + " labels length must match sample count."));
  }
  if (dense.samples < 3) {
    return StatusOr<std::vector<double>>(Status::invalid_argument(std::string(where) + " requires at least 3 samples."));
  }

  double y_mean = 0.0;
  for (double v : y) y_mean += v;
  y_mean /= static_cast<double>(dense.samples);
  double y_ss = 0.0;
  for (double v : y) {
    const double d = v - y_mean;
    y_ss += d * d;
  }
  if (!(y_ss > 0.0)) {
    return StatusOr<std::vector<double>>(Status::invalid_argument(std::string(where) + " requires non-constant targets."));
  }

  std::vector<double> scores(static_cast<std::size_t>(dense.features), 0.0);
  constexpr double eps = 1e-15;
  const double n = static_cast<double>(dense.samples);
  for (std::int64_t f = 0; f < dense.features; ++f) {
    double x_mean = 0.0;
    for (std::int64_t i = 0; i < dense.samples; ++i) {
      x_mean += dense.x[static_cast<std::size_t>(i * dense.features + f)];
    }
    x_mean /= n;

    double x_ss = 0.0;
    double xy = 0.0;
    for (std::int64_t i = 0; i < dense.samples; ++i) {
      const double xv = dense.x[static_cast<std::size_t>(i * dense.features + f)] - x_mean;
      const double yv = y[static_cast<std::size_t>(i)] - y_mean;
      x_ss += xv * xv;
      xy += xv * yv;
    }
    const double r2 = (xy * xy) / ((x_ss + eps) * (y_ss + eps));
    const double score = (r2 / (1.0 - r2 + eps)) * (n - 2.0);
    scores[static_cast<std::size_t>(f)] = absolute_score ? std::abs(score) : score;
  }
  return StatusOr<std::vector<double>>(std::move(scores));
}

[[nodiscard]] StatusOr<std::vector<double>> feature_scores(const DenseRows& dense,
                                                           const std::vector<double>& y,
                                                           FeatureScoreMode mode,
                                                           bool absolute_score,
                                                           const char* where) {
  if (mode == FeatureScoreMode::ClassF) {
    return class_f_scores(dense, y, absolute_score, where);
  }
  if (mode == FeatureScoreMode::RegressionF) {
    return regression_f_scores(dense, y, absolute_score, where);
  }
  return StatusOr<std::vector<double>>(Status::invalid_argument(std::string(where) + " received unknown FeatureScoreMode."));
}

[[nodiscard]] std::vector<std::int64_t> rank_from_scores(const std::vector<double>& scores) {
  std::vector<std::int64_t> ids(scores.size(), 0);
  std::iota(ids.begin(), ids.end(), std::int64_t{0});
  std::stable_sort(ids.begin(), ids.end(), [&](std::int64_t lhs, std::int64_t rhs) {
    const double lv = scores[static_cast<std::size_t>(lhs)];
    const double rv = scores[static_cast<std::size_t>(rhs)];
    if (lv != rv) return lv > rv;
    return lhs < rhs;
  });
  return ids;
}

[[nodiscard]] StatusOr<std::int64_t> pick_count(std::int64_t feature_count, const FeaturePickOptions& options) {
  if (feature_count <= 0) {
    return StatusOr<std::int64_t>(Status::shape_mismatch("pick_features requires at least one feature."));
  }
  if (options.fraction > 0.0) {
    if (!std::isfinite(options.fraction)) {
      return StatusOr<std::int64_t>(Status::invalid_argument("pick_features fraction must be finite."));
    }
    const double frac = options.fraction <= 1.0 ? options.fraction : options.fraction / 100.0;
    if (frac <= 0.0 || frac > 1.0) {
      return StatusOr<std::int64_t>(Status::invalid_argument("pick_features fraction must be in (0, 1] or percent in (0, 100]."));
    }
    return StatusOr<std::int64_t>(std::max<std::int64_t>(1, static_cast<std::int64_t>(std::ceil(frac * static_cast<double>(feature_count)))));
  }
  if (options.top_k <= 0) {
    return StatusOr<std::int64_t>(Status::invalid_argument("pick_features top_k must be > 0 when fraction is not set."));
  }
  return StatusOr<std::int64_t>(std::min<std::int64_t>(options.top_k, feature_count));
}

} // namespace

StatusOr<FeatureBatch> take_features(const FeatureBatch& batch,
                                     pluss::msharp::RangeView::Int feature_indices) {
  if (feature_indices.empty()) {
    return StatusOr<FeatureBatch>(Status::invalid_argument("take_features requires at least one feature index."));
  }
  StatusOr<DenseRows> dense_or = to_dense_rows_cpu(batch, "take_features");
  if (!dense_or.ok()) return StatusOr<FeatureBatch>(dense_or.status());
  DenseRows dense = dense_or.move_value();

  std::vector<double> selected(static_cast<std::size_t>(dense.samples * static_cast<std::int64_t>(feature_indices.size())), 0.0);
  for (std::int64_t i = 0; i < dense.samples; ++i) {
    const std::size_t src_base = static_cast<std::size_t>(i * dense.features);
    const std::size_t dst_base = static_cast<std::size_t>(i * static_cast<std::int64_t>(feature_indices.size()));
    for (std::size_t j = 0; j < feature_indices.size(); ++j) {
      const std::int64_t fid = feature_indices[j];
      if (fid < 0 || fid >= dense.features) {
        return StatusOr<FeatureBatch>(Status::shape_mismatch("take_features feature index out of range."));
      }
      selected[dst_base + j] = dense.x[src_base + static_cast<std::size_t>(fid)];
    }
  }

  Series rows = pluss::make_series(selected,
                                   {dense.samples, static_cast<std::int64_t>(feature_indices.size())},
                                   f64_cpu());
  if (dense.source_layout == ObservationLayout::Columns) {
    rows = rows.transpose(0, 1).contiguous();
  }
  return StatusOr<FeatureBatch>(FeatureBatch{
      .x = rows.to(dense.source_platform, ScalarType::Float64),
      .layout = dense.source_layout,
  });
}

StatusOr<VariancePruneResult> variance_prune(const FeatureBatch& batch, double variance_floor) {
  if (!std::isfinite(variance_floor) || variance_floor < 0.0) {
    return StatusOr<VariancePruneResult>(Status::invalid_argument("variance_prune variance_floor must be finite and >= 0."));
  }
  StatusOr<DenseRows> dense_or = to_dense_rows_cpu(batch, "variance_prune");
  if (!dense_or.ok()) return StatusOr<VariancePruneResult>(dense_or.status());
  DenseRows dense = dense_or.move_value();

  std::vector<double> variances(static_cast<std::size_t>(dense.features), 0.0);
  std::vector<std::int64_t> keep{};
  keep.reserve(static_cast<std::size_t>(dense.features));

  for (std::int64_t f = 0; f < dense.features; ++f) {
    double mean = 0.0;
    for (std::int64_t i = 0; i < dense.samples; ++i) {
      mean += dense.x[static_cast<std::size_t>(i * dense.features + f)];
    }
    mean /= static_cast<double>(dense.samples);

    double var = 0.0;
    for (std::int64_t i = 0; i < dense.samples; ++i) {
      const double d = dense.x[static_cast<std::size_t>(i * dense.features + f)] - mean;
      var += d * d;
    }
    var /= static_cast<double>(dense.samples);
    variances[static_cast<std::size_t>(f)] = var;
    if (var > variance_floor) keep.push_back(f);
  }

  if (keep.empty()) {
    keep.push_back(static_cast<std::int64_t>(std::max_element(variances.begin(), variances.end()) - variances.begin()));
  }

  StatusOr<FeatureBatch> reduced_or = take_features(batch, pluss::msharp::RangeView::Int(keep));
  if (!reduced_or.ok()) return StatusOr<VariancePruneResult>(reduced_or.status());

  return StatusOr<VariancePruneResult>(VariancePruneResult{
      .reduced = reduced_or.move_value(),
      .selected_feature_indices = pluss::make_series(keep, {static_cast<std::int64_t>(keep.size())}, i64_cpu()),
      .feature_variances = pluss::make_series(variances, {dense.features}, f64_cpu()),
  });
}

StatusOr<FeatureRankReport> rank_features(const LabeledBatch& batch, const FeatureRankOptions& options) {
  StatusOr<DenseRows> dense_or = to_dense_rows_cpu(batch.features, "rank_features");
  if (!dense_or.ok()) return StatusOr<FeatureRankReport>(dense_or.status());
  DenseRows dense = dense_or.move_value();
  StatusOr<std::vector<double>> y_or = flat_labels(batch, "rank_features");
  if (!y_or.ok()) return StatusOr<FeatureRankReport>(y_or.status());
  StatusOr<std::vector<double>> scores_or =
      feature_scores(dense, y_or.value(), options.mode, options.absolute_score, "rank_features");
  if (!scores_or.ok()) return StatusOr<FeatureRankReport>(scores_or.status());

  std::vector<double> scores = scores_or.move_value();
  std::vector<std::int64_t> ranked = rank_from_scores(scores);
  return StatusOr<FeatureRankReport>(FeatureRankReport{
      .scores = pluss::make_series(std::move(scores), {dense.features}, f64_cpu()),
      .ranked_feature_indices = pluss::make_series(std::move(ranked), {dense.features}, i64_cpu()),
  });
}

StatusOr<FeaturePickResult> pick_features(const LabeledBatch& batch, const FeaturePickOptions& options) {
  StatusOr<DenseRows> dense_or = to_dense_rows_cpu(batch.features, "pick_features");
  if (!dense_or.ok()) return StatusOr<FeaturePickResult>(dense_or.status());
  DenseRows dense = dense_or.move_value();
  StatusOr<std::vector<double>> y_or = flat_labels(batch, "pick_features");
  if (!y_or.ok()) return StatusOr<FeaturePickResult>(y_or.status());
  StatusOr<std::vector<double>> scores_or =
      feature_scores(dense, y_or.value(), options.mode, options.absolute_score, "pick_features");
  if (!scores_or.ok()) return StatusOr<FeaturePickResult>(scores_or.status());
  StatusOr<std::int64_t> count_or = pick_count(dense.features, options);
  if (!count_or.ok()) return StatusOr<FeaturePickResult>(count_or.status());

  std::vector<double> scores = scores_or.move_value();
  std::vector<std::int64_t> ranked = rank_from_scores(scores);
  std::vector<std::int64_t> selected(ranked.begin(),
                                     ranked.begin() + static_cast<std::ptrdiff_t>(count_or.value()));
  StatusOr<FeatureBatch> reduced_or = take_features(batch.features, pluss::msharp::RangeView::Int(selected));
  if (!reduced_or.ok()) return StatusOr<FeaturePickResult>(reduced_or.status());

  return StatusOr<FeaturePickResult>(FeaturePickResult{
      .reduced = reduced_or.move_value(),
      .selected_feature_indices = pluss::make_series(selected, {static_cast<std::int64_t>(selected.size())}, i64_cpu()),
      .scores = pluss::make_series(std::move(scores), {dense.features}, f64_cpu()),
      .ranked_feature_indices = pluss::make_series(std::move(ranked), {dense.features}, i64_cpu()),
  });
}

StatusOr<Series> f_score_rank(const LabeledBatch& batch, const ScoreRankOptions& options) {
  if (options.top_k <= 0) {
    return StatusOr<Series>(Status::invalid_argument("f_score_rank requires ScoreRankOptions.top_k > 0."));
  }
  StatusOr<FeatureRankReport> rank_or =
      rank_features(batch, FeatureRankOptions{.mode = FeatureScoreMode::ClassF, .absolute_score = options.absolute_score});
  if (!rank_or.ok()) return StatusOr<Series>(rank_or.status());
  const std::int64_t out_k = std::min<std::int64_t>(options.top_k, rank_or.value().ranked_feature_indices.numel());
  const Series ids = rank_or.value().ranked_feature_indices.reshape({rank_or.value().ranked_feature_indices.numel()})
                         .index_select(0, pluss::series::arange(0, out_k, i64_cpu()));
  return StatusOr<Series>(ids);
}

StatusOr<Series> f_regression_rank(const LabeledBatch& batch, const ScoreRankOptions& options) {
  if (options.top_k <= 0) {
    return StatusOr<Series>(Status::invalid_argument("f_regression_rank requires ScoreRankOptions.top_k > 0."));
  }
  StatusOr<FeatureRankReport> rank_or =
      rank_features(batch, FeatureRankOptions{.mode = FeatureScoreMode::RegressionF, .absolute_score = options.absolute_score});
  if (!rank_or.ok()) return StatusOr<Series>(rank_or.status());
  const std::int64_t out_k = std::min<std::int64_t>(options.top_k, rank_or.value().ranked_feature_indices.numel());
  const Series ids = rank_or.value().ranked_feature_indices.reshape({rank_or.value().ranked_feature_indices.numel()})
                         .index_select(0, pluss::series::arange(0, out_k, i64_cpu()));
  return StatusOr<Series>(ids);
}

} // namespace pluss::ai::nodej
