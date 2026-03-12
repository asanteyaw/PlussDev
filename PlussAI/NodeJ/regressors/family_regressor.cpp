#include "pluss/ai/nodej/builders.h"

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <array>
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
    case ModelFamily::Cluster:
      return "Cluster";
    case ModelFamily::Decompose:
      return "Decompose";
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
    case ModelFamily::Cluster:
    case ModelFamily::Decompose:
      return false;
  }
  return false;
}

[[nodiscard]] Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

[[nodiscard]] Config i64_cpu() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

[[nodiscard]] Config f64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(platform);
}

[[nodiscard]] Series state_f64(std::vector<double> values) {
  const std::int64_t n = static_cast<std::int64_t>(values.size());
  return pluss::make_series(std::move(values), {n}, f64_cpu());
}

[[nodiscard]] Series state_i64(std::vector<std::int64_t> values) {
  const std::int64_t n = static_cast<std::int64_t>(values.size());
  return pluss::make_series(std::move(values), {n}, i64_cpu());
}

[[nodiscard]] std::vector<double> vec_f64(const Series& x) {
  if (!x.defined()) return {};
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

[[nodiscard]] std::vector<std::int64_t> vec_i64(const Series& x) {
  if (!x.defined()) return {};
  return x.to(pluss::pCPU, ScalarType::Int64).reshape({x.numel()}).to_vec<std::int64_t>();
}

[[nodiscard]] Series make_index_series(const std::vector<std::int64_t>& indices, Platform platform) {
  return pluss::make_series(
      indices,
      {static_cast<std::int64_t>(indices.size())},
      Config{}.with_dtype(ScalarType::Int64).with_platform(platform));
}

[[nodiscard]] bool all_finite(const Series& x) {
  return x.isfinite().all().item<bool>();
}

[[nodiscard]] bool has_nan(const Series& x) {
  return x.isnan().any().item<bool>();
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

  Series x = batch.x.to(ScalarType::Float64);
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
  Series y = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
  if (has_nan(y)) {
    return StatusOr<Series>(Status::invalid_argument(
        std::string(where) + " does not support NaN labels; impute/drop missing labels before learn()."));
  }
  return StatusOr<Series>(std::move(y));
}

[[nodiscard]] Series prepend_intercept(const Series& x_rows) {
  const Series ones = pluss::ones({x_rows.sizes()[0], 1}, f64_on(x_rows.platform()));
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
    Series penalty = pluss::eye(feature_dim, feature_dim, f64_on(design.platform())).mul(std::sqrt(lambda));
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, penalty.platform());
      penalty = pluss::index_put(penalty, std::vector<Series>{idx0, idx0}, pluss::zeros({1}, f64_on(penalty.platform())), false);
    }
    x_aug = x_aug.unite({penalty}, 0);
    y_aug = y_aug.unite({pluss::zeros({feature_dim, 1}, f64_on(design.platform()))}, 0);
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
                                                  const std::vector<MetricKind>& requested,
                                                  double quantile) {
  const Series y = truth.to(ScalarType::Float64).reshape({truth.numel()});
  const Series yhat = prediction.to(ScalarType::Float64).reshape({prediction.numel()});
  if (y.numel() != yhat.numel()) {
    return StatusOr<Series>(Status::shape_mismatch("assess regression metrics require equal truth/prediction lengths."));
  }
  if (y.numel() == 0) {
    return StatusOr<Series>(Status::invalid_argument("assess regression metrics require at least one sample."));
  }
  if (!(quantile > 0.0 && quantile < 1.0)) {
    return StatusOr<Series>(Status::invalid_argument("AssessOptions.regression_quantile must satisfy 0 < q < 1."));
  }

  const Series err = yhat.sub(y);
  const Series residual = y.sub(yhat);
  const Series sse = err.square().sum();
  const Series mse = err.square().mean();
  const Series mae = err.abs().mean();
  const Series neg_indicator = residual.lt(0.0).to(ScalarType::Float64);
  const Series quantile_loss = residual.mul(quantile).sub(residual.mul(neg_indicator)).mean();
  const Series y_centered = y.sub(y.mean());
  const Series sst = y_centered.square().sum();
  const Series zero = pluss::zeros_like(sst, ScalarType::Float64);
  const Series one = pluss::ones_like(sst, ScalarType::Float64);
  const Series sse_small = sse.le(std::numeric_limits<double>::epsilon()).to(ScalarType::Float64);
  const Series r2 = sst.gt(std::numeric_limits<double>::epsilon())
                        .where(one.sub(sse.div(sst)),
                               sse_small.where(one, zero));

  std::vector<Series> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Mse:
        out.push_back(mse.reshape({1}));
        break;
      case MetricKind::Rmse:
        out.push_back(mse.sqrt().reshape({1}));
        break;
      case MetricKind::Mae:
        out.push_back(mae.reshape({1}));
        break;
      case MetricKind::R2:
        out.push_back(r2.reshape({1}));
        break;
      case MetricKind::QuantileLoss:
        out.push_back(quantile_loss.reshape({1}));
        break;
      default:
        return StatusOr<Series>(Status::invalid_argument("Unsupported regression metric in assess()."));
    }
  }
  if (out.empty()) {
    return StatusOr<Series>(pluss::make_series(std::vector<double>{}, {0}, f64_on(y.platform())));
  }
  Series merged = out.front();
  for (std::size_t i = 1; i < out.size(); ++i) {
    merged = merged.unite({out[i]}, 0);
  }
  return StatusOr<Series>(merged.reshape({static_cast<std::int64_t>(out.size())}).to(y.platform(), ScalarType::Float64));
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

[[nodiscard]] Status validate_kernel_regression_options(const KernelFamilyOptions& options) {
  if (!std::isfinite(options.c) || options.c <= 0.0) {
    return Status::invalid_argument("KernelFamilyOptions.c must be finite and > 0.");
  }
  if (!std::isfinite(options.gamma) || options.gamma < 0.0) {
    return Status::invalid_argument("KernelFamilyOptions.gamma must be finite and >= 0.");
  }
  if (!std::isfinite(options.epsilon) || options.epsilon < 0.0) {
    return Status::invalid_argument("KernelFamilyOptions.epsilon must be finite and >= 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_bayes_regression_options(const BayesFamilyOptions& options) {
  if (!options.use_gaussian) {
    return Status::not_implemented("Bayes regressor currently supports gaussian likelihoods only.");
  }
  if (!std::isfinite(options.smoothing) || options.smoothing <= 0.0) {
    return Status::invalid_argument("BayesFamilyOptions.smoothing must be finite and > 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_ensemble_regression_options(const EnsembleFamilyOptions& options) {
  if (options.method != EnsembleMethod::Bagging && options.method != EnsembleMethod::RandomForest) {
    return Status::not_implemented(
        "Ensemble regressor currently supports EnsembleMethod::Bagging and EnsembleMethod::RandomForest only.");
  }
  if (options.learners <= 0) {
    return Status::invalid_argument("EnsembleFamilyOptions.learners must be positive.");
  }
  if (!std::isfinite(options.subsample) || options.subsample <= 0.0 || options.subsample > 1.0) {
    return Status::invalid_argument("EnsembleFamilyOptions.subsample must be finite and in (0, 1].");
  }
  return Status::success();
}

[[nodiscard]] Series standardize_fit_rows(const Series& x_rows, Series& mean, Series& scale) {
  const std::int64_t feature_count = x_rows.sizes()[1];
  mean = x_rows.mean(0, false).reshape({feature_count});
  const Series centered = x_rows.sub(mean.reshape({1, feature_count}));
  scale = centered.square().mean(0, false).reshape({feature_count}).clamp_min(1e-12).sqrt();
  return centered.div(scale.reshape({1, feature_count}));
}

[[nodiscard]] Series standardize_apply_rows(const Series& x_rows, const Series& mean, const Series& scale) {
  const std::int64_t feature_count = x_rows.sizes()[1];
  return x_rows.sub(mean.reshape({1, feature_count})).div(scale.reshape({1, feature_count}));
}

[[nodiscard]] Series pairwise_distance2(const Series& query_rows, const Series& train_rows) {
  const Series dot = query_rows.matmul(train_rows.transpose(0, 1));
  const Series q_norm = query_rows.square().sum(1, true);
  const Series t_norm = train_rows.square().sum(1, true).transpose(0, 1);
  return q_norm.add(t_norm).sub(dot.mul(2.0)).relu();
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

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    k_ = std::min<std::int64_t>(options_.k, sample_count_);
    if (k_ <= 0) {
      return Status::invalid_argument("NeighborRegressor requires at least one training sample.");
    }

    Series x_train = matrix.x_rows;
    if (options_.standardize) {
      x_train = standardize_fit_rows(x_train, mean_, scale_);
    } else {
      mean_ = Series::undefined();
      scale_ = Series::undefined();
    }
    x_train_ = std::move(x_train);
    y_train_ = y_or.value().reshape({sample_count_});

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

    Series x_query = matrix.x_rows;
    if (options_.standardize) {
      x_query = standardize_apply_rows(x_query, mean_, scale_);
    }

    const std::int64_t neighbor_count = std::min<std::int64_t>(k_, sample_count_);
    const Series dist2 = pairwise_distance2(x_query, x_train_);
    const Series sort_idx = dist2.argsort(1, false, true);
    const Series k_cols = pluss::arange(0, neighbor_count, Config{}.with_dtype(ScalarType::Int64).with_platform(sort_idx.platform()));
    const Series knn_idx = sort_idx.index_select(1, k_cols);
    const Series y_train_2d =
        y_train_.reshape({1, sample_count_}).expand(std::vector<std::int64_t>{matrix.sample_count, sample_count_});
    const Series y_knn = y_train_2d.gather(1, knn_idx);

    Series pred = Series::undefined();
    if (options_.distance_weighted) {
      constexpr double eps = 1e-12;
      const Series d2_knn = dist2.gather(1, knn_idx);
      const Series zero_mask = d2_knn.le(eps).to(ScalarType::Float64);
      const Series has_zero = zero_mask.any(1, true).to(ScalarType::Float64);
      const Series inv_dist = pluss::ones_like(d2_knn, ScalarType::Float64).div(d2_knn.sqrt().add(eps));
      const Series weights = has_zero.mul(zero_mask).add(has_zero.mul(-1.0).add(1.0).mul(inv_dist));
      const Series wsum = weights.sum(1, false);
      const Series ysum = weights.mul(y_knn).sum(1, false);
      pred = wsum.gt(0.0).where(ysum.div(wsum), pluss::zeros_like(wsum, ScalarType::Float64));
    } else {
      pred = y_knn.mean(1, false);
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

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("NeighborRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(feature_count_),
                   static_cast<double>(sample_count_),
                   static_cast<double>(k_)}),
        x_train_,
        y_train_,
        mean_,
        scale_,
    });
  }

  Status load_state(const std::vector<Series>& state) override {
    if (state.size() < 5) return Status::invalid_argument("NeighborRegressor::load_state expects 5 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 4) return Status::invalid_argument("NeighborRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    sample_count_ = static_cast<std::int64_t>(meta[2]);
    k_ = static_cast<std::int64_t>(meta[3]);
    x_train_ = state[1];
    y_train_ = state[2];
    mean_ = state[3];
    scale_ = state[4];
    return Status::success();
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
  Series x_train_ = Series::undefined();
  Series y_train_ = Series::undefined();
  Series mean_ = Series::undefined();
  Series scale_ = Series::undefined();
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
    x_train_ = matrix.x_rows;
    y_train_ = y_or.value().reshape({sample_count_});

    nodes_.clear();
    nodes_.reserve(static_cast<std::size_t>(2 * sample_count_ + 1));
    std::vector<std::int64_t> root_indices(static_cast<std::size_t>(sample_count_));
    std::iota(root_indices.begin(), root_indices.end(), static_cast<std::int64_t>(0));
    build_node(root_indices, 0);

    if (nodes_.empty()) {
      return Status::internal("TreeRegressor::learn failed to build any tree nodes.");
    }

    x_train_ = Series::undefined();
    y_train_ = Series::undefined();

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

    std::vector<double> pred{};
    pred.reserve(static_cast<std::size_t>(matrix.sample_count));

    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      std::int64_t node_idx = 0;
      while (!nodes_[static_cast<std::size_t>(node_idx)].is_leaf) {
        const TreeNode& node = nodes_[static_cast<std::size_t>(node_idx)];
        const double xv = x_at(matrix.x_rows, i, node.feature);
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

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("TreeRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    std::vector<double> flat{};
    flat.reserve(nodes_.size() * 6);
    for (const auto& node : nodes_) {
      flat.push_back(node.is_leaf ? 1.0 : 0.0);
      flat.push_back(static_cast<double>(node.feature));
      flat.push_back(node.threshold);
      flat.push_back(static_cast<double>(node.left));
      flat.push_back(static_cast<double>(node.right));
      flat.push_back(node.leaf_value);
    }
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(feature_count_),
                   static_cast<double>(sample_count_),
                   static_cast<double>(nodes_.size())}),
        state_f64(std::move(flat)),
    });
  }

  Status load_state(const std::vector<Series>& state) override {
    if (state.size() < 2) return Status::invalid_argument("TreeRegressor::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 4) return Status::invalid_argument("TreeRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    sample_count_ = static_cast<std::int64_t>(meta[2]);
    const std::size_t node_count = static_cast<std::size_t>(std::max<double>(0.0, meta[3]));
    const auto flat = vec_f64(state[1]);
    nodes_.assign(node_count, TreeNode{});
    for (std::size_t i = 0; i < node_count; ++i) {
      const std::size_t base = i * 6;
      if (base + 5 >= flat.size()) break;
      TreeNode node{};
      node.is_leaf = flat[base] > 0.5;
      node.feature = static_cast<std::int64_t>(flat[base + 1]);
      node.threshold = flat[base + 2];
      node.left = static_cast<std::int64_t>(flat[base + 3]);
      node.right = static_cast<std::int64_t>(flat[base + 4]);
      node.leaf_value = flat[base + 5];
      nodes_[i] = node;
    }
    x_train_ = Series::undefined();
    y_train_ = Series::undefined();
    return Status::success();
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

  [[nodiscard]] double x_at(const Series& x_rows, std::int64_t row, std::int64_t feature) const {
    const Series row_idx = make_index_series(std::vector<std::int64_t>{row}, x_rows.platform());
    const Series col_idx = make_index_series(std::vector<std::int64_t>{feature}, x_rows.platform());
    return x_rows.index_select(0, row_idx).index_select(1, col_idx).reshape({1}).item<double>();
  }

  [[nodiscard]] double y_at(std::int64_t row) const {
    const Series row_idx = make_index_series(std::vector<std::int64_t>{row}, y_train_.platform());
    return y_train_.index_select(0, row_idx).reshape({1}).item<double>();
  }

  std::int64_t build_node(const std::vector<std::int64_t>& indices, std::int64_t depth) {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());

    double sum = 0.0;
    double sumsq = 0.0;
    for (std::int64_t idx : indices) {
      const double yi = y_at(idx);
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
      if (x_at(x_train_, idx, best.feature) < best.threshold) {
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
        values.emplace_back(x_at(x_train_, idx, f), y_at(idx));
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
  Series x_train_ = Series::undefined();
  Series y_train_ = Series::undefined();
  std::vector<TreeNode> nodes_{};
};

class BayesRegressor final : public RegressorBase {
public:
  explicit BayesRegressor(BayesFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_bayes_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "BayesRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "BayesRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "BayesRegressor::learn");
    if (!y_or.ok()) return y_or.status();
    Series y = y_or.move_value();

    Series design = prepend_intercept(matrix.x_rows);
    StatusOr<Series> beta_or = solve_ridge_least_squares(design, y, options_.smoothing, true, pinv_rcond_);
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

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "BayesRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(
          Status::invalid_argument("BayesRegressor::infer does not support NaN features; apply the same fitted imputer used for learn()."));
    }

    Series design = prepend_intercept(matrix.x_rows);
    const Series pred = design.matmul(beta_.reshape({beta_.numel(), 1})).reshape({matrix.sample_count});
    if (!all_finite(pred)) {
      return StatusOr<Series>(Status::numerical_failure("BayesRegressor::infer produced non-finite predictions."));
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

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("BayesRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(feature_count_),
                   pinv_rcond_}),
        beta_,
    });
  }

  Status load_state(const std::vector<Series>& state) override {
    if (state.size() < 2) return Status::invalid_argument("BayesRegressor::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 3) return Status::invalid_argument("BayesRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    pinv_rcond_ = meta[2];
    beta_ = state[1];
    return Status::success();
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

  BayesFamilyOptions options_{};
  bool trained_{false};
  std::int64_t feature_count_{0};
  Series beta_ = Series::undefined();
  double pinv_rcond_{1e-12};
};

class EnsembleRegressor final : public RegressorBase {
public:
  explicit EnsembleRegressor(EnsembleFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_ensemble_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "EnsembleRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "EnsembleRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "EnsembleRegressor::learn");
    if (!y_or.ok()) return y_or.status();
    const Series y_train = y_or.value().reshape({matrix.sample_count});
    const Series x_train = matrix.x_rows;

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;

    const std::int64_t learner_count = options_.learners;
    const std::int64_t subsample_count =
        std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(options_.subsample * sample_count_)));
    const std::int64_t mtry = options_.method == EnsembleMethod::RandomForest
                                  ? std::max<std::int64_t>(1, feature_count_ / 3)
                                  : feature_count_;

    std::mt19937_64 rng{};
    if (options.random.deterministic) {
      rng.seed(options.random.seed);
    } else {
      std::random_device rd;
      rng.seed((static_cast<std::uint64_t>(rd()) << 32U) ^ static_cast<std::uint64_t>(rd()));
    }

    learners_.clear();
    learners_.reserve(static_cast<std::size_t>(learner_count));
    std::uniform_int_distribution<std::int64_t> row_dist(0, sample_count_ - 1);

    for (std::int64_t l = 0; l < learner_count; ++l) {
      std::vector<std::int64_t> feature_ids(static_cast<std::size_t>(feature_count_));
      std::iota(feature_ids.begin(), feature_ids.end(), static_cast<std::int64_t>(0));
      if (mtry < feature_count_) {
        std::shuffle(feature_ids.begin(), feature_ids.end(), rng);
        feature_ids.resize(static_cast<std::size_t>(mtry));
        std::sort(feature_ids.begin(), feature_ids.end());
      }

      const std::int64_t learner_features = static_cast<std::int64_t>(feature_ids.size());
      const Series feature_index = make_index_series(feature_ids, x_train.platform());
      const Series x_feature_view = x_train.index_select(1, feature_index);
      std::vector<std::int64_t> row_ids(static_cast<std::size_t>(subsample_count), 0);

      for (std::int64_t i = 0; i < subsample_count; ++i) {
        row_ids[static_cast<std::size_t>(i)] = row_dist(rng);
      }

      const Series row_index = make_index_series(row_ids, x_train.platform());
      const Series x_series = x_feature_view.index_select(0, row_index).reshape({subsample_count, learner_features});
      const Series y_series = y_train.index_select(0, row_index).reshape({subsample_count});
      const LabeledBatch sub_batch{
          .features = FeatureBatch{.x = x_series, .layout = ObservationLayout::Rows},
          .y = y_series,
      };

      auto tree = std::make_unique<TreeRegressor>(TreeFamilyOptions{
          .criterion = TreeCriterion::Mse,
      });
      Status tree_status = tree->learn(sub_batch, options);
      if (!tree_status.ok()) {
        return Status::internal("EnsembleRegressor::learn failed while fitting base tree.");
      }

      learners_.push_back(EnsembleMember{
          .tree = std::move(tree),
          .feature_ids = std::move(feature_ids),
      });
    }

    if (learners_.empty()) {
      return Status::internal("EnsembleRegressor::learn did not build any learners.");
    }

    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "EnsembleRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "EnsembleRegressor::infer does not support NaN features; apply the fitted imputer used for learn()."));
    }

    Series pred_sum = pluss::zeros({matrix.sample_count}, f64_cpu()).to(matrix.x_rows.platform());

    for (const EnsembleMember& member : learners_) {
      const std::int64_t learner_features = static_cast<std::int64_t>(member.feature_ids.size());
      const Series feature_index = make_index_series(member.feature_ids, matrix.x_rows.platform());
      const Series x_sub = matrix.x_rows.index_select(1, feature_index);

      const FeatureBatch sub_batch{
          .x = x_sub.reshape({matrix.sample_count, learner_features}),
          .layout = ObservationLayout::Rows,
      };
      StatusOr<Series> member_pred_or = member.tree->infer(sub_batch);
      if (!member_pred_or.ok()) return StatusOr<Series>(member_pred_or.status());
      pred_sum = pred_sum.add(member_pred_or.value().to(matrix.x_rows.platform(), ScalarType::Float64));
    }

    const double denom = static_cast<double>(learners_.size());
    Series out = pred_sum.div(denom);
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

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("EnsembleRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    std::vector<Series> out{};
    out.reserve(2 + learners_.size() * 4);
    out.push_back(state_f64({trained_ ? 1.0 : 0.0,
                             static_cast<double>(sample_count_),
                             static_cast<double>(feature_count_),
                             static_cast<double>(learners_.size())}));
    for (const auto& member : learners_) {
      out.push_back(state_i64(member.feature_ids));
      if (!member.tree) {
        out.push_back(state_i64({0}));
        continue;
      }
      StatusOr<std::vector<Series>> tree_state_or = member.tree->dump_state();
      if (!tree_state_or.ok()) return StatusOr<std::vector<Series>>(tree_state_or.status());
      const auto& tree_state = tree_state_or.value();
      out.push_back(state_i64({static_cast<std::int64_t>(tree_state.size())}));
      out.insert(out.end(), tree_state.begin(), tree_state.end());
    }
    return StatusOr<std::vector<Series>>(std::move(out));
  }

  Status load_state(const std::vector<Series>& state) override {
    if (state.empty()) return Status::invalid_argument("EnsembleRegressor::load_state expects metadata tensor.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 4) return Status::invalid_argument("EnsembleRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    const std::int64_t learner_count = static_cast<std::int64_t>(meta[3]);

    learners_.clear();
    learners_.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, learner_count)));
    std::size_t cursor = 1;
    for (std::int64_t li = 0; li < learner_count; ++li) {
      if (cursor >= state.size()) return Status::invalid_argument("EnsembleRegressor::load_state truncated state.");
      EnsembleMember member{};
      member.feature_ids = vec_i64(state[cursor++]);
      if (cursor >= state.size()) return Status::invalid_argument("EnsembleRegressor::load_state missing tree state size.");
      const auto size_vec = vec_i64(state[cursor++]);
      const std::int64_t tree_items = size_vec.empty() ? 0 : size_vec.front();
      if (tree_items <= 0) {
        learners_.push_back(std::move(member));
        continue;
      }
      std::vector<Series> tree_state{};
      tree_state.reserve(static_cast<std::size_t>(tree_items));
      for (std::int64_t ti = 0; ti < tree_items; ++ti) {
        if (cursor >= state.size()) return Status::invalid_argument("EnsembleRegressor::load_state truncated tree state.");
        tree_state.push_back(state[cursor++]);
      }
      auto tree = std::make_unique<TreeRegressor>(TreeFamilyOptions{.criterion = TreeCriterion::Mse});
      Status load_status = tree->load_state(tree_state);
      if (!load_status.ok()) return load_status;
      member.tree = std::move(tree);
      learners_.push_back(std::move(member));
    }
    return Status::success();
  }

private:
  struct EnsembleMember {
    std::unique_ptr<TreeRegressor> tree{};
    std::vector<std::int64_t> feature_ids{};
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

  EnsembleFamilyOptions options_{};
  bool trained_{false};
  std::int64_t sample_count_{0};
  std::int64_t feature_count_{0};
  std::vector<EnsembleMember> learners_{};
};

[[nodiscard]] double resolved_gamma(double gamma, std::int64_t feature_count) {
  if (gamma > 0.0) return gamma;
  return 1.0 / static_cast<double>(std::max<std::int64_t>(feature_count, 1));
}

[[nodiscard]] double kernel_eval(KernelKind kind,
                                 double gamma,
                                 const double* a,
                                 const double* b,
                                 std::int64_t feature_count) {
  double dot = 0.0;
  double d2 = 0.0;
  for (std::int64_t f = 0; f < feature_count; ++f) {
    const double av = a[static_cast<std::size_t>(f)];
    const double bv = b[static_cast<std::size_t>(f)];
    dot += av * bv;
    const double d = av - bv;
    d2 += d * d;
  }

  switch (kind) {
    case KernelKind::Linear:
      return dot;
    case KernelKind::Rbf:
      return std::exp(-gamma * d2);
    case KernelKind::Polynomial:
      return std::pow(gamma * dot + 1.0, 3.0);
    case KernelKind::Sigmoid:
      return std::tanh(gamma * dot);
  }
  return dot;
}

[[nodiscard]] Series kernel_matrix(const Series& a_rows, const Series& b_rows, KernelKind kind, double gamma) {
  const Series dot = a_rows.matmul(b_rows.transpose(0, 1));
  switch (kind) {
    case KernelKind::Linear:
      return dot;
    case KernelKind::Rbf: {
      const Series a_norm = a_rows.square().sum(1, true);
      const Series b_norm = b_rows.square().sum(1, true).transpose(0, 1);
      const Series d2 = a_norm.add(b_norm).sub(dot.mul(2.0)).relu();
      return d2.mul(-gamma).exp();
    }
    case KernelKind::Polynomial:
      return dot.mul(gamma).add(1.0).pow(3.0);
    case KernelKind::Sigmoid:
      return dot.mul(gamma).tanh();
  }
  return dot;
}

class KernelRegressor final : public RegressorBase {
public:
  explicit KernelRegressor(KernelFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_kernel_regression_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "KernelRegressor::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "KernelRegressor::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<Series> y_or = to_label_vector(batch, matrix.sample_count, "KernelRegressor::learn");
    if (!y_or.ok()) return y_or.status();
    const Series y = y_or.value().reshape({matrix.sample_count, 1});

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    gamma_ = resolved_gamma(options_.gamma, feature_count_);
    lambda_ = 1.0 / options_.c;
    x_train_ = matrix.x_rows;
    Series k_series = kernel_matrix(x_train_, x_train_, options_.kernel, gamma_);
    k_series = k_series.add(pluss::eye(sample_count_, sample_count_, f64_cpu()).to(x_train_.platform()).mul(lambda_));
    const Series alpha = pluss::subsp::math::linalg::pinverse(k_series, 1e-12).matmul(y).reshape({sample_count_});
    if (!all_finite(alpha)) {
      return Status::numerical_failure("KernelRegressor::learn produced non-finite dual coefficients.");
    }

    alpha_ = alpha;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "KernelRegressor::infer");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "KernelRegressor::infer does not support NaN features; apply the fitted imputer used for learn()."));
    }

    const Series k_query = kernel_matrix(matrix.x_rows, x_train_, options_.kernel, gamma_);
    const Series pred = k_query.matmul(alpha_.reshape({sample_count_, 1})).reshape({matrix.sample_count});
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

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("KernelRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(sample_count_),
                   static_cast<double>(feature_count_),
                   gamma_,
                   lambda_}),
        x_train_,
        alpha_,
    });
  }

  Status load_state(const std::vector<Series>& state) override {
    if (state.size() < 3) return Status::invalid_argument("KernelRegressor::load_state expects 3 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 5) return Status::invalid_argument("KernelRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    gamma_ = meta[3];
    lambda_ = meta[4];
    x_train_ = state[1];
    alpha_ = state[2];
    return Status::success();
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

  KernelFamilyOptions options_{};
  bool trained_{false};
  std::int64_t sample_count_{0};
  std::int64_t feature_count_{0};
  double gamma_{1.0};
  double lambda_{1.0};
  Series x_train_ = Series::undefined();
  Series alpha_ = Series::undefined();
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

    const Series truth = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
    if (has_nan(truth)) {
      return StatusOr<Series>(Status::invalid_argument("LinearRegressor::assess requires finite labels."));
    }

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    }
    return regression_metrics(truth, pred_or.value(), metrics, options.regression_quantile);
  }

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(feature_count_), pinv_rcond_}),
        beta_,
    });
  }

  Status load_state(const std::vector<Series>& state) override {
    if (state.size() < 2) return Status::invalid_argument("LinearRegressor::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 3) return Status::invalid_argument("LinearRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    pinv_rcond_ = meta[2];
    beta_ = state[1];
    return Status::success();
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

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(features_)}),
    });
  }

  Status load_state(const std::vector<Series>& state) override {
    if (state.empty()) return Status::invalid_argument("FamilyRegressor::load_state expects metadata tensor.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 2) return Status::invalid_argument("FamilyRegressor::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    features_ = static_cast<std::int64_t>(meta[1]);
    return Status::success();
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
  if (spec.family == ModelFamily::Ensemble) {
    const auto* options = std::get_if<EnsembleFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Ensemble regressor requires EnsembleFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<EnsembleRegressor>(*options));
  }
  if (spec.family == ModelFamily::Neighbor) {
    const auto* options = std::get_if<NeighborFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Neighbor regressor requires NeighborFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<NeighborRegressor>(*options));
  }
  if (spec.family == ModelFamily::Kernel) {
    const auto* options = std::get_if<KernelFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Kernel regressor requires KernelFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<KernelRegressor>(*options));
  }
  if (spec.family == ModelFamily::Bayes) {
    const auto* options = std::get_if<BayesFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<RegressorBase>>(
          Status::invalid_argument("Bayes regressor requires BayesFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<BayesRegressor>(*options));
  }
  return StatusOr<std::unique_ptr<RegressorBase>>(std::make_unique<FamilyRegressor>(spec.family));
}

} // namespace pluss::ai::nodej
