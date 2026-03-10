#include "pluss/ai/nodej/builders.h"

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

using pluss::Config;
using pluss::ScalarType;

const char* family_name(ModelFamily family) {
  switch (family) {
    case ModelFamily::Linear:
      return "Linear";
    case ModelFamily::Tree:
      return "Tree";
    case ModelFamily::Ensemble:
      return "Ensemble";
    case ModelFamily::Neighbor:
      return "Neighbor";
    case ModelFamily::Kernel:
      return "Kernel";
    case ModelFamily::Bayes:
      return "Bayes";
  }
  return "Unknown";
}

bool is_phase0_family(ModelFamily family) {
  switch (family) {
    case ModelFamily::Linear:
    case ModelFamily::Tree:
    case ModelFamily::Ensemble:
    case ModelFamily::Neighbor:
    case ModelFamily::Kernel:
    case ModelFamily::Bayes:
      return true;
  }
  return false;
}

[[nodiscard]] Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

[[nodiscard]] Config i64_cpu() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

[[nodiscard]] bool all_finite(const Series& x) {
  return x.isfinite().all().item<double>() == 1.0;
}

[[nodiscard]] bool has_nan(const Series& x) {
  return x.isnan().any().item<double>() == 1.0;
}

struct DenseFeatureMatrix {
  Series x_rows = Series::undefined();
  std::int64_t sample_count = 0;
  std::int64_t feature_count = 0;
  Platform original_platform = pluss::pCPU;
  ScalarType original_dtype = ScalarType::Float64;
};

[[nodiscard]] StatusOr<DenseFeatureMatrix> to_dense_rows(const FeatureBatch& batch, const char* where) {
  Status check = validate_features(batch);
  if (!check.ok()) return StatusOr<DenseFeatureMatrix>(std::move(check));
  if (batch.x.dim() != 2) {
    return StatusOr<DenseFeatureMatrix>(
        Status::shape_mismatch(std::string(where) + " requires rank-2 feature matrices."));
  }

  DenseFeatureMatrix out{};
  out.sample_count = batch.sample_count();
  out.feature_count = batch.feature_count();
  out.original_platform = batch.x.platform();
  out.original_dtype = batch.x.scalar_type();

  Series x = batch.x.to(pluss::pCPU, ScalarType::Float64);
  if (batch.layout == ObservationLayout::Columns) {
    x = x.transpose(0, 1);
  }
  if (x.sizes().size() != 2 || x.sizes()[0] != out.sample_count || x.sizes()[1] != out.feature_count) {
    return StatusOr<DenseFeatureMatrix>(
        Status::shape_mismatch(std::string(where) + " encountered inconsistent feature matrix shape."));
  }
  out.x_rows = std::move(x);
  return StatusOr<DenseFeatureMatrix>(std::move(out));
}

[[nodiscard]] StatusOr<Series> to_label_vector(const LabeledBatch& batch,
                                               std::int64_t expected_samples,
                                               const char* where) {
  if (!batch.y.defined()) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires a defined label vector."));
  }
  if (batch.y.numel() != expected_samples) {
    return StatusOr<Series>(Status::shape_mismatch(std::string(where) + " label count must match sample count."));
  }
  Series y = batch.y.to(pluss::pCPU, ScalarType::Float64).reshape({batch.y.numel()});
  if (has_nan(y)) {
    return StatusOr<Series>(Status::invalid_argument(
        std::string(where) + " does not support NaN labels; impute/drop missing labels before learn()."));
  }
  return StatusOr<Series>(std::move(y));
}

[[nodiscard]] Series prepend_intercept(const Series& x_rows) {
  const Series ones = pluss::ones({x_rows.sizes()[0], 1}, f64_cpu());
  return ones.unite({x_rows}, 1);
}

[[nodiscard]] StatusOr<Series> solve_ridge_least_squares(const Series& design,
                                                         const Series& labels,
                                                         double lambda,
                                                         bool fit_intercept,
                                                         double pinv_rcond) {
  if (!std::isfinite(lambda) || lambda < 0.0) {
    return StatusOr<Series>(Status::invalid_argument("Linear regression requires finite regularization_strength >= 0."));
  }
  if (!std::isfinite(pinv_rcond) || pinv_rcond <= 0.0) {
    return StatusOr<Series>(Status::invalid_argument("Linear regression requires pinv_rcond > 0."));
  }

  Series x_aug = design;
  Series y_aug = labels.reshape({labels.numel(), 1});
  const std::int64_t feature_dim = design.sizes()[1];

  if (lambda > 0.0) {
    Series penalty = pluss::eye(feature_dim, feature_dim, f64_cpu()).mul(std::sqrt(lambda));
    if (fit_intercept) {
      const Series idx0 = pluss::make_series(std::vector<std::int64_t>{0}, {1}, i64_cpu());
      penalty = pluss::index_put(penalty, std::vector<Series>{idx0, idx0}, pluss::zeros({1}, f64_cpu()), false);
    }
    x_aug = x_aug.unite({penalty}, 0);
    y_aug = y_aug.unite({pluss::zeros({feature_dim, 1}, f64_cpu())}, 0);
  }

  const std::array<double, 5> rcond_scales{1.0, 10.0, 100.0, 1000.0, 1.0e6};
  Series beta = Series::undefined();
  for (double scale : rcond_scales) {
    beta = pluss::subsp::math::linalg::pinverse(x_aug, pinv_rcond * scale).matmul(y_aug).reshape({feature_dim});
    if (all_finite(beta)) {
      return StatusOr<Series>(std::move(beta));
    }
  }
  return StatusOr<Series>(Status::numerical_failure("Linear regression failed to produce finite coefficients."));
}

[[nodiscard]] StatusOr<Series> regression_metrics(const Series& truth,
                                                  const Series& prediction,
                                                  const std::vector<MetricKind>& requested) {
  const Series y = truth.to(pluss::pCPU, ScalarType::Float64).reshape({truth.numel()});
  const Series yhat = prediction.to(pluss::pCPU, ScalarType::Float64).reshape({prediction.numel()});
  if (y.numel() != yhat.numel()) {
    return StatusOr<Series>(Status::shape_mismatch("assess regression metrics require equal truth/prediction lengths."));
  }
  const std::vector<double> yt = y.to_vec<double>();
  const std::vector<double> yp = yhat.to_vec<double>();
  if (yt.empty()) {
    return StatusOr<Series>(Status::invalid_argument("assess regression metrics require at least one sample."));
  }

  double mean_y = 0.0;
  for (double v : yt) mean_y += v;
  mean_y /= static_cast<double>(yt.size());

  double sse = 0.0;
  double sae = 0.0;
  double sst = 0.0;
  for (std::size_t i = 0; i < yt.size(); ++i) {
    const double err = yp[i] - yt[i];
    sse += err * err;
    sae += std::abs(err);
    const double d = yt[i] - mean_y;
    sst += d * d;
  }

  const double n = static_cast<double>(yt.size());
  const double mse = sse / n;
  const double mae = sae / n;
  const double r2 = (sst <= std::numeric_limits<double>::epsilon()) ? (sse <= std::numeric_limits<double>::epsilon() ? 1.0 : 0.0)
                                                                     : (1.0 - (sse / sst));

  std::vector<double> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Mse:
        out.push_back(mse);
        break;
      case MetricKind::Rmse:
        out.push_back(std::sqrt(mse));
        break;
      case MetricKind::Mae:
        out.push_back(mae);
        break;
      case MetricKind::R2:
        out.push_back(r2);
        break;
      default:
        return StatusOr<Series>(Status::invalid_argument("Unsupported regression metric in assess()."));
    }
  }

  return StatusOr<Series>(pluss::make_series(out, {static_cast<std::int64_t>(out.size())}, f64_cpu()));
}

[[nodiscard]] Status validate_linear_regression_options(const LinearFamilyOptions& options) {
  if (options.objective != LinearObjective::LeastSquares) {
    return Status::not_implemented("Linear regressor currently supports LinearObjective::LeastSquares only.");
  }
  if (options.regularization != RegularizationKind::None && options.regularization != RegularizationKind::L2) {
    return Status::not_implemented("Linear regressor currently supports RegularizationKind::None or RegularizationKind::L2.");
  }
  if (!std::isfinite(options.regularization_strength) || options.regularization_strength < 0.0) {
    return Status::invalid_argument("LinearFamilyOptions.regularization_strength must be finite and >= 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_neighbor_regression_options(const NeighborFamilyOptions& options) {
  if (options.k <= 0) {
    return Status::invalid_argument("Neighbor regressor requires NeighborFamilyOptions.k > 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_tree_regression_options(const TreeFamilyOptions& options) {
  if (options.criterion != TreeCriterion::Mse) {
    return Status::not_implemented("Tree regressor currently supports TreeCriterion::Mse only.");
  }
  if (options.max_depth < -1) {
    return Status::invalid_argument("TreeFamilyOptions.max_depth must be -1 (unlimited) or >= 0.");
  }
  if (options.min_samples_leaf <= 0) {
    return Status::invalid_argument("TreeFamilyOptions.min_samples_leaf must be positive.");
  }
  if (options.min_samples_split < 2) {
    return Status::invalid_argument("TreeFamilyOptions.min_samples_split must be >= 2.");
  }
  return Status::success();
}

void standardize_fit(std::vector<double>& values,
                     std::int64_t sample_count,
                     std::int64_t feature_count,
                     std::vector<double>& mean,
                     std::vector<double>& scale) {
  mean.assign(static_cast<std::size_t>(feature_count), 0.0);
  scale.assign(static_cast<std::size_t>(feature_count), 1.0);
  constexpr double floor = 1e-12;

  for (std::int64_t f = 0; f < feature_count; ++f) {
    double sum = 0.0;
    for (std::int64_t i = 0; i < sample_count; ++i) {
      sum += values[static_cast<std::size_t>(i * feature_count + f)];
    }
    const double m = sum / static_cast<double>(sample_count);
    mean[static_cast<std::size_t>(f)] = m;

    double var = 0.0;
    for (std::int64_t i = 0; i < sample_count; ++i) {
      const double d = values[static_cast<std::size_t>(i * feature_count + f)] - m;
      var += d * d;
    }
    var /= static_cast<double>(sample_count);
    const double s = std::sqrt(std::max(var, floor));
    scale[static_cast<std::size_t>(f)] = s;
  }

  for (std::int64_t i = 0; i < sample_count; ++i) {
    for (std::int64_t f = 0; f < feature_count; ++f) {
      const std::size_t idx = static_cast<std::size_t>(i * feature_count + f);
      values[idx] = (values[idx] - mean[static_cast<std::size_t>(f)]) / scale[static_cast<std::size_t>(f)];
    }
  }
}

void standardize_apply(std::vector<double>& values,
                       std::int64_t sample_count,
                       std::int64_t feature_count,
                       const std::vector<double>& mean,
                       const std::vector<double>& scale) {
  for (std::int64_t i = 0; i < sample_count; ++i) {
    for (std::int64_t f = 0; f < feature_count; ++f) {
      const std::size_t idx = static_cast<std::size_t>(i * feature_count + f);
      values[idx] = (values[idx] - mean[static_cast<std::size_t>(f)]) / scale[static_cast<std::size_t>(f)];
    }
  }
}

class NeighborRegressor final : public RegressorBase {
public:
  explicit NeighborRegressor(NeighborFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_neighbor_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "NeighborRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "NeighborRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "NeighborRegressor::learn");
    if (!y_or.ok()) return y_or.status();

    x_train_ = matrix.x_rows.to_vec<double>();
    y_train_ = y_or.value().to_vec<double>();
    if (static_cast<std::int64_t>(y_train_.size()) != matrix.sample_count) {
      return Status::internal("NeighborRegressor::learn internal label size mismatch.");
    }

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    k_ = std::min<std::int64_t>(options_.k, sample_count_);
    if (k_ <= 0) {
      return Status::invalid_argument("NeighborRegressor requires at least one training sample.");
    }

    if (options_.standardize) {
      standardize_fit(x_train_, sample_count_, feature_count_, mean_, scale_);
    } else {
      mean_.clear();
      scale_.clear();
    }

    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "NeighborRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "NeighborRegressor::infer does not support NaN features; apply the fitted imputer used for learn()."));
    }

    std::vector<double> x_query = matrix.x_rows.to_vec<double>();
    if (options_.standardize) {
      standardize_apply(x_query, matrix.sample_count, feature_count_, mean_, scale_);
    }

    std::vector<double> pred{};
    pred.reserve(static_cast<std::size_t>(matrix.sample_count));
    std::vector<std::pair<double, std::int64_t>> dist_idx(static_cast<std::size_t>(sample_count_));

    for (std::int64_t qi = 0; qi < matrix.sample_count; ++qi) {
      for (std::int64_t ti = 0; ti < sample_count_; ++ti) {
        double d2 = 0.0;
        for (std::int64_t f = 0; f < feature_count_; ++f) {
          const double qv = x_query[static_cast<std::size_t>(qi * feature_count_ + f)];
          const double tv = x_train_[static_cast<std::size_t>(ti * feature_count_ + f)];
          const double d = qv - tv;
          d2 += d * d;
        }
        dist_idx[static_cast<std::size_t>(ti)] = {d2, ti};
      }

      const std::int64_t neighbor_count = std::min<std::int64_t>(k_, sample_count_);
      if (neighbor_count < sample_count_) {
        std::nth_element(dist_idx.begin(), dist_idx.begin() + neighbor_count, dist_idx.end(),
                         [](const auto& a, const auto& b) { return a.first < b.first; });
      }
      std::sort(dist_idx.begin(), dist_idx.begin() + neighbor_count,
                [](const auto& a, const auto& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
                });

      double yhat = 0.0;
      if (options_.distance_weighted) {
        constexpr double eps = 1e-12;
        bool has_zero = false;
        for (std::int64_t i = 0; i < neighbor_count; ++i) {
          if (dist_idx[static_cast<std::size_t>(i)].first <= eps) {
            has_zero = true;
            break;
          }
        }

        double wsum = 0.0;
        double ysum = 0.0;
        for (std::int64_t i = 0; i < neighbor_count; ++i) {
          const auto [d2, idx] = dist_idx[static_cast<std::size_t>(i)];
          double w = 0.0;
          if (has_zero) {
            w = d2 <= eps ? 1.0 : 0.0;
          } else {
            w = 1.0 / (std::sqrt(d2) + eps);
          }
          ysum += w * y_train_[static_cast<std::size_t>(idx)];
          wsum += w;
        }
        yhat = wsum > 0.0 ? (ysum / wsum) : 0.0;
      } else {
        double sum = 0.0;
        for (std::int64_t i = 0; i < neighbor_count; ++i) {
          const auto idx = dist_idx[static_cast<std::size_t>(i)].second;
          sum += y_train_[static_cast<std::size_t>(idx)];
        }
        yhat = sum / static_cast<double>(neighbor_count);
      }
      pred.push_back(yhat);
    }

    Series out = pluss::make_series(pred, {static_cast<std::int64_t>(pred.size())}, f64_cpu());
    return StatusOr<Series>(out.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());

    const Series truth = batch.y.to(pluss::pCPU, ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("NeighborRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics);
  }

private:
  [[nodiscard]] Status validate_ready(const FeatureBatch& batch, const char* where) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition(std::string("Estimator must be learned before ") + where + "().");
    if (batch.feature_count() != feature_count_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    return Status::success();
  }

  NeighborFamilyOptions options_{};
  bool trained_{false};
  std::int64_t sample_count_{0};
  std::int64_t feature_count_{0};
  std::int64_t k_{1};
  std::vector<double> x_train_{};
  std::vector<double> y_train_{};
  std::vector<double> mean_{};
  std::vector<double> scale_{};
};

class TreeRegressor final : public RegressorBase {
public:
  explicit TreeRegressor(TreeFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_tree_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "TreeRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "TreeRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "TreeRegressor::learn");
    if (!y_or.ok()) return y_or.status();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    x_train_ = matrix.x_rows.to_vec<double>();
    y_train_ = y_or.value().to_vec<double>();
    if (static_cast<std::int64_t>(y_train_.size()) != sample_count_) {
      return Status::internal("TreeRegressor::learn internal label size mismatch.");
    }

    nodes_.clear();
    nodes_.reserve(static_cast<std::size_t>(2 * sample_count_ + 1));
    std::vector<std::int64_t> root_indices(static_cast<std::size_t>(sample_count_));
    std::iota(root_indices.begin(), root_indices.end(), static_cast<std::int64_t>(0));
    build_node(root_indices, 0);

    if (nodes_.empty()) {
      return Status::internal("TreeRegressor::learn failed to build any tree nodes.");
    }

    x_train_.clear();
    x_train_.shrink_to_fit();
    y_train_.clear();
    y_train_.shrink_to_fit();

    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "TreeRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "TreeRegressor::infer does not support NaN features; apply the fitted imputer used for learn()."));
    }

    const std::vector<double> x_query = matrix.x_rows.to_vec<double>();
    std::vector<double> pred{};
    pred.reserve(static_cast<std::size_t>(matrix.sample_count));

    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      std::int64_t node_idx = 0;
      while (!nodes_[static_cast<std::size_t>(node_idx)].is_leaf) {
        const TreeNode& node = nodes_[static_cast<std::size_t>(node_idx)];
        const double xv = x_query[static_cast<std::size_t>(i * feature_count_ + node.feature)];
        node_idx = xv < node.threshold ? node.left : node.right;
      }
      pred.push_back(nodes_[static_cast<std::size_t>(node_idx)].leaf_value);
    }

    Series out = pluss::make_series(pred, {static_cast<std::int64_t>(pred.size())}, f64_cpu());
    return StatusOr<Series>(out.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());

    const Series truth = batch.y.to(pluss::pCPU, ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("TreeRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics);
  }

private:
  struct TreeNode {
    bool is_leaf = true;
    std::int64_t feature = -1;
    double threshold = 0.0;
    std::int64_t left = -1;
    std::int64_t right = -1;
    double leaf_value = 0.0;
  };

  struct SplitCandidate {
    bool valid = false;
    std::int64_t feature = -1;
    double threshold = 0.0;
    double gain = 0.0;
  };

  [[nodiscard]] Status validate_ready(const FeatureBatch& batch, const char* where) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition(std::string("Estimator must be learned before ") + where + "().");
    if (batch.feature_count() != feature_count_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    return Status::success();
  }

  [[nodiscard]] double x_at(std::int64_t row, std::int64_t feature) const {
    return x_train_[static_cast<std::size_t>(row * feature_count_ + feature)];
  }

  std::int64_t build_node(const std::vector<std::int64_t>& indices, std::int64_t depth) {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());

    double sum = 0.0;
    double sumsq = 0.0;
    for (std::int64_t idx : indices) {
      const double yi = y_train_[static_cast<std::size_t>(idx)];
      sum += yi;
      sumsq += yi * yi;
    }

    const double mean = sum / static_cast<double>(n);
    const double sse = sumsq - (sum * sum) / static_cast<double>(n);
    const double parent_impurity = std::max(0.0, sse / static_cast<double>(n));

    TreeNode node{};
    node.leaf_value = mean;

    const std::int64_t node_index = static_cast<std::int64_t>(nodes_.size());
    nodes_.push_back(node);

    const bool depth_limited = options_.max_depth >= 0 && depth >= options_.max_depth;
    const bool size_limited = n < options_.min_samples_split || n < (2 * options_.min_samples_leaf);
    constexpr double impurity_floor = 1e-14;
    if (depth_limited || size_limited || parent_impurity <= impurity_floor) {
      return node_index;
    }

    SplitCandidate best = find_best_split(indices, parent_impurity);
    if (!best.valid || best.gain <= impurity_floor) {
      return node_index;
    }

    std::vector<std::int64_t> left{};
    std::vector<std::int64_t> right{};
    left.reserve(indices.size());
    right.reserve(indices.size());
    for (std::int64_t idx : indices) {
      if (x_at(idx, best.feature) < best.threshold) {
        left.push_back(idx);
      } else {
        right.push_back(idx);
      }
    }
    if (left.empty() || right.empty()) {
      return node_index;
    }

    TreeNode& split_node = nodes_[static_cast<std::size_t>(node_index)];
    split_node.is_leaf = false;
    split_node.feature = best.feature;
    split_node.threshold = best.threshold;
    split_node.left = build_node(left, depth + 1);
    split_node.right = build_node(right, depth + 1);
    return node_index;
  }

  [[nodiscard]] SplitCandidate find_best_split(const std::vector<std::int64_t>& indices, double parent_impurity) const {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());
    SplitCandidate best{};

    for (std::int64_t f = 0; f < feature_count_; ++f) {
      std::vector<std::pair<double, double>> values{};
      values.reserve(static_cast<std::size_t>(n));
      for (std::int64_t idx : indices) {
        values.emplace_back(x_at(idx, f), y_train_[static_cast<std::size_t>(idx)]);
      }
      std::sort(values.begin(), values.end(),
                [](const auto& a, const auto& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
                });
      if (values.front().first == values.back().first) continue;

      double left_sum = 0.0;
      double left_sumsq = 0.0;
      double total_sum = 0.0;
      double total_sumsq = 0.0;
      for (const auto& pair : values) {
        total_sum += pair.second;
        total_sumsq += pair.second * pair.second;
      }

      for (std::int64_t i = 0; i < n - 1; ++i) {
        const double yi = values[static_cast<std::size_t>(i)].second;
        left_sum += yi;
        left_sumsq += yi * yi;
        const std::int64_t left_n = i + 1;
        const std::int64_t right_n = n - left_n;
        if (left_n < options_.min_samples_leaf || right_n < options_.min_samples_leaf) continue;

        const double xi = values[static_cast<std::size_t>(i)].first;
        const double xj = values[static_cast<std::size_t>(i + 1)].first;
        if (xi == xj) continue;

        const double right_sum = total_sum - left_sum;
        const double right_sumsq = total_sumsq - left_sumsq;
        const double left_sse = left_sumsq - (left_sum * left_sum) / static_cast<double>(left_n);
        const double right_sse = right_sumsq - (right_sum * right_sum) / static_cast<double>(right_n);
        const double weighted_impurity = std::max(0.0, (left_sse + right_sse) / static_cast<double>(n));
        const double gain = parent_impurity - weighted_impurity;
        const double threshold = xi + (xj - xi) * 0.5;

        const bool better_gain = gain > best.gain + 1e-15;
        const bool tie_break = std::abs(gain - best.gain) <= 1e-15
                               && (f < best.feature || (f == best.feature && threshold < best.threshold));
        if (!best.valid || better_gain || tie_break) {
          best.valid = true;
          best.feature = f;
          best.threshold = threshold;
          best.gain = gain;
        }
      }
    }

    return best;
  }

  TreeFamilyOptions options_{};
  bool trained_{false};
  std::int64_t sample_count_{0};
  std::int64_t feature_count_{0};
  std::vector<double> x_train_{};
  std::vector<double> y_train_{};
  std::vector<TreeNode> nodes_{};
};

class LinearRegressor final : public RegressorBase {
public:
  explicit LinearRegressor(LinearFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_linear_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "LinearRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "LinearRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "LinearRegressor::learn");
    if (!y_or.ok()) return y_or.status();
    Series y = y_or.move_value();

    Series design = matrix.x_rows;
    if (options_.fit_intercept) {
      design = prepend_intercept(design);
    }

    const double lambda =
        options_.regularization == RegularizationKind::L2 ? options_.regularization_strength : 0.0;
    StatusOr<Series> beta_or = solve_ridge_least_squares(design, y, lambda, options_.fit_intercept, pinv_rcond_);
    if (!beta_or.ok()) return beta_or.status();

    beta_ = beta_or.move_value();
    feature_count_ = matrix.feature_count;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "LinearRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "LinearRegressor::infer does not support NaN features; apply the same fitted imputer used for learn()."));
    }

    Series design = matrix.x_rows;
    if (options_.fit_intercept) {
      design = prepend_intercept(design);
    }

    const Series pred = design.matmul(beta_.reshape({beta_.numel(), 1})).reshape({matrix.sample_count});
    if (!all_finite(pred)) {
      return StatusOr<Series>(Status::numerical_failure("LinearRegressor::infer produced non-finite predictions."));
    }
    return StatusOr<Series>(pred.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());

    const Series truth = batch.y.to(pluss::pCPU, ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("LinearRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics);
  }

private:
  [[nodiscard]] Status validate_ready(const FeatureBatch& batch, const char* where) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition(std::string("Estimator must be learned before ") + where + "().");
    if (batch.feature_count() != feature_count_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    return Status::success();
  }

  LinearFamilyOptions options_{};
  bool trained_{false};
  std::int64_t feature_count_{0};
  Series beta_ = Series::undefined();
  double pinv_rcond_{1e-12};
};

class FamilyRegressor final : public RegressorBase {
public:
  explicit FamilyRegressor(ModelFamily family) : family_(family) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    features_ = batch.features.feature_count();
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch);
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));
    return StatusOr<Series>(
        Status::not_implemented(std::string(family_name(family_)) + " regressor infer() is scheduled after Phase 0."));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    (void)options;
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));
    if (!trained_) return StatusOr<Series>(Status::precondition("Estimator must be learned before assess()."));
    if (batch.features.feature_count() != features_) {
      return StatusOr<Series>(Status::shape_mismatch("Feature count mismatch between learn and assess batch."));
    }
    return StatusOr<Series>(
        Status::not_implemented(std::string(family_name(family_)) + " regressor assess() is scheduled after Phase 0."));
  }

private:
  [[nodiscard]] Status validate_ready(const FeatureBatch& batch) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition("Estimator must be learned before infer().");
    if (batch.feature_count() != features_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    return Status::success();
  }

  ModelFamily family_;
  bool trained_{false};
  std::int64_t features_{0};
};

} // namespace

StatusOr<std::unique_ptr<RegressorBase>> make_regressor(const ModelSpec& spec) {
  if (spec.task != TaskKind::Regression) {
    return StatusOr<std::unique_ptr<RegressorBase>>(
        Status::invalid_argument("make_regressor requires ModelSpec.task = TaskKind::Regression."));
  }
  if (!is_phase0_family(spec.family)) {
    return StatusOr<std::unique_ptr<RegressorBase>>(Status::invalid_argument("Unsupported regressor family."));
  }
  if (spec.family == ModelFamily::Linear) {
    const auto* options = std::get_if<LinearFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Linear regressor requires LinearFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<LinearRegressor>(*options));
  }
  if (spec.family == ModelFamily::Tree) {
    const auto* options = std::get_if<TreeFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Tree regressor requires TreeFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<TreeRegressor>(*options));
  }
  if (spec.family == ModelFamily::Neighbor) {
    const auto* options = std::get_if<NeighborFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Neighbor regressor requires NeighborFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<NeighborRegressor>(*options));
  }
  return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<FamilyRegressor>(spec.family));
}

} // namespace pluss::ai::nodej
