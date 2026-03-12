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
#include <stdexcept>
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

[[nodiscard]] Config f64_on(Platform platform) {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(platform);
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

[[nodiscard]] Series prepend_intercept(const Series& x_rows) {
  const Series ones = pluss::ones({x_rows.sizes()[0], 1}, f64_on(x_rows.platform()));
  return ones.unite({x_rows}, 1);
}

[[nodiscard]] Status validate_linear_classifier_options(const LinearFamilyOptions& options) {
  if (options.objective != LinearObjective::Logistic && options.objective != LinearObjective::Hinge) {
    return Status::not_implemented("Linear classifier currently supports LinearObjective::Logistic or LinearObjective::Hinge.");
  }
  if (options.regularization != RegularizationKind::None && options.regularization != RegularizationKind::L2) {
    return Status::not_implemented("Linear classifier currently supports RegularizationKind::None or RegularizationKind::L2.");
  }
  if (!std::isfinite(options.regularization_strength) || options.regularization_strength < 0.0) {
    return Status::invalid_argument("LinearFamilyOptions.regularization_strength must be finite and >= 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_neighbor_classifier_options(const NeighborFamilyOptions& options) {
  if (options.k <= 0) {
    return Status::invalid_argument("Neighbor classifier requires NeighborFamilyOptions.k > 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_tree_classifier_options(const TreeFamilyOptions& options) {
  if (options.criterion != TreeCriterion::Gini && options.criterion != TreeCriterion::Entropy) {
    return Status::not_implemented("Tree classifier currently supports TreeCriterion::Gini or TreeCriterion::Entropy only.");
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

[[nodiscard]] Status validate_bayes_classifier_options(const BayesFamilyOptions& options) {
  if (!options.use_gaussian) {
    return Status::not_implemented("Bayes classifier currently supports gaussian likelihoods only.");
  }
  if (!std::isfinite(options.smoothing) || options.smoothing <= 0.0) {
    return Status::invalid_argument("BayesFamilyOptions.smoothing must be finite and > 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_kernel_classifier_options(const KernelFamilyOptions& options) {
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

[[nodiscard]] Status validate_ensemble_classifier_options(const EnsembleFamilyOptions& options) {
  if (options.method != EnsembleMethod::Bagging && options.method != EnsembleMethod::RandomForest) {
    return Status::not_implemented(
        "Ensemble classifier currently supports EnsembleMethod::Bagging and EnsembleMethod::RandomForest only.");
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

struct BinaryLabelEncoding {
  Series y01 = Series::undefined();
  double negative = 0.0;
  double positive = 1.0;
  ScalarType label_dtype = ScalarType::Float64;
};

[[nodiscard]] StatusOr<Series> extract_sorted_classes(const LabeledBatch& batch,
                                                      std::int64_t expected_samples,
                                                      const char* where) {
  if (!batch.y.defined()) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires a defined label vector."));
  }
  if (batch.y.numel() != expected_samples) {
    return StatusOr<Series>(Status::shape_mismatch(std::string(where) + " label count must match sample count."));
  }

  const Series y64 = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
  if (has_nan(y64)) {
    return StatusOr<Series>(
        Status::invalid_argument(std::string(where) + " does not support NaN labels."));
  }

  const Series classes_raw = y64.unique().sort(0, false, false);
  const Series classes = classes_raw.reshape({classes_raw.numel()});
  if (classes.numel() < 2) {
    return StatusOr<Series>(
        Status::invalid_argument(std::string(where) + " requires at least two distinct classes."));
  }
  return StatusOr<Series>(classes);
}

[[nodiscard]] bool same_sorted_classes(const Series& a, const Series& b) {
  if (a.numel() != b.numel()) return false;
  return a.eq(b).all().item<bool>();
}

[[nodiscard]] StatusOr<Series> classification_metrics(const Series& truth01,
                                                      const Series& labels01,
                                                      const Series& probs,
                                                      const std::vector<MetricKind>& requested);
[[nodiscard]] StatusOr<Series> classification_metrics_multiclass(const Series& truth_labels,
                                                                 const Series& pred_labels,
                                                                 const Series& score_matrix,
                                                                 const Series& classes,
                                                                 const std::vector<MetricKind>& requested);

[[nodiscard]] StatusOr<Series> resolve_parameter_bounds(const Series& raw_bounds,
                                                        std::int64_t param_count,
                                                        Platform platform,
                                                        const char* where) {
  if (!raw_bounds.defined()) return StatusOr<Series>(Series::undefined());

  Series bounds = raw_bounds.to(platform, ScalarType::Float64).reshape({raw_bounds.numel()});
  if (bounds.numel() == 1) {
    bounds = bounds.reshape({1}).expand(std::vector<std::int64_t>{param_count}).reshape({param_count});
  } else if (bounds.numel() != param_count) {
    return StatusOr<Series>(Status::shape_mismatch(
        std::string(where) + " bounds must be scalar or length equal to parameter count."));
  }

  if (!all_finite(bounds)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " bounds must be finite."));
  }
  return StatusOr<Series>(std::move(bounds));
}

[[nodiscard]] Series labels_from_scores(const Series& scores,
                                        double negative_label,
                                        double positive_label,
                                        Platform out_platform,
                                        ScalarType out_dtype);
[[nodiscard]] Series labels_from_probabilities(const Series& probs,
                                               double negative_label,
                                               double positive_label,
                                               Platform out_platform,
                                               ScalarType out_dtype);

[[nodiscard]] StatusOr<BinaryLabelEncoding> encode_binary_labels(const LabeledBatch& batch,
                                                                 std::int64_t expected_samples,
                                                                 const char* where) {
  if (!batch.y.defined()) {
    return StatusOr<BinaryLabelEncoding>(Status::invalid_argument(std::string(where) + " requires a defined label vector."));
  }
  if (batch.y.numel() != expected_samples) {
    return StatusOr<BinaryLabelEncoding>(Status::shape_mismatch(std::string(where) + " label count must match sample count."));
  }

  const Series y64 = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
  if (has_nan(y64)) {
    return StatusOr<BinaryLabelEncoding>(
        Status::invalid_argument(std::string(where) + " does not support NaN labels."));
  }

  const Series classes = y64.unique().sort(0, false, false);
  if (classes.numel() != 2) {
    return StatusOr<BinaryLabelEncoding>(
        Status::invalid_argument(std::string(where) + " currently requires exactly two distinct classes."));
  }

  const Series idx0 = pluss::make_series(std::vector<std::int64_t>{0}, {1}, i64_cpu());
  const Series idx1 = pluss::make_series(std::vector<std::int64_t>{1}, {1}, i64_cpu());
  const double negative = pluss::index(classes, std::vector<Series>{idx0}).reshape({1}).item<double>();
  const double positive = pluss::index(classes, std::vector<Series>{idx1}).reshape({1}).item<double>();
  const Series y01 = y64.eq(positive).to(ScalarType::Float64);

  BinaryLabelEncoding out{};
  out.y01 = y01;
  out.negative = negative;
  out.positive = positive;
  out.label_dtype = batch.y.scalar_type();
  return StatusOr<BinaryLabelEncoding>(std::move(out));
}

class NeighborClassifier final : public ClassifierBase {
public:
  explicit NeighborClassifier(NeighborFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_neighbor_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "NeighborClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "NeighborClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(batch, matrix.sample_count, "NeighborClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    k_ = std::min<std::int64_t>(options_.k, sample_count_);
    if (k_ <= 0) {
      return Status::invalid_argument("NeighborClassifier requires at least one training sample.");
    }

    Series x_train = matrix.x_rows;
    if (options_.standardize) {
      x_train = standardize_fit_rows(x_train, mean_, scale_);
    } else {
      mean_ = Series::undefined();
      scale_ = Series::undefined();
    }
    x_train_ = std::move(x_train);
    y_train_ = labels.y01.reshape({sample_count_});

    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "NeighborClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "NeighborClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
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

    Series probs = Series::undefined();
    if (options_.distance_weighted) {
      constexpr double eps = 1e-12;
      const Series d2_knn = dist2.gather(1, knn_idx);
      const Series zero_mask = d2_knn.le(eps).to(ScalarType::Float64);
      const Series has_zero = zero_mask.any(1, true).to(ScalarType::Float64);
      const Series inv_dist = pluss::ones_like(d2_knn, ScalarType::Float64).div(d2_knn.sqrt().add(eps));
      const Series weights = has_zero.mul(zero_mask).add(has_zero.mul(-1.0).add(1.0).mul(inv_dist));
      const Series wsum = weights.sum(1, false);
      const Series ysum = weights.mul(y_knn).sum(1, false);
      probs = wsum.gt(0.0).where(ysum.div(wsum), pluss::zeros_like(wsum, ScalarType::Float64)).clamp(0.0, 1.0);
    } else {
      probs = y_knn.mean(1, false).clamp(0.0, 1.0);
    }

    return StatusOr<Series>(probs.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    StatusOr<Series> probs_or = infer_scores(batch, options);
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());
    return StatusOr<Series>(
        labels_from_probabilities(probs_or.value().reshape({probs_or.value().numel()}),
                                  negative_label_,
                                  positive_label_,
                                  batch.x.platform(),
                                  label_dtype_));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "NeighborClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("NeighborClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> probs_or = infer_scores(batch.features);
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(ScalarType::Float64)
                              .eq(positive_label_)
                              .to(ScalarType::Float64)
                              .reshape({pred_or.value().numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, probs_or.value(), metrics);
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
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

[[nodiscard]] double binary_class_impurity(double positive_count, double sample_count, TreeCriterion criterion) {
  if (sample_count <= 0.0) return 0.0;
  const double p = std::clamp(positive_count / sample_count, 0.0, 1.0);
  const double q = 1.0 - p;
  switch (criterion) {
    case TreeCriterion::Gini:
      return 1.0 - (p * p + q * q);
    case TreeCriterion::Entropy: {
      constexpr double eps = 1e-12;
      const double pc = std::clamp(p, eps, 1.0);
      const double qc = std::clamp(q, eps, 1.0);
      return -(p > 0.0 ? p * std::log(pc) : 0.0) - (q > 0.0 ? q * std::log(qc) : 0.0);
    }
    default:
      return 0.0;
  }
}

class TreeClassifier final : public ClassifierBase {
public:
  explicit TreeClassifier(TreeFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_tree_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "TreeClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "TreeClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(batch, matrix.sample_count, "TreeClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    x_train_ = matrix.x_rows;
    y_train_ = labels.y01.reshape({sample_count_});

    nodes_.clear();
    nodes_.reserve(static_cast<std::size_t>(2 * sample_count_ + 1));
    std::vector<std::int64_t> root_indices(static_cast<std::size_t>(sample_count_));
    std::iota(root_indices.begin(), root_indices.end(), static_cast<std::int64_t>(0));
    build_node(root_indices, 0);

    if (nodes_.empty()) {
      return Status::internal("TreeClassifier::learn failed to build any tree nodes.");
    }

    x_train_ = Series::undefined();
    y_train_ = Series::undefined();

    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "TreeClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "TreeClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }

    std::vector<double> probs{};
    probs.reserve(static_cast<std::size_t>(matrix.sample_count));

    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      std::int64_t node_idx = 0;
      while (!nodes_[static_cast<std::size_t>(node_idx)].is_leaf) {
        const TreeNode& node = nodes_[static_cast<std::size_t>(node_idx)];
        const double xv = x_at(matrix.x_rows, i, node.feature);
        node_idx = xv < node.threshold ? node.left : node.right;
      }
      probs.push_back(std::clamp(nodes_[static_cast<std::size_t>(node_idx)].leaf_positive_probability, 0.0, 1.0));
    }

    Series out = pluss::make_series(probs, {static_cast<std::int64_t>(probs.size())}, f64_cpu());
    return StatusOr<Series>(out.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    StatusOr<Series> probs_or = infer_scores(batch, options);
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());
    return StatusOr<Series>(
        labels_from_probabilities(probs_or.value().reshape({probs_or.value().numel()}),
                                  negative_label_,
                                  positive_label_,
                                  batch.x.platform(),
                                  label_dtype_));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "TreeClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("TreeClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> probs_or = infer_scores(batch.features);
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(ScalarType::Float64)
                              .eq(positive_label_)
                              .to(ScalarType::Float64)
                              .reshape({pred_or.value().numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, probs_or.value(), metrics);
  }

private:
  struct TreeNode {
    bool is_leaf = true;
    std::int64_t feature = -1;
    double threshold = 0.0;
    std::int64_t left = -1;
    std::int64_t right = -1;
    double leaf_positive_probability = 0.0;
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
    double positive_count = 0.0;
    for (std::int64_t idx : indices) positive_count += y_at(idx);

    const double parent_impurity = binary_class_impurity(positive_count, static_cast<double>(n), options_.criterion);
    TreeNode node{};
    node.leaf_positive_probability = positive_count / static_cast<double>(n);

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
      double total_positive = 0.0;
      for (std::int64_t idx : indices) {
        const double yi = y_at(idx);
        total_positive += yi;
        values.emplace_back(x_at(x_train_, idx, f), yi);
      }
      std::sort(values.begin(), values.end(),
                [](const auto& a, const auto& b) {
                  if (a.first != b.first) return a.first < b.first;
                  return a.second < b.second;
                });
      if (values.front().first == values.back().first) continue;

      double left_positive = 0.0;
      for (std::int64_t i = 0; i < n - 1; ++i) {
        left_positive += values[static_cast<std::size_t>(i)].second;
        const std::int64_t left_n = i + 1;
        const std::int64_t right_n = n - left_n;
        if (left_n < options_.min_samples_leaf || right_n < options_.min_samples_leaf) continue;
        const double xi = values[static_cast<std::size_t>(i)].first;
        const double xj = values[static_cast<std::size_t>(i + 1)].first;
        if (xi == xj) continue;

        const double right_positive = total_positive - left_positive;
        const double left_impurity = binary_class_impurity(left_positive, static_cast<double>(left_n), options_.criterion);
        const double right_impurity =
            binary_class_impurity(right_positive, static_cast<double>(right_n), options_.criterion);
        const double weighted_impurity =
            (static_cast<double>(left_n) * left_impurity + static_cast<double>(right_n) * right_impurity)
            / static_cast<double>(n);
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
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

class EnsembleClassifier final : public ClassifierBase {
public:
  explicit EnsembleClassifier(EnsembleFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_ensemble_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "EnsembleClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "EnsembleClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(batch, matrix.sample_count, "EnsembleClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    const Series x_train = matrix.x_rows;
    const Series y_train = labels.y01.reshape({sample_count_});

    const std::int64_t learner_count = options_.learners;
    const std::int64_t subsample_count =
        std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(options_.subsample * sample_count_)));
    const std::int64_t mtry = options_.method == EnsembleMethod::RandomForest
                                  ? std::max<std::int64_t>(1, static_cast<std::int64_t>(std::sqrt(static_cast<double>(feature_count_))))
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
      bool trained_tree = false;
      constexpr std::int64_t max_attempts = 16;

      for (std::int64_t attempt = 0; attempt < max_attempts && !trained_tree; ++attempt) {
        std::vector<std::int64_t> row_ids(static_cast<std::size_t>(subsample_count), 0);

        for (std::int64_t i = 0; i < subsample_count; ++i) {
          row_ids[static_cast<std::size_t>(i)] = row_dist(rng);
        }

        const Series row_index = make_index_series(row_ids, x_train.platform());
        const Series x_sub = x_feature_view.index_select(0, row_index);
        const Series y_sub = y_train.index_select(0, row_index);
        const double positive_count = y_sub.sum().item<double>();
        if (positive_count <= 0.0 || positive_count >= static_cast<double>(subsample_count)) {
          continue;
        }

        const LabeledBatch sub_batch{
            .features = FeatureBatch{.x = x_sub.reshape({subsample_count, learner_features}), .layout = ObservationLayout::Rows},
            .y = y_sub.reshape({subsample_count}),
        };

        auto tree = std::make_unique<TreeClassifier>(TreeFamilyOptions{});
        Status tree_status = tree->learn(sub_batch, options);
        if (!tree_status.ok()) {
          continue;
        }

        learners_.push_back(EnsembleMember{
            .tree = std::move(tree),
            .feature_ids = feature_ids,
        });
        trained_tree = true;
      }

      if (!trained_tree) {
        return Status::internal(
            "EnsembleClassifier::learn could not draw a class-diverse bootstrap sample for a base learner.");
      }
    }

    if (learners_.empty()) {
      return Status::internal("EnsembleClassifier::learn did not build any learners.");
    }

    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "EnsembleClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "EnsembleClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }

    Series probs_sum = pluss::zeros({matrix.sample_count}, f64_cpu()).to(matrix.x_rows.platform());

    for (const EnsembleMember& member : learners_) {
      const std::int64_t learner_features = static_cast<std::int64_t>(member.feature_ids.size());
      const Series feature_index = make_index_series(member.feature_ids, matrix.x_rows.platform());
      const Series x_sub = matrix.x_rows.index_select(1, feature_index);

      const FeatureBatch sub_batch{
          .x = x_sub.reshape({matrix.sample_count, learner_features}),
          .layout = ObservationLayout::Rows,
      };
      StatusOr<Series> member_probs_or = member.tree->infer_scores(sub_batch);
      if (!member_probs_or.ok()) return StatusOr<Series>(member_probs_or.status());
      probs_sum = probs_sum.add(member_probs_or.value().to(matrix.x_rows.platform(), ScalarType::Float64));
    }

    const double denom = static_cast<double>(learners_.size());
    constexpr double eps = 1e-12;
    const Series probs = probs_sum.div(denom).clamp(eps, 1.0 - eps);
    const Series scores = probs.log().sub(probs.mul(-1.0).add(1.0).log());
    return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    const Series out =
        labels_from_scores(scores_or.value().reshape({scores_or.value().numel()}), negative_label_, positive_label_, batch.x.platform(), label_dtype_);
    return StatusOr<Series>(out);
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "EnsembleClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("EnsembleClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> scores_or = infer_scores(batch.features);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    const Series prob_series = scores_or.value().sigmoid();

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(ScalarType::Float64)
                              .eq(positive_label_)
                              .to(ScalarType::Float64)
                              .reshape({pred_or.value().numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, prob_series, metrics);
  }

private:
  struct EnsembleMember {
    std::unique_ptr<TreeClassifier> tree{};
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
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

[[nodiscard]] Series labels_from_scores(const Series& scores,
                                        double negative_label,
                                        double positive_label,
                                        Platform out_platform,
                                        ScalarType out_dtype) {
  const Series mask = scores.ge(0.0).to(ScalarType::Float64);
  const Series labels = mask.mul(positive_label - negative_label).add(negative_label);
  return labels.to(out_platform, out_dtype);
}

[[nodiscard]] Series labels_from_probabilities(const Series& probs,
                                               double negative_label,
                                               double positive_label,
                                               Platform out_platform,
                                               ScalarType out_dtype) {
  const Series mask = probs.ge(0.5).to(ScalarType::Float64);
  const Series labels = mask.mul(positive_label - negative_label).add(negative_label);
  return labels.to(out_platform, out_dtype);
}

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

class BayesClassifier final : public ClassifierBase {
public:
  explicit BayesClassifier(BayesFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_bayes_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "BayesClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "BayesClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(batch, matrix.sample_count, "BayesClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    const Series x = matrix.x_rows;
    const Series y01 = labels.y01.reshape({sample_count_});
    const Series ycol = y01.reshape({sample_count_, 1});
    const Series one = pluss::ones({sample_count_, 1}, Config{}.with_dtype(ScalarType::Float64).with_platform(x.platform()));
    const Series mask1 = ycol;
    const Series mask0 = one.sub(mask1);

    const double n1 = mask1.sum().item<double>();
    const double n0 = mask0.sum().item<double>();
    if (n0 <= 0.0 || n1 <= 0.0) {
      return Status::invalid_argument("BayesClassifier::learn requires both classes to be present in training data.");
    }

    mean0_ = x.mul(mask0).sum(0, false).div(n0).reshape({feature_count_});
    mean1_ = x.mul(mask1).sum(0, false).div(n1).reshape({feature_count_});

    const Series d0 = x.sub(mean0_.reshape({1, feature_count_}));
    const Series d1 = x.sub(mean1_.reshape({1, feature_count_}));
    var0_ = d0.square().mul(mask0).sum(0, false).div(n0).add(options_.smoothing).reshape({feature_count_});
    var1_ = d1.square().mul(mask1).sum(0, false).div(n1).add(options_.smoothing).reshape({feature_count_});
    if (!all_finite(var0_) || var0_.le(0.0).any().item<bool>()) {
      return Status::numerical_failure("BayesClassifier::learn produced invalid variance for class 0.");
    }
    if (!all_finite(var1_) || var1_.le(0.0).any().item<bool>()) {
      return Status::numerical_failure("BayesClassifier::learn produced invalid variance for class 1.");
    }

    prior0_ = n0 / static_cast<double>(sample_count_);
    prior1_ = n1 / static_cast<double>(sample_count_);
    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "BayesClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "BayesClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }

    constexpr double two_pi = 6.28318530717958647692;
    const Series x = matrix.x_rows;
    const Series mu0 = mean0_.reshape({1, feature_count_});
    const Series mu1 = mean1_.reshape({1, feature_count_});
    const Series v0 = var0_.reshape({1, feature_count_});
    const Series v1 = var1_.reshape({1, feature_count_});

    const Series d0 = x.sub(mu0);
    const Series d1 = x.sub(mu1);
    const Series q0 = d0.square().div(v0).sum(1, false);
    const Series q1 = d1.square().div(v1).sum(1, false);
    const Series c0 = v0.mul(two_pi).log().sum(1, false).mul(-0.5).add(std::log(prior0_));
    const Series c1 = v1.mul(two_pi).log().sum(1, false).mul(-0.5).add(std::log(prior1_));
    const Series logp0 = q0.mul(-0.5).add(c0);
    const Series logp1 = q1.mul(-0.5).add(c1);
    const Series scores = logp1.sub(logp0);
    return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());

    const Series out =
        labels_from_scores(scores_or.value().reshape({scores_or.value().numel()}), negative_label_, positive_label_, batch.x.platform(), label_dtype_);
    return StatusOr<Series>(out);
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "BayesClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("BayesClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> scores_or = infer_scores(batch.features);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    const Series prob_series = scores_or.value().sigmoid();

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(ScalarType::Float64)
                              .eq(positive_label_)
                              .to(ScalarType::Float64)
                              .reshape({pred_or.value().numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, prob_series, metrics);
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
  std::int64_t sample_count_{0};
  std::int64_t feature_count_{0};
  Series mean0_ = Series::undefined();
  Series mean1_ = Series::undefined();
  Series var0_ = Series::undefined();
  Series var1_ = Series::undefined();
  double prior0_{0.5};
  double prior1_{0.5};
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

class KernelClassifier final : public ClassifierBase {
public:
  explicit KernelClassifier(KernelFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_kernel_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "KernelClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "KernelClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(batch, matrix.sample_count, "KernelClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    gamma_ = resolved_gamma(options_.gamma, feature_count_);
    lambda_ = 1.0 / options_.c;
    x_train_ = matrix.x_rows;
    Series k_series = kernel_matrix(x_train_, x_train_, options_.kernel, gamma_);
    k_series = k_series.add(pluss::eye(sample_count_, sample_count_, f64_cpu()).to(x_train_.platform()).mul(lambda_));
    const Series y_pm1 = labels.y01.mul(2.0).sub(1.0).reshape({sample_count_, 1});
    const Series alpha = pluss::subsp::math::linalg::pinverse(k_series, 1e-12).matmul(y_pm1).reshape({sample_count_});
    if (!all_finite(alpha)) {
      return Status::numerical_failure("KernelClassifier::learn produced non-finite dual coefficients.");
    }

    alpha_ = alpha;
    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "KernelClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "KernelClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }

    const Series k_query = kernel_matrix(matrix.x_rows, x_train_, options_.kernel, gamma_);
    const Series scores = k_query.matmul(alpha_.reshape({sample_count_, 1})).reshape({matrix.sample_count});
    return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());

    const Series out =
        labels_from_scores(scores_or.value().reshape({scores_or.value().numel()}), negative_label_, positive_label_, batch.x.platform(), label_dtype_);
    return StatusOr<Series>(out);
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "KernelClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("KernelClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> scores_or = infer_scores(batch.features);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    const Series prob_series = scores_or.value().sigmoid();

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(ScalarType::Float64)
                              .eq(positive_label_)
                              .to(ScalarType::Float64)
                              .reshape({pred_or.value().numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, prob_series, metrics);
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
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

[[nodiscard]] Series logistic_gradient(const Series& design,
                                       const Series& y01,
                                       const Series& weights,
                                       bool fit_intercept,
                                       double lambda) {
  const std::int64_t n = design.sizes()[0];
  const Series logits = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series probs = logits.sigmoid();
  const Series error = probs.sub(y01);
  Series grad = design.transpose(0, 1).matmul(error.reshape({n, 1})).div(static_cast<double>(n)).reshape({weights.numel()});

  if (lambda > 0.0) {
    Series ridge = weights;
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, ridge.platform());
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::zeros({1}, f64_on(ridge.platform())), false);
    }
    grad = grad.add(ridge.mul(lambda));
  }
  return grad;
}

[[nodiscard]] double logistic_objective(const Series& design,
                                        const Series& y01,
                                        const Series& weights,
                                        bool fit_intercept,
                                        double lambda) {
  const std::int64_t n = design.sizes()[0];
  const Series logits = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series loss_terms = logits.softplus().sub(y01.mul(logits));
  double loss = loss_terms.mean().item<double>();

  if (lambda > 0.0) {
    Series ridge = weights;
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, ridge.platform());
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::zeros({1}, f64_on(ridge.platform())), false);
    }
    loss += 0.5 * lambda * ridge.square().sum().item<double>();
  }
  return loss;
}

[[nodiscard]] Series hinge_gradient(const Series& design,
                                    const Series& y01,
                                    const Series& weights,
                                    bool fit_intercept,
                                    double lambda) {
  const std::int64_t n = design.sizes()[0];
  const Series scores = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series ypm = y01.mul(2.0).sub(1.0);
  const Series margin = ypm.mul(scores).mul(-1.0).add(1.0);
  const Series active = margin.gt(0.0).to(ScalarType::Float64);
  const Series coeff = ypm.mul(active).mul(-1.0);
  Series grad = design.transpose(0, 1).matmul(coeff.reshape({n, 1})).div(static_cast<double>(n)).reshape({weights.numel()});

  if (lambda > 0.0) {
    Series ridge = weights;
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, ridge.platform());
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::zeros({1}, f64_on(ridge.platform())), false);
    }
    grad = grad.add(ridge.mul(lambda));
  }
  return grad;
}

[[nodiscard]] double hinge_objective(const Series& design,
                                     const Series& y01,
                                     const Series& weights,
                                     bool fit_intercept,
                                     double lambda) {
  const std::int64_t n = design.sizes()[0];
  const Series scores = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series ypm = y01.mul(2.0).sub(1.0);
  const Series margin = ypm.mul(scores).mul(-1.0).add(1.0);
  double loss = margin.relu().mean().item<double>();

  if (lambda > 0.0) {
    Series ridge = weights;
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, ridge.platform());
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::zeros({1}, f64_on(ridge.platform())), false);
    }
    loss += 0.5 * lambda * ridge.square().sum().item<double>();
  }
  return loss;
}

[[nodiscard]] StatusOr<Series> classification_metrics(const Series& truth01,
                                                      const Series& labels01,
                                                      const Series& probs,
                                                      const std::vector<MetricKind>& requested) {
  const Series y = truth01.to(ScalarType::Float64).reshape({truth01.numel()});
  const Series yh = labels01.to(ScalarType::Float64).reshape({labels01.numel()});
  const Series p = probs.to(ScalarType::Float64).reshape({probs.numel()});

  if (y.numel() != yh.numel() || y.numel() != p.numel() || y.numel() == 0) {
    return StatusOr<Series>(Status::shape_mismatch("assess classification metrics require equal non-empty vectors."));
  }

  constexpr double eps = 1e-12;
  const Series yb = y.ge(0.5).to(ScalarType::Float64);
  const Series yhb = yh.ge(0.5).to(ScalarType::Float64);
  const Series clipped = p.clamp(eps, 1.0 - eps);

  const Series tp = yb.mul(yhb).sum();
  const Series tn = yb.mul(-1.0).add(1.0).mul(yhb.mul(-1.0).add(1.0)).sum();
  const Series fp = yb.mul(-1.0).add(1.0).mul(yhb).sum();
  const Series fn = yb.mul(yhb.mul(-1.0).add(1.0)).sum();
  const Series accuracy = yb.eq(yhb).to(ScalarType::Float64).mean();
  const Series tpr_den = tp.add(fn);
  const Series tnr_den = tn.add(fp);
  const Series zero = pluss::zeros_like(tpr_den, ScalarType::Float64);
  const Series tpr = tpr_den.gt(0.0).where(tp.div(tpr_den), zero);
  const Series tnr = tnr_den.gt(0.0).where(tn.div(tnr_den), zero);
  const Series bal_accuracy = tpr.add(tnr).mul(0.5);
  const Series precision_den = tp.add(fp);
  const Series precision = precision_den.gt(0.0).where(tp.div(precision_den), zero);
  const Series recall = tpr;
  const Series f1_den = precision.add(recall);
  const Series f1 = f1_den.gt(0.0).where(precision.mul(recall).mul(2.0).div(f1_den), zero);
  const Series logloss = yb.mul(clipped.log()).add(yb.mul(-1.0).add(1.0).mul(clipped.mul(-1.0).add(1.0).log())).mul(-1.0).mean();

  const Series pos_count = yb.sum();
  const Series neg_count = yb.mul(-1.0).add(1.0).sum();
  const Series has_both = pos_count.gt(0.0).to(ScalarType::Float64).mul(neg_count.gt(0.0).to(ScalarType::Float64));
  const Series pos_safe = pos_count.clamp_min(1.0);
  const Series neg_safe = neg_count.clamp_min(1.0);
  const std::int64_t n = p.numel();
  const Platform platform = p.platform();
  const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
  const Series order = p.argsort(0, true, true);
  const Series p_sorted = p.index_select(0, order);
  const Series y_sorted = yb.index_select(0, order);
  const Series one_vec = pluss::ones({n}, f64_on(platform));
  const Series tp_cum = y_sorted.cumsum(0, false);
  const Series fp_cum = one_vec.sub(y_sorted).cumsum(0, false);

  Series idx_next = pluss::arange(1, n, i64_cfg);
  if (idx_next.numel() == 0) {
    idx_next = pluss::make_series(std::vector<std::int64_t>{0}, {1}, i64_cfg);
  }
  Series p_next = p_sorted.index_select(0, idx_next).reshape({idx_next.numel()});
  if (n == 1) {
    p_next = p_sorted.add(1.0);
  } else {
    p_next = p_next.unite({p_sorted.reshape({n}).index_select(0, pluss::arange(n - 1, n, i64_cfg)).add(1.0)}, 0);
  }
  const Series boundary = p_sorted.ne(p_next).reshape({n});
  const Series tpr_curve = tp_cum.div(pos_safe);
  const Series fpr_curve = fp_cum.div(neg_safe);
  const Series tpr_u = tpr_curve.masked_select(boundary);
  const Series fpr_u = fpr_curve.masked_select(boundary);
  const std::int64_t m = fpr_u.numel();
  const Series tpr_prev = (m > 1)
                              ? pluss::zeros({1}, f64_on(platform)).unite(
                                    {tpr_u.index_select(0, pluss::arange(0, m - 1, i64_cfg))},
                                    0)
                              : pluss::zeros({1}, f64_on(platform));
  const Series fpr_prev = (m > 1)
                              ? pluss::zeros({1}, f64_on(platform)).unite(
                                    {fpr_u.index_select(0, pluss::arange(0, m - 1, i64_cfg))},
                                    0)
                              : pluss::zeros({1}, f64_on(platform));
  const Series roc_auc_raw = fpr_u.sub(fpr_prev).mul(tpr_u.add(tpr_prev)).mul(0.5).sum().clamp(0.0, 1.0).reshape({1});
  const Series roc_auc = has_both.gt(0.5).where(roc_auc_raw, pluss::ones({1}, f64_on(platform)).mul(0.5));

  std::vector<Series> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Accuracy:
        out.push_back(accuracy.reshape({1}));
        break;
      case MetricKind::BalancedAccuracy:
        out.push_back(bal_accuracy.reshape({1}));
        break;
      case MetricKind::Precision:
        out.push_back(precision.reshape({1}));
        break;
      case MetricKind::Recall:
        out.push_back(recall.reshape({1}));
        break;
      case MetricKind::F1:
        out.push_back(f1.reshape({1}));
        break;
      case MetricKind::RocAuc:
        out.push_back(roc_auc.reshape({1}));
        break;
      case MetricKind::LogLoss:
        out.push_back(logloss.reshape({1}));
        break;
      default:
        return StatusOr<Series>(Status::invalid_argument("Unsupported classification metric in assess()."));
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

[[nodiscard]] StatusOr<Series> classification_metrics_multiclass(const Series& truth_labels,
                                                                 const Series& pred_labels,
                                                                 const Series& score_matrix,
                                                                 const Series& classes,
                                                                 const std::vector<MetricKind>& requested) {
  const Series y = truth_labels.to(ScalarType::Float64).reshape({truth_labels.numel()});
  const Series yh = pred_labels.to(ScalarType::Float64).reshape({pred_labels.numel()});
  const Series cls = classes.to(ScalarType::Float64).reshape({classes.numel()});
  const std::int64_t n = y.numel();
  const std::int64_t k = cls.numel();
  const Platform platform = y.platform();
  if (n == 0 || yh.numel() != n || k < 2) {
    return StatusOr<Series>(Status::shape_mismatch("multiclass assess requires non-empty equal-size label vectors."));
  }

  Series logits = score_matrix.to(platform, ScalarType::Float64);
  if (logits.dim() != 2 || logits.sizes()[0] != n || logits.sizes()[1] != k) {
    return StatusOr<Series>(Status::shape_mismatch("multiclass assess expects score matrix shape [samples, classes]."));
  }

  const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(platform);
  const Series k_scalar = pluss::ones({1}, f64_on(platform)).mul(static_cast<double>(k));
  Series precision_sum = pluss::zeros({1}, f64_on(platform));
  Series recall_sum = pluss::zeros({1}, f64_on(platform));
  Series f1_sum = pluss::zeros({1}, f64_on(platform));

  for (std::int64_t ci = 0; ci < k; ++ci) {
    const Series idx = pluss::arange(ci, ci + 1, i64_cfg);
    const Series cls_value = cls.index_select(0, idx).reshape({1});
    const Series y_c = y.eq(cls_value).to(ScalarType::Float64);
    const Series yh_c = yh.eq(cls_value).to(ScalarType::Float64);
    const Series tp = y_c.mul(yh_c).sum();
    const Series fp = y_c.mul(-1.0).add(1.0).mul(yh_c).sum();
    const Series fn = y_c.mul(yh_c.mul(-1.0).add(1.0)).sum();
    const Series zero = pluss::zeros_like(tp, ScalarType::Float64);
    const Series precision_den = tp.add(fp);
    const Series recall_den = tp.add(fn);
    const Series precision = precision_den.gt(0.0).where(tp.div(precision_den), zero);
    const Series recall = recall_den.gt(0.0).where(tp.div(recall_den), zero);
    const Series f1_den = precision.add(recall);
    const Series f1 = f1_den.gt(0.0).where(precision.mul(recall).mul(2.0).div(f1_den), zero);
    precision_sum = precision_sum.add(precision.reshape({1}));
    recall_sum = recall_sum.add(recall.reshape({1}));
    f1_sum = f1_sum.add(f1.reshape({1}));
  }

  const Series accuracy = y.eq(yh).to(ScalarType::Float64).mean().reshape({1});
  const Series macro_precision = precision_sum.div(k_scalar).reshape({1});
  const Series macro_recall = recall_sum.div(k_scalar).reshape({1});
  const Series macro_f1 = f1_sum.div(k_scalar).reshape({1});
  const Series balanced_accuracy = macro_recall.reshape({1});

  constexpr double eps = 1e-12;
  const Series probs = logits.softmax(1);
  Series one_hot = Series::undefined();
  for (std::int64_t ci = 0; ci < k; ++ci) {
    const Series idx = pluss::arange(ci, ci + 1, i64_cfg);
    const Series cls_value = cls.index_select(0, idx).reshape({1});
    const Series col = y.eq(cls_value).to(ScalarType::Float64).reshape({n, 1});
    one_hot = one_hot.defined() ? one_hot.unite({col}, 1) : col;
  }
  const Series logloss = probs.mul(one_hot).sum(1, false).clamp(eps, 1.0).log().mul(-1.0).mean().reshape({1});

  std::vector<Series> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Accuracy:
        out.push_back(accuracy);
        break;
      case MetricKind::BalancedAccuracy:
        out.push_back(balanced_accuracy);
        break;
      case MetricKind::Precision:
        out.push_back(macro_precision);
        break;
      case MetricKind::Recall:
        out.push_back(macro_recall);
        break;
      case MetricKind::F1:
        out.push_back(macro_f1);
        break;
      case MetricKind::LogLoss:
        out.push_back(logloss);
        break;
      case MetricKind::RocAuc:
        return StatusOr<Series>(
            Status::not_implemented("multiclass RocAuc is not implemented yet; use one-vs-rest externally if needed."));
      default:
        return StatusOr<Series>(Status::invalid_argument("Unsupported classification metric in multiclass assess()."));
    }
  }

  if (out.empty()) {
    return StatusOr<Series>(pluss::make_series(std::vector<double>{}, {0}, f64_on(platform)));
  }
  Series merged = out.front();
  for (std::size_t i = 1; i < out.size(); ++i) {
    merged = merged.unite({out[i]}, 0);
  }
  return StatusOr<Series>(merged.reshape({static_cast<std::int64_t>(out.size())}).to(platform, ScalarType::Float64));
}

class LinearClassifier final : public ClassifierBase {
public:
  explicit LinearClassifier(LinearFamilyOptions options) : options_(options) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    Status option_status = validate_linear_classifier_options(options_);
    if (!option_status.ok()) return option_status;

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "LinearClassifier::learn");
    if (!matrix_or.ok()) return matrix_or.status();
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return Status::invalid_argument(
          "LinearClassifier::learn does not support NaN features; apply a nodej::prep imputer before learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(batch, matrix.sample_count, "LinearClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();

    Series design = matrix.x_rows;
    if (options_.fit_intercept) {
      design = prepend_intercept(design);
    }

    const std::int64_t param_count = design.sizes()[1];
    Series weights = pluss::zeros({param_count}, f64_on(design.platform()));
    const double lambda =
        options_.regularization == RegularizationKind::L2 ? options_.regularization_strength : 0.0;

    optim::AdamState adam = optim::adam_state_like(std::vector<Series>{weights});
    optim::AdamOptions adam_opt{};
    adam_opt.learning_rate = 0.05;
    adam_opt.weight_decay = 0.0;

    const bool use_parameter_bounds = options.parameter_lower_bounds.defined() || options.parameter_upper_bounds.defined();
    optim::MinimaxConstraints minimax_constraints{};
    optim::MinimaxOptions minimax_options{};
    optim::MinimaxState minimax_state{};

    if (use_parameter_bounds) {
      StatusOr<Series> lb_or =
          resolve_parameter_bounds(options.parameter_lower_bounds, param_count, design.platform(), "LinearClassifier::learn lower");
      if (!lb_or.ok()) return lb_or.status();
      StatusOr<Series> ub_or =
          resolve_parameter_bounds(options.parameter_upper_bounds, param_count, design.platform(), "LinearClassifier::learn upper");
      if (!ub_or.ok()) return ub_or.status();

      Series lower = lb_or.move_value();
      Series upper = ub_or.move_value();
      if (lower.defined() && upper.defined() && !lower.le(upper).all().item<bool>()) {
        return Status::invalid_argument("LinearClassifier::learn lower bounds must be <= upper bounds.");
      }

      if (lower.defined()) minimax_constraints.lbs = {lower};
      if (upper.defined()) minimax_constraints.ubs = {upper};
      minimax_options.adam = adam_opt;
      minimax_options.project_bounds = true;
      minimax_options.enable_constraint_autodiff = false;
      minimax_state = optim::minimax_state_like(std::vector<Series>{weights}, minimax_constraints);
    }

    double prev_loss = std::numeric_limits<double>::infinity();
    for (std::int64_t iter = 0; iter < options.max_iterations; ++iter) {
      Series grad = Series::undefined();
      if (options_.objective == LinearObjective::Logistic) {
        grad = logistic_gradient(design, labels.y01, weights, options_.fit_intercept, lambda);
      } else {
        grad = hinge_gradient(design, labels.y01, weights, options_.fit_intercept, lambda);
      }
      if (!all_finite(grad)) {
        return Status::numerical_failure("LinearClassifier::learn encountered non-finite gradient.");
      }

      std::vector<Series> params{weights};
      try {
        if (use_parameter_bounds) {
          optim::minimax_step_inplace(params, std::vector<Series>{grad}, minimax_state, minimax_options, minimax_constraints);
        } else {
          optim::adam_step_inplace(params, std::vector<Series>{grad}, adam, adam_opt);
        }
      } catch (const std::exception& ex) {
        return Status::numerical_failure(std::string("LinearClassifier::learn optimizer failure: ") + ex.what());
      }
      weights = params[0];

      const double loss = options_.objective == LinearObjective::Logistic
                              ? logistic_objective(design, labels.y01, weights, options_.fit_intercept, lambda)
                              : hinge_objective(design, labels.y01, weights, options_.fit_intercept, lambda);
      if (!std::isfinite(loss)) {
        return Status::numerical_failure("LinearClassifier::learn encountered non-finite objective.");
      }
      if (std::abs(prev_loss - loss) <= options.tolerance) {
        break;
      }
      prev_loss = loss;
    }

    if (!all_finite(weights)) {
      return Status::numerical_failure("LinearClassifier::learn produced non-finite coefficients.");
    }

    weights_ = std::move(weights);
    feature_count_ = matrix.feature_count;
    negative_label_ = labels.negative;
    positive_label_ = labels.positive;
    label_dtype_ = labels.label_dtype;
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "LinearClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "LinearClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }

    Series design = matrix.x_rows;
    if (options_.fit_intercept) {
      design = prepend_intercept(design);
    }

    const Series scores = design.matmul(weights_.reshape({weights_.numel(), 1})).reshape({matrix.sample_count});
    if (!all_finite(scores)) {
      return StatusOr<Series>(Status::numerical_failure("LinearClassifier::infer_scores produced non-finite scores."));
    }
    return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());

    const Series out =
        labels_from_scores(scores_or.value().reshape({scores_or.value().numel()}), negative_label_, positive_label_, batch.x.platform(), label_dtype_);
    return StatusOr<Series>(out);
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    StatusOr<BinaryLabelEncoding> truth_or = encode_binary_labels(batch, batch.features.sample_count(), "LinearClassifier::assess");
    if (!truth_or.ok()) return StatusOr<Series>(truth_or.status());
    const BinaryLabelEncoding truth = truth_or.move_value();
    if (truth.negative != negative_label_ || truth.positive != positive_label_) {
      return StatusOr<Series>(
          Status::invalid_argument("LinearClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> scores_or = infer_scores(batch.features);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    const Series probs = scores_or.value().sigmoid();

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(ScalarType::Float64)
                              .eq(positive_label_)
                              .to(ScalarType::Float64)
                              .reshape({pred_or.value().numel()});

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) {
      metrics = {MetricKind::Accuracy};
    }
    return classification_metrics(truth.y01, pred01, probs, metrics);
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
  Series weights_ = Series::undefined();
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

class FamilyClassifier final : public ClassifierBase {
public:
  explicit FamilyClassifier(ModelFamily family) : family_(family) {}

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
        Status::not_implemented(std::string(family_name(family_)) + " classifier infer() is scheduled after Phase 0."));
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    (void)options;
    Status ready_status = validate_ready(batch);
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));
    return StatusOr<Series>(Status::not_implemented(std::string(family_name(family_))
                                                    + " classifier infer_scores() is scheduled after Phase 0."));
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
        Status::not_implemented(std::string(family_name(family_)) + " classifier assess() is scheduled after Phase 0."));
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

[[nodiscard]] StatusOr<std::unique_ptr<ClassifierBase>> make_binary_classifier_impl(const ModelSpec& spec) {
  if (spec.family == ModelFamily::Linear) {
    const auto* options = std::get_if<LinearFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Linear classifier requires LinearFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<LinearClassifier>(*options));
  }
  if (spec.family == ModelFamily::Tree) {
    const auto* options = std::get_if<TreeFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Tree classifier requires TreeFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<TreeClassifier>(*options));
  }
  if (spec.family == ModelFamily::Ensemble) {
    const auto* options = std::get_if<EnsembleFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Ensemble classifier requires EnsembleFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<EnsembleClassifier>(*options));
  }
  if (spec.family == ModelFamily::Neighbor) {
    const auto* options = std::get_if<NeighborFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Neighbor classifier requires NeighborFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<NeighborClassifier>(*options));
  }
  if (spec.family == ModelFamily::Bayes) {
    const auto* options = std::get_if<BayesFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Bayes classifier requires BayesFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<BayesClassifier>(*options));
  }
  if (spec.family == ModelFamily::Kernel) {
    const auto* options = std::get_if<KernelFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Kernel classifier requires KernelFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<KernelClassifier>(*options));
  }
  return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<FamilyClassifier>(spec.family));
}

class OvRClassifier final : public ClassifierBase {
public:
  explicit OvRClassifier(ModelSpec spec) : spec_(std::move(spec)) {}

  [[nodiscard]] bool trained() const noexcept override { return trained_; }

  Status learn(const LabeledBatch& batch, const TrainOptions& options = {}) override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return data_status;
    if (options.max_iterations <= 0) return Status::invalid_argument("TrainOptions.max_iterations must be positive.");
    if (!(options.tolerance > 0.0)) return Status::invalid_argument("TrainOptions.tolerance must be positive.");

    StatusOr<Series> classes_or = extract_sorted_classes(batch, batch.features.sample_count(), "OvRClassifier::learn");
    if (!classes_or.ok()) return classes_or.status();
    const Series classes = classes_or.move_value();
    const std::int64_t class_count = classes.numel();
    const std::int64_t sample_count = batch.features.sample_count();
    const Config i64_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(classes.platform());

    std::vector<std::unique_ptr<ClassifierBase>> models{};
    models.reserve(static_cast<std::size_t>(class_count));

    if (class_count == 2) {
      StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_binary_classifier_impl(spec_);
      if (!model_or.ok()) return model_or.status();
      std::unique_ptr<ClassifierBase> model = model_or.move_value();
      Status learn_status = model->learn(batch, options);
      if (!learn_status.ok()) return learn_status;
      models.push_back(std::move(model));
      multiclass_ = false;
    } else {
      const Series y64 = batch.y.to(ScalarType::Float64).reshape({sample_count});
      for (std::int64_t ci = 0; ci < class_count; ++ci) {
        const Series idx = pluss::arange(ci, ci + 1, i64_cfg);
        const Series cls_value = classes.index_select(0, idx).reshape({1});
        const Series y01 = y64.eq(cls_value).to(ScalarType::Float64);
        const LabeledBatch binary_batch{
            .features = batch.features,
            .y = y01,
        };
        StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_binary_classifier_impl(spec_);
        if (!model_or.ok()) return model_or.status();
        std::unique_ptr<ClassifierBase> model = model_or.move_value();
        Status learn_status = model->learn(binary_batch, options);
        if (!learn_status.ok()) return learn_status;
        models.push_back(std::move(model));
      }
      multiclass_ = true;
    }

    models_ = std::move(models);
    classes_ = classes.to(ScalarType::Float64).reshape({class_count});
    label_dtype_ = batch.y.scalar_type();
    feature_count_ = batch.features.feature_count();
    trained_ = true;
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer_scores(const FeatureBatch& batch,
                                              const InferOptions& options = {}) const override {
    Status ready_status = validate_ready(batch, "infer_scores");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    if (!multiclass_) {
      return models_.front()->infer_scores(batch, options);
    }

    const std::int64_t sample_count = batch.sample_count();
    Series scores = Series::undefined();
    for (const std::unique_ptr<ClassifierBase>& model : models_) {
      StatusOr<Series> score_or = model->infer_scores(batch, options);
      if (!score_or.ok()) return StatusOr<Series>(score_or.status());
      const Series col = score_or.value().to(batch.x.platform(), ScalarType::Float64).reshape({sample_count, 1});
      scores = scores.defined() ? scores.unite({col}, 1) : col;
    }
    if (!scores.defined()) {
      return StatusOr<Series>(Status::internal("OvRClassifier::infer_scores has no trained learners."));
    }
    return StatusOr<Series>(scores.reshape({sample_count, static_cast<std::int64_t>(models_.size())}));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status ready_status = validate_ready(batch, "infer");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    if (!multiclass_) {
      return models_.front()->infer(batch, options);
    }

    StatusOr<Series> score_or = infer_scores(batch, options);
    if (!score_or.ok()) return StatusOr<Series>(score_or.status());
    const Series scores = score_or.value().reshape({batch.sample_count(), classes_.numel()});
    const Series idx = scores.argmax(1, false).reshape({batch.sample_count()}).to(ScalarType::Int64);
    const Series labels = classes_.to(scores.platform(), ScalarType::Float64).index_select(0, idx);
    return StatusOr<Series>(labels.to(batch.x.platform(), label_dtype_).reshape({batch.sample_count()}));
  }

  [[nodiscard]] StatusOr<Series> assess(const LabeledBatch& batch,
                                        const AssessOptions& options = {}) const override {
    Status data_status = validate_labeled(batch);
    if (!data_status.ok()) return StatusOr<Series>(std::move(data_status));

    Status ready_status = validate_ready(batch.features, "assess");
    if (!ready_status.ok()) return StatusOr<Series>(std::move(ready_status));

    if (!multiclass_) {
      return models_.front()->assess(batch, options);
    }

    StatusOr<Series> classes_or = extract_sorted_classes(batch, batch.features.sample_count(), "OvRClassifier::assess");
    if (!classes_or.ok()) return StatusOr<Series>(classes_or.status());
    if (!same_sorted_classes(classes_or.value().to(ScalarType::Float64), classes_)) {
      return StatusOr<Series>(Status::invalid_argument(
          "OvRClassifier::assess class labels must match classes seen during learn()."));
    }

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    StatusOr<Series> scores_or = infer_scores(batch.features);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());

    std::vector<MetricKind> metrics = options.metrics;
    if (metrics.empty()) metrics = {MetricKind::Accuracy};
    return classification_metrics_multiclass(batch.y, pred_or.value(), scores_or.value(), classes_, metrics);
  }

private:
  [[nodiscard]] Status validate_ready(const FeatureBatch& batch, const char* where) const {
    Status data_status = validate_features(batch);
    if (!data_status.ok()) return data_status;
    if (!trained_) return Status::precondition(std::string("Estimator must be learned before ") + where + "().");
    if (batch.feature_count() != feature_count_) {
      return Status::shape_mismatch("Feature count mismatch between learn and inference batch.");
    }
    if (models_.empty()) {
      return Status::precondition("Estimator must hold at least one trained learner.");
    }
    return Status::success();
  }

  ModelSpec spec_{};
  bool trained_{false};
  bool multiclass_{false};
  std::int64_t feature_count_{0};
  ScalarType label_dtype_{ScalarType::Float64};
  Series classes_ = Series::undefined();
  std::vector<std::unique_ptr<ClassifierBase>> models_{};
};

} // namespace

StatusOr<std::unique_ptr<ClassifierBase>> make_classifier(const ModelSpec& spec) {
  if (spec.task != TaskKind::Classification) {
    return StatusOr<std::unique_ptr<ClassifierBase>>(
        Status::invalid_argument("make_classifier requires ModelSpec.task = TaskKind::Classification."));
  }
  if (!is_phase0_family(spec.family)) {
    return StatusOr<std::unique_ptr<ClassifierBase>>(Status::invalid_argument("Unsupported classifier family."));
  }
  StatusOr<std::unique_ptr<ClassifierBase>> check_or = make_binary_classifier_impl(spec);
  if (!check_or.ok()) return StatusOr<std::unique_ptr<ClassifierBase>>(check_or.status());
  return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<OvRClassifier>(spec));
}

} // namespace pluss::ai::nodej
