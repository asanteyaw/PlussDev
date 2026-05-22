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

StatusOr<Series> f_score_rank(const LabeledBatch& batch, const ScoreRankOptions& options) {
  if (options.top_k <= 0) {
    return StatusOr<Series>(Status::invalid_argument("f_score_rank requires ScoreRankOptions.top_k > 0."));
  }
  StatusOr<DenseRows> dense_or = to_dense_rows_cpu(batch.features, "f_score_rank");
  if (!dense_or.ok()) return StatusOr<Series>(dense_or.status());
  DenseRows dense = dense_or.move_value();
  StatusOr<std::vector<double>> y_or = flat_labels(batch, "f_score_rank");
  if (!y_or.ok()) return StatusOr<Series>(y_or.status());
  const std::vector<double> y = y_or.move_value();
  std::vector<double> classes = normalized_unique(y);
  if (classes.size() < 2) {
    return StatusOr<Series>(Status::invalid_argument("f_score_rank requires at least two classes."));
  }
  std::map<double, std::int64_t> class_to_id{};
  for (std::size_t i = 0; i < classes.size(); ++i) class_to_id[classes[i]] = static_cast<std::int64_t>(i);
  std::vector<std::int64_t> cls(static_cast<std::size_t>(dense.samples), 0);
  for (std::int64_t i = 0; i < dense.samples; ++i) {
    double v = y[static_cast<std::size_t>(i)];
    if (v == 0.0) v = 0.0;
    cls[static_cast<std::size_t>(i)] = class_to_id[v];
  }

  std::vector<std::pair<double, std::int64_t>> scores{};
  scores.reserve(static_cast<std::size_t>(dense.features));
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
    scores.emplace_back(options.absolute_score ? std::abs(score) : score, f);
  }

  std::partial_sort(scores.begin(),
                    scores.begin() + static_cast<std::ptrdiff_t>(std::min<std::int64_t>(options.top_k, dense.features)),
                    scores.end(),
                    [](const auto& a, const auto& b) {
                      if (a.first != b.first) return a.first > b.first;
                      return a.second < b.second;
                    });

  const std::int64_t out_k = std::min<std::int64_t>(options.top_k, dense.features);
  std::vector<std::int64_t> ids(static_cast<std::size_t>(out_k), 0);
  for (std::int64_t i = 0; i < out_k; ++i) {
    ids[static_cast<std::size_t>(i)] = scores[static_cast<std::size_t>(i)].second;
  }
  return StatusOr<Series>(pluss::make_series(ids, {out_k}, i64_cpu()));
}

StatusOr<Series> f_regression_rank(const LabeledBatch& batch, const ScoreRankOptions& options) {
  if (options.top_k <= 0) {
    return StatusOr<Series>(Status::invalid_argument("f_regression_rank requires ScoreRankOptions.top_k > 0."));
  }
  StatusOr<DenseRows> dense_or = to_dense_rows_cpu(batch.features, "f_regression_rank");
  if (!dense_or.ok()) return StatusOr<Series>(dense_or.status());
  DenseRows dense = dense_or.move_value();
  StatusOr<std::vector<double>> y_or = flat_labels(batch, "f_regression_rank");
  if (!y_or.ok()) return StatusOr<Series>(y_or.status());
  const std::vector<double> y = y_or.move_value();

  if (static_cast<std::int64_t>(y.size()) != dense.samples) {
    return StatusOr<Series>(Status::shape_mismatch("f_regression_rank labels length must match sample count."));
  }
  if (dense.samples < 3) {
    return StatusOr<Series>(Status::invalid_argument("f_regression_rank requires at least 3 samples."));
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
    return StatusOr<Series>(Status::invalid_argument("f_regression_rank requires non-constant targets."));
  }

  std::vector<std::pair<double, std::int64_t>> scores{};
  scores.reserve(static_cast<std::size_t>(dense.features));
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
    scores.emplace_back(options.absolute_score ? std::abs(score) : score, f);
  }

  std::partial_sort(scores.begin(),
                    scores.begin() + static_cast<std::ptrdiff_t>(std::min<std::int64_t>(options.top_k, dense.features)),
                    scores.end(),
                    [](const auto& a, const auto& b) {
                      if (a.first != b.first) return a.first > b.first;
                      return a.second < b.second;
                    });

  const std::int64_t out_k = std::min<std::int64_t>(options.top_k, dense.features);
  std::vector<std::int64_t> ids(static_cast<std::size_t>(out_k), 0);
  for (std::int64_t i = 0; i < out_k; ++i) {
    ids[static_cast<std::size_t>(i)] = scores[static_cast<std::size_t>(i)].second;
  }
  return StatusOr<Series>(pluss::make_series(ids, {out_k}, i64_cpu()));
}

} // namespace pluss::ai::nodej
