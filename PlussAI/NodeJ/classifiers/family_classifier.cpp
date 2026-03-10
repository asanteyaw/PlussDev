#include "pluss/ai/nodej/builders.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
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

[[nodiscard]] Series prepend_intercept(const Series& x_rows) {
  const Series ones = pluss::ones({x_rows.sizes()[0], 1}, f64_cpu());
  return ones.unite({x_rows}, 1);
}

[[nodiscard]] Status validate_linear_classifier_options(const LinearFamilyOptions& options) {
  if (options.objective != LinearObjective::Logistic) {
    return Status::not_implemented("Linear classifier currently supports LinearObjective::Logistic only.");
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

struct BinaryLabelEncoding {
  Series y01 = Series::undefined();
  double negative = 0.0;
  double positive = 1.0;
  ScalarType label_dtype = ScalarType::Float64;
};

[[nodiscard]] StatusOr<Series> classification_metrics(const Series& truth01,
                                                      const Series& labels01,
                                                      const Series& probs,
                                                      const std::vector<MetricKind>& requested);

[[nodiscard]] StatusOr<BinaryLabelEncoding> encode_binary_labels(const LabeledBatch& batch,
                                                                 std::int64_t expected_samples,
                                                                 const char* where) {
  if (!batch.y.defined()) {
    return StatusOr<BinaryLabelEncoding>(Status::invalid_argument(std::string(where) + " requires a defined label vector."));
  }
  if (batch.y.numel() != expected_samples) {
    return StatusOr<BinaryLabelEncoding>(Status::shape_mismatch(std::string(where) + " label count must match sample count."));
  }

  const Series y64 = batch.y.to(pluss::pCPU, ScalarType::Float64).reshape({batch.y.numel()});
  if (has_nan(y64)) {
    return StatusOr<BinaryLabelEncoding>(
        Status::invalid_argument(std::string(where) + " does not support NaN labels."));
  }

  std::vector<double> labels = y64.to_vec<double>();
  std::sort(labels.begin(), labels.end());
  labels.erase(std::unique(labels.begin(), labels.end()), labels.end());
  if (labels.size() != 2) {
    return StatusOr<BinaryLabelEncoding>(
        Status::invalid_argument(std::string(where) + " currently requires exactly two distinct classes."));
  }

  const double negative = labels[0];
  const double positive = labels[1];
  std::vector<double> y01_vec{};
  y01_vec.reserve(static_cast<std::size_t>(y64.numel()));
  for (double v : y64.to_vec<double>()) {
    y01_vec.push_back(v == positive ? 1.0 : 0.0);
  }

  BinaryLabelEncoding out{};
  out.y01 = pluss::make_series(y01_vec, {static_cast<std::int64_t>(y01_vec.size())}, f64_cpu());
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

    x_train_ = matrix.x_rows.to_vec<double>();
    y_train_ = labels.y01.to_vec<double>();
    if (static_cast<std::int64_t>(y_train_.size()) != matrix.sample_count) {
      return Status::internal("NeighborClassifier::learn internal label size mismatch.");
    }

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    k_ = std::min<std::int64_t>(options_.k, sample_count_);
    if (k_ <= 0) {
      return Status::invalid_argument("NeighborClassifier requires at least one training sample.");
    }

    if (options_.standardize) {
      standardize_fit(x_train_, sample_count_, feature_count_, mean_, scale_);
    } else {
      mean_.clear();
      scale_.clear();
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

    StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "NeighborClassifier::infer_scores");
    if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
    DenseFeatureMatrix matrix = matrix_or.move_value();
    if (has_nan(matrix.x_rows)) {
      return StatusOr<Series>(Status::invalid_argument(
          "NeighborClassifier::infer_scores does not support NaN features; apply the fitted imputer used for learn()."));
    }

    std::vector<double> x_query = matrix.x_rows.to_vec<double>();
    if (options_.standardize) {
      standardize_apply(x_query, matrix.sample_count, feature_count_, mean_, scale_);
    }

    std::vector<double> probs{};
    probs.reserve(static_cast<std::size_t>(matrix.sample_count));
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

      double p1 = 0.0;
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
        p1 = wsum > 0.0 ? (ysum / wsum) : 0.0;
      } else {
        double sum = 0.0;
        for (std::int64_t i = 0; i < neighbor_count; ++i) {
          const auto idx = dist_idx[static_cast<std::size_t>(i)].second;
          sum += y_train_[static_cast<std::size_t>(idx)];
        }
        p1 = sum / static_cast<double>(neighbor_count);
      }
      probs.push_back(std::clamp(p1, 0.0, 1.0));
    }

    Series out = pluss::make_series(probs, {static_cast<std::int64_t>(probs.size())}, f64_cpu());
    return StatusOr<Series>(out.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    StatusOr<Series> probs_or = infer_scores(batch, options);
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());

    const std::vector<double> probs = probs_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::vector<double> labels{};
    labels.reserve(probs.size());
    for (double p : probs) {
      labels.push_back(p >= 0.5 ? positive_label_ : negative_label_);
    }
    Series out = pluss::make_series(labels, {static_cast<std::int64_t>(labels.size())}, f64_cpu());
    out = out.to(batch.x.platform(), label_dtype_);
    return StatusOr<Series>(std::move(out));
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
                              .to(pluss::pCPU, ScalarType::Float64)
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
  std::vector<double> x_train_{};
  std::vector<double> y_train_{};
  std::vector<double> mean_{};
  std::vector<double> scale_{};
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
    x_train_ = matrix.x_rows.to_vec<double>();
    y_train_ = labels.y01.to_vec<double>();
    if (static_cast<std::int64_t>(y_train_.size()) != sample_count_) {
      return Status::internal("TreeClassifier::learn internal label size mismatch.");
    }

    nodes_.clear();
    nodes_.reserve(static_cast<std::size_t>(2 * sample_count_ + 1));
    std::vector<std::int64_t> root_indices(static_cast<std::size_t>(sample_count_));
    std::iota(root_indices.begin(), root_indices.end(), static_cast<std::int64_t>(0));
    build_node(root_indices, 0);

    if (nodes_.empty()) {
      return Status::internal("TreeClassifier::learn failed to build any tree nodes.");
    }

    x_train_.clear();
    x_train_.shrink_to_fit();
    y_train_.clear();
    y_train_.shrink_to_fit();

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

    const std::vector<double> x_query = matrix.x_rows.to_vec<double>();
    std::vector<double> probs{};
    probs.reserve(static_cast<std::size_t>(matrix.sample_count));

    for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
      std::int64_t node_idx = 0;
      while (!nodes_[static_cast<std::size_t>(node_idx)].is_leaf) {
        const TreeNode& node = nodes_[static_cast<std::size_t>(node_idx)];
        const double xv = x_query[static_cast<std::size_t>(i * feature_count_ + node.feature)];
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

    const std::vector<double> probs = probs_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::vector<double> labels{};
    labels.reserve(probs.size());
    for (double p : probs) {
      labels.push_back(p >= 0.5 ? positive_label_ : negative_label_);
    }
    Series out = pluss::make_series(labels, {static_cast<std::int64_t>(labels.size())}, f64_cpu());
    out = out.to(batch.x.platform(), label_dtype_);
    return StatusOr<Series>(std::move(out));
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
                              .to(pluss::pCPU, ScalarType::Float64)
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

  [[nodiscard]] double x_at(std::int64_t row, std::int64_t feature) const {
    return x_train_[static_cast<std::size_t>(row * feature_count_ + feature)];
  }

  std::int64_t build_node(const std::vector<std::int64_t>& indices, std::int64_t depth) {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());
    double positive_count = 0.0;
    for (std::int64_t idx : indices) positive_count += y_train_[static_cast<std::size_t>(idx)];

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
      double total_positive = 0.0;
      for (std::int64_t idx : indices) {
        const double yi = y_train_[static_cast<std::size_t>(idx)];
        total_positive += yi;
        values.emplace_back(x_at(idx, f), yi);
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
  std::vector<double> x_train_{};
  std::vector<double> y_train_{};
  std::vector<TreeNode> nodes_{};
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
      const Series idx0 = pluss::make_series(std::vector<std::int64_t>{0}, {1}, i64_cpu());
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::zeros({1}, f64_cpu()), false);
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
      const Series idx0 = pluss::make_series(std::vector<std::int64_t>{0}, {1}, i64_cpu());
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::zeros({1}, f64_cpu()), false);
    }
    loss += 0.5 * lambda * ridge.square().sum().item<double>();
  }
  return loss;
}

[[nodiscard]] StatusOr<Series> classification_metrics(const Series& truth01,
                                                      const Series& labels01,
                                                      const Series& probs,
                                                      const std::vector<MetricKind>& requested) {
  const std::vector<double> y = truth01.to(pluss::pCPU, ScalarType::Float64).reshape({truth01.numel()}).to_vec<double>();
  const std::vector<double> yh = labels01.to(pluss::pCPU, ScalarType::Float64).reshape({labels01.numel()}).to_vec<double>();
  const std::vector<double> p = probs.to(pluss::pCPU, ScalarType::Float64).reshape({probs.numel()}).to_vec<double>();

  if (y.size() != yh.size() || y.size() != p.size() || y.empty()) {
    return StatusOr<Series>(Status::shape_mismatch("assess classification metrics require equal non-empty vectors."));
  }

  double correct = 0.0;
  double tp = 0.0;
  double tn = 0.0;
  double fp = 0.0;
  double fn = 0.0;
  double logloss = 0.0;
  constexpr double eps = 1e-12;

  for (std::size_t i = 0; i < y.size(); ++i) {
    const double yt = y[i] >= 0.5 ? 1.0 : 0.0;
    const double yp = yh[i] >= 0.5 ? 1.0 : 0.0;
    correct += (yt == yp) ? 1.0 : 0.0;

    if (yt == 1.0 && yp == 1.0) tp += 1.0;
    if (yt == 0.0 && yp == 0.0) tn += 1.0;
    if (yt == 0.0 && yp == 1.0) fp += 1.0;
    if (yt == 1.0 && yp == 0.0) fn += 1.0;

    const double pi = std::clamp(p[i], eps, 1.0 - eps);
    logloss += -(yt * std::log(pi) + (1.0 - yt) * std::log(1.0 - pi));
  }

  const double n = static_cast<double>(y.size());
  const double accuracy = correct / n;
  const double tpr = (tp + fn > 0.0) ? tp / (tp + fn) : 0.0;
  const double tnr = (tn + fp > 0.0) ? tn / (tn + fp) : 0.0;
  const double bal_accuracy = 0.5 * (tpr + tnr);
  logloss /= n;

  std::vector<double> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Accuracy:
        out.push_back(accuracy);
        break;
      case MetricKind::BalancedAccuracy:
        out.push_back(bal_accuracy);
        break;
      case MetricKind::LogLoss:
        out.push_back(logloss);
        break;
      default:
        return StatusOr<Series>(Status::invalid_argument("Unsupported classification metric in assess()."));
    }
  }

  return StatusOr<Series>(pluss::make_series(out, {static_cast<std::int64_t>(out.size())}, f64_cpu()));
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
    Series weights = pluss::zeros({param_count}, f64_cpu());
    const double lambda =
        options_.regularization == RegularizationKind::L2 ? options_.regularization_strength : 0.0;

    optim::AdamState adam = optim::adam_state_like(std::vector<Series>{weights});
    optim::AdamOptions adam_opt{};
    adam_opt.learning_rate = 0.05;
    adam_opt.weight_decay = 0.0;

    double prev_loss = std::numeric_limits<double>::infinity();
    for (std::int64_t iter = 0; iter < options.max_iterations; ++iter) {
      const Series grad = logistic_gradient(design, labels.y01, weights, options_.fit_intercept, lambda);
      if (!all_finite(grad)) {
        return Status::numerical_failure("LinearClassifier::learn encountered non-finite gradient.");
      }

      std::vector<Series> params{weights};
      optim::adam_step_inplace(params, std::vector<Series>{grad}, adam, adam_opt);
      weights = params[0];

      const double loss = logistic_objective(design, labels.y01, weights, options_.fit_intercept, lambda);
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

    const Series scores = scores_or.value().to(pluss::pCPU, ScalarType::Float64).reshape({scores_or.value().numel()});
    const std::vector<double> score_vec = scores.to_vec<double>();
    std::vector<double> labels{};
    labels.reserve(score_vec.size());
    for (double s : score_vec) {
      labels.push_back(s >= 0.0 ? positive_label_ : negative_label_);
    }

    Series out = pluss::make_series(labels, {static_cast<std::int64_t>(labels.size())}, f64_cpu());
    out = out.to(batch.x.platform(), label_dtype_);
    return StatusOr<Series>(std::move(out));
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
    const Series probs = scores_or.value().to(pluss::pCPU, ScalarType::Float64).sigmoid();

    StatusOr<Series> pred_or = infer(batch.features);
    if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
    const Series pred01 = pred_or.value()
                              .to(pluss::pCPU, ScalarType::Float64)
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

} // namespace

StatusOr<std::unique_ptr<ClassifierBase>> make_classifier(const ModelSpec& spec) {
  if (spec.task != TaskKind::Classification) {
    return StatusOr<std::unique_ptr<ClassifierBase>>(
        Status::invalid_argument("make_classifier requires ModelSpec.task = TaskKind::Classification."));
  }
  if (!is_phase0_family(spec.family)) {
    return StatusOr<std::unique_ptr<ClassifierBase>>(Status::invalid_argument("Unsupported classifier family."));
  }
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
  if (spec.family == ModelFamily::Neighbor) {
    const auto* options = std::get_if<NeighborFamilyOptions>(&spec.options);
    if (options == nullptr) {
      return StatusOr<std::unique_ptr<ClassifierBase>>(
          Status::invalid_argument("Neighbor classifier requires NeighborFamilyOptions in ModelSpec.options."));
    }
    return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<NeighborClassifier>(*options));
  }
  return StatusOr<std::unique_ptr<ClassifierBase>>(std::make_unique<FamilyClassifier>(spec.family));
}

} // namespace pluss::ai::nodej
