#include "pluss/ai/nodej/builders.h"

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/linalg.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <future>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
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

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t x) {
  x += 0x9e3779b97f4a7c15ULL;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  return x ^ (x >> 31);
}

[[nodiscard]] std::int64_t resolved_parallel_workers(std::int64_t tasks) {
  if (tasks <= 1) return 1;
  const std::uint32_t hw = std::thread::hardware_concurrency();
  const std::int64_t cap = hw == 0 ? 1 : static_cast<std::int64_t>(hw);
  return std::max<std::int64_t>(1, std::min<std::int64_t>(tasks, cap));
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

[[nodiscard]] Series knn_probability_cpu(const Series& query_rows,
                                         const Series& train_rows,
                                         const Series& y_train,
                                         std::int64_t k,
                                         bool distance_weighted) {
  const std::int64_t nq = query_rows.sizes()[0];
  const std::int64_t nt = train_rows.sizes()[0];
  const std::int64_t d = train_rows.sizes()[1];
  const auto qv = query_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const auto tv = train_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const auto yv = y_train.to(pluss::pCPU, ScalarType::Float64).reshape({y_train.numel()}).contiguous().to_vec<double>();
  const std::int64_t kk = std::max<std::int64_t>(1, std::min<std::int64_t>(k, nt));
  std::vector<double> out(static_cast<std::size_t>(nq), 0.0);
  constexpr double eps = 1e-12;

  for (std::int64_t qi = 0; qi < nq; ++qi) {
    std::vector<std::pair<double, std::int64_t>> best{};
    best.reserve(static_cast<std::size_t>(kk));
    const std::size_t qbase = static_cast<std::size_t>(qi * d);
    for (std::int64_t ti = 0; ti < nt; ++ti) {
      const std::size_t tbase = static_cast<std::size_t>(ti * d);
      double d2 = 0.0;
      for (std::int64_t f = 0; f < d; ++f) {
        const double dv = qv[qbase + static_cast<std::size_t>(f)] - tv[tbase + static_cast<std::size_t>(f)];
        d2 += dv * dv;
      }
      if (static_cast<std::int64_t>(best.size()) < kk) {
        best.emplace_back(d2, ti);
        if (static_cast<std::int64_t>(best.size()) == kk) {
          std::make_heap(best.begin(), best.end());
        }
      } else if (d2 < best.front().first) {
        std::pop_heap(best.begin(), best.end());
        best.back() = std::pair<double, std::int64_t>{d2, ti};
        std::push_heap(best.begin(), best.end());
      }
    }
    std::sort_heap(best.begin(), best.end());

    if (distance_weighted) {
      double zero_sum = 0.0;
      double zero_count = 0.0;
      for (const auto& [d2, ti] : best) {
        if (d2 <= eps) {
          zero_sum += yv[static_cast<std::size_t>(ti)];
          zero_count += 1.0;
        }
      }
      if (zero_count > 0.0) {
        out[static_cast<std::size_t>(qi)] = std::clamp(zero_sum / zero_count, 0.0, 1.0);
        continue;
      }

      double wsum = 0.0;
      double ysum = 0.0;
      for (const auto& [d2, ti] : best) {
        const double w = 1.0 / std::sqrt(d2 + eps);
        wsum += w;
        ysum += w * yv[static_cast<std::size_t>(ti)];
      }
      out[static_cast<std::size_t>(qi)] = wsum > 0.0 ? std::clamp(ysum / wsum, 0.0, 1.0) : 0.0;
    } else {
      double sum = 0.0;
      for (const auto& [_, ti] : best) {
        sum += yv[static_cast<std::size_t>(ti)];
      }
      out[static_cast<std::size_t>(qi)] = std::clamp(sum / static_cast<double>(kk), 0.0, 1.0);
    }
  }

  return pluss::make_series(out, {nq}, f64_cpu());
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
                                        double positive_threshold,
                                        Platform out_platform,
                                        ScalarType out_dtype);
[[nodiscard]] Series labels_from_probabilities(const Series& probs,
                                               double negative_label,
                                               double positive_label,
                                               double positive_threshold,
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
  const double negative = classes.index({idx0}).reshape({1}).item<double>();
  const double positive = classes.index({idx1}).reshape({1}).item<double>();
  const Series y01 = y64.eq(positive).to(ScalarType::Float64);

  BinaryLabelEncoding out{};
  out.y01 = y01;
  out.negative = negative;
  out.positive = positive;
  out.label_dtype = batch.y.scalar_type();
  return StatusOr<BinaryLabelEncoding>(std::move(out));
}

[[nodiscard]] Status validate_positive_threshold(double threshold, const char* where) {
  if (!std::isfinite(threshold) || threshold < 0.0 || threshold > 1.0) {
    return Status::invalid_argument(std::string(where) + " requires positive_threshold in [0, 1].");
  }
  return Status::success();
}

[[nodiscard]] StatusOr<Series> resolve_training_weights(const LabeledBatch& batch,
                                                        const TrainOptions& options,
                                                        const BinaryLabelEncoding& labels,
                                                        const char* where) {
  const std::int64_t n = labels.y01.numel();
  if (n <= 0) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires at least one training sample."));
  }
  const Platform platform = labels.y01.platform();
  Series weights = pluss::ones({n}, f64_on(platform));

  auto apply_sample_weight = [&](const Series& src, const char* label) -> Status {
    if (!src.defined()) return Status::success();
    if (src.numel() != n) {
      return Status::shape_mismatch(std::string(where) + " " + label + " length must match sample count.");
    }
    Series w = src.to(platform, ScalarType::Float64).reshape({n});
    if (!all_finite(w)) {
      return Status::invalid_argument(std::string(where) + " " + label + " must be finite.");
    }
    if (w.le(0.0).any().item<bool>()) {
      return Status::invalid_argument(std::string(where) + " " + label + " must be strictly positive.");
    }
    weights = weights.mul(w);
    return Status::success();
  };

  Status sample_status = apply_sample_weight(batch.sample_weights, "batch.sample_weights");
  if (!sample_status.ok()) return StatusOr<Series>(std::move(sample_status));
  sample_status = apply_sample_weight(options.sample_weights, "TrainOptions.sample_weights");
  if (!sample_status.ok()) return StatusOr<Series>(std::move(sample_status));

  if (options.class_weight == ClassWeightMode::Balanced) {
    const double pos = labels.y01.sum().item<double>();
    const double neg = static_cast<double>(n) - pos;
    if (!(pos > 0.0 && neg > 0.0)) {
      return StatusOr<Series>(
          Status::invalid_argument(std::string(where) + " balanced class weights require both classes to be present."));
    }
    const double w_pos = static_cast<double>(n) / (2.0 * pos);
    const double w_neg = static_cast<double>(n) / (2.0 * neg);
    const Series cw = labels.y01.mul(w_pos).add(labels.y01.mul(-1.0).add(1.0).mul(w_neg));
    weights = weights.mul(cw);
  } else if (options.class_weight == ClassWeightMode::Custom) {
    if (!options.class_weights.defined()) {
      return StatusOr<Series>(Status::invalid_argument(
          std::string(where) + " TrainOptions.class_weights must be provided for ClassWeightMode::Custom."));
    }
    Series cw = options.class_weights.to(platform, ScalarType::Float64).reshape({options.class_weights.numel()});
    if (cw.numel() != 2) {
      return StatusOr<Series>(Status::shape_mismatch(
          std::string(where) + " TrainOptions.class_weights must be length 2: [negative, positive]."));
    }
    if (!all_finite(cw) || cw.le(0.0).any().item<bool>()) {
      return StatusOr<Series>(Status::invalid_argument(
          std::string(where) + " TrainOptions.class_weights values must be finite and > 0."));
    }
    const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, platform);
    const Series idx1 = make_index_series(std::vector<std::int64_t>{1}, platform);
    const double w_neg = cw.index_select(0, idx0).reshape({1}).item<double>();
    const double w_pos = cw.index_select(0, idx1).reshape({1}).item<double>();
    const Series per_sample = labels.y01.mul(w_pos).add(labels.y01.mul(-1.0).add(1.0).mul(w_neg));
    weights = weights.mul(per_sample);
  }

  if (!all_finite(weights)) {
    return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite training weights."));
  }
  if (weights.le(0.0).all().item<bool>()) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " produced zero total training weight."));
  }

  const double mean_w = weights.mean().item<double>();
  if (std::isfinite(mean_w) && mean_w > 0.0) {
    weights = weights.div(mean_w);
  }
  return StatusOr<Series>(weights);
}

[[nodiscard]] StatusOr<Series> fit_platt_calibrator(const Series& scores,
                                                    const Series& y01,
                                                    const Series& sample_weights,
                                                    std::int64_t max_iterations,
                                                    double tolerance,
                                                    const char* where) {
  if (scores.numel() != y01.numel() || scores.numel() != sample_weights.numel()) {
    return StatusOr<Series>(Status::shape_mismatch(std::string(where) + " expects equal scores/labels/weights lengths."));
  }
  if (scores.numel() == 0) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires non-empty scores."));
  }
  if (max_iterations <= 0 || !(tolerance > 0.0)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires positive iterations and tolerance."));
  }

  const Platform platform = scores.platform();
  Series params = pluss::make_series(std::vector<double>{1.0, 0.0}, {2}, f64_on(platform));
  const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, platform);
  const Series idx1 = make_index_series(std::vector<std::int64_t>{1}, platform);
  const double weight_sum = sample_weights.sum().item<double>();
  if (!(weight_sum > 0.0) || !std::isfinite(weight_sum)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires strictly positive total sample weight."));
  }

  constexpr double eps = 1e-12;
  double prev_loss = std::numeric_limits<double>::infinity();
  for (std::int64_t iter = 0; iter < max_iterations; ++iter) {
    const double a = params.index_select(0, idx0).reshape({1}).item<double>();
    const double b = params.index_select(0, idx1).reshape({1}).item<double>();

    const Series logits = scores.mul(a).add(b);
    const Series probs = logits.sigmoid();
    const Series err = probs.sub(y01);
    const Series weighted_err = err.mul(sample_weights);
    const Series grad_a = weighted_err.mul(scores).sum().div(weight_sum);
    const Series grad_b = weighted_err.sum().div(weight_sum);
    const Series grad = grad_a.reshape({1}).unite({grad_b.reshape({1})}, 0);
    if (!all_finite(grad)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite calibration gradient."));
    }

    const double lr = 0.05;
    params = params.sub(grad.mul(lr));

    const Series clipped = probs.clamp(eps, 1.0 - eps);
    const Series loss_terms = y01.mul(clipped.log()).add(y01.mul(-1.0).add(1.0).mul(clipped.mul(-1.0).add(1.0).log())).mul(-1.0);
    const double loss = loss_terms.mul(sample_weights).sum().div(weight_sum).item<double>();
    if (!std::isfinite(loss)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite calibration objective."));
    }
    if (std::abs(prev_loss - loss) <= tolerance) {
      break;
    }
    prev_loss = loss;
  }

  if (!all_finite(params)) {
    return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite calibration parameters."));
  }
  return StatusOr<Series>(params);
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
    Series probs = Series::undefined();
    if (x_query.platform() == pluss::pCPU && x_train_.platform() == pluss::pCPU) {
      probs = knn_probability_cpu(x_query, x_train_, y_train_, neighbor_count, options_.distance_weighted);
    } else {
      const Series dist2 = pairwise_distance2(x_query, x_train_);
      const Series sort_idx = dist2.argsort(1, false, true);
      const Series k_cols =
          pluss::arange(0, neighbor_count, Config{}.with_dtype(ScalarType::Int64).with_platform(sort_idx.platform()));
      const Series knn_idx = sort_idx.index_select(1, k_cols);
      const Series y_train_2d =
          y_train_.reshape({1, sample_count_}).expand(std::vector<std::int64_t>{matrix.sample_count, sample_count_});
      const Series y_knn = y_train_2d.gather(1, knn_idx);

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
    }

    return StatusOr<Series>(probs.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "NeighborClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> probs_or = infer_scores(batch, options);
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());
    return StatusOr<Series>(
        labels_from_probabilities(probs_or.value().reshape({probs_or.value().numel()}),
                                  negative_label_,
                                  positive_label_,
                                  options.positive_threshold,
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

    const InferOptions infer_options{
        .layout = options.layout,
        .missing = MissingPolicy::KeepNaN,
        .unknown_categorical = UnknownCategoryPolicy::Ignore,
        .positive_threshold = options.positive_threshold,
    };
    StatusOr<Series> pred_or = infer(batch.features, infer_options);
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

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(sample_count_),
                   static_cast<double>(feature_count_),
                   static_cast<double>(k_),
                   negative_label_,
                   positive_label_,
                   static_cast<double>(static_cast<std::int64_t>(label_dtype_))}),
        x_train_,
        y_train_,
        mean_,
        scale_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 5) return Status::invalid_argument("NeighborClassifier::load_state expects 5 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 7) return Status::invalid_argument("NeighborClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    k_ = static_cast<std::int64_t>(meta[3]);
    negative_label_ = meta[4];
    positive_label_ = meta[5];
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[6]));
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
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return Status::not_implemented(
          "TreeClassifier::learn currently has a CPU-only implementation. Provide CPU features for tree learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(batch, matrix.sample_count, "TreeClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    x_train_host_ = matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
    y_train_host_ = labels.y01.to(pluss::pCPU, ScalarType::Float64).reshape({sample_count_}).contiguous().to_vec<double>();

    nodes_.clear();
    nodes_.reserve(static_cast<std::size_t>(2 * sample_count_ + 1));
    std::vector<std::int64_t> root_indices(static_cast<std::size_t>(sample_count_));
    std::iota(root_indices.begin(), root_indices.end(), static_cast<std::int64_t>(0));
    build_node(root_indices, 0);

    if (nodes_.empty()) {
      return Status::internal("TreeClassifier::learn failed to build any tree nodes.");
    }

    x_train_host_.clear();
    y_train_host_.clear();

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
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return StatusOr<Series>(Status::not_implemented(
          "TreeClassifier::infer_scores currently has a CPU-only implementation. Provide CPU features for tree inference."));
    }

    std::vector<double> probs{};
    Status pred_status = predict_prob_host_rows(
        matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>(),
        matrix.sample_count,
        matrix.feature_count,
        probs);
    if (!pred_status.ok()) return StatusOr<Series>(std::move(pred_status));

    Series out = pluss::make_series(probs, {static_cast<std::int64_t>(probs.size())}, f64_cpu());
    return StatusOr<Series>(out.to(matrix.original_platform, ScalarType::Float64));
  }

  Status predict_prob_host_rows(const std::vector<double>& x_rows,
                                std::int64_t sample_count,
                                std::int64_t feature_count,
                                std::vector<double>& out) const {
    if (!trained_) return Status::precondition("TreeClassifier must be learned before host prediction.");
    if (feature_count != feature_count_) return Status::shape_mismatch("TreeClassifier host prediction feature mismatch.");
    const std::size_t expected = static_cast<std::size_t>(sample_count * feature_count);
    if (x_rows.size() != expected) {
      return Status::shape_mismatch("TreeClassifier host prediction row-major size mismatch.");
    }
    out.assign(static_cast<std::size_t>(sample_count), 0.0);
    for (std::int64_t i = 0; i < sample_count; ++i) {
      std::int64_t node_idx = 0;
      while (!nodes_[static_cast<std::size_t>(node_idx)].is_leaf) {
        const TreeNode& node = nodes_[static_cast<std::size_t>(node_idx)];
        const double xv = x_rows[static_cast<std::size_t>(i * feature_count + node.feature)];
        node_idx = xv < node.threshold ? node.left : node.right;
      }
      out[static_cast<std::size_t>(i)] =
          std::clamp(nodes_[static_cast<std::size_t>(node_idx)].leaf_positive_probability, 0.0, 1.0);
    }
    return Status::success();
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "TreeClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> probs_or = infer_scores(batch, options);
    if (!probs_or.ok()) return StatusOr<Series>(probs_or.status());
    return StatusOr<Series>(
        labels_from_probabilities(probs_or.value().reshape({probs_or.value().numel()}),
                                  negative_label_,
                                  positive_label_,
                                  options.positive_threshold,
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

    const InferOptions infer_options{
        .layout = options.layout,
        .missing = MissingPolicy::KeepNaN,
        .unknown_categorical = UnknownCategoryPolicy::Ignore,
        .positive_threshold = options.positive_threshold,
    };
    StatusOr<Series> pred_or = infer(batch.features, infer_options);
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

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    std::vector<double> flat{};
    flat.reserve(nodes_.size() * 6);
    for (const auto& node : nodes_) {
      flat.push_back(node.is_leaf ? 1.0 : 0.0);
      flat.push_back(static_cast<double>(node.feature));
      flat.push_back(node.threshold);
      flat.push_back(static_cast<double>(node.left));
      flat.push_back(static_cast<double>(node.right));
      flat.push_back(node.leaf_positive_probability);
    }
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(sample_count_),
                   static_cast<double>(feature_count_),
                   negative_label_,
                   positive_label_,
                   static_cast<double>(static_cast<std::int64_t>(label_dtype_)),
                   static_cast<double>(nodes_.size())}),
        state_f64(std::move(flat)),
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 2) return Status::invalid_argument("TreeClassifier::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 7) return Status::invalid_argument("TreeClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    negative_label_ = meta[3];
    positive_label_ = meta[4];
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[5]));
    const std::size_t node_count = static_cast<std::size_t>(std::max<double>(0.0, meta[6]));
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
      node.leaf_positive_probability = flat[base + 5];
      nodes_[i] = node;
    }
    x_train_host_.clear();
    y_train_host_.clear();
    return Status::success();
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

  [[nodiscard]] double x_train_at(std::int64_t row, std::int64_t feature) const {
    return x_train_host_[static_cast<std::size_t>(row * feature_count_ + feature)];
  }

  [[nodiscard]] double y_train_at(std::int64_t row) const {
    return y_train_host_[static_cast<std::size_t>(row)];
  }

  std::int64_t build_node(const std::vector<std::int64_t>& indices, std::int64_t depth) {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());
    double positive_count = 0.0;
    for (std::int64_t idx : indices) positive_count += y_train_at(idx);

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
      if (x_train_at(idx, best.feature) < best.threshold) {
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

  [[nodiscard]] bool split_better(const SplitCandidate& candidate, const SplitCandidate& incumbent) const {
    if (!candidate.valid) return false;
    if (!incumbent.valid) return true;
    const bool better_gain = candidate.gain > incumbent.gain + 1e-15;
    const bool tie_break = std::abs(candidate.gain - incumbent.gain) <= 1e-15
                           && (candidate.feature < incumbent.feature
                               || (candidate.feature == incumbent.feature && candidate.threshold < incumbent.threshold));
    return better_gain || tie_break;
  }

  [[nodiscard]] SplitCandidate find_best_split_for_feature(const std::vector<std::int64_t>& indices,
                                                           double parent_impurity,
                                                           std::int64_t feature) const {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());
    SplitCandidate best{};
    best.feature = feature;

    std::vector<std::pair<double, double>> values{};
    values.reserve(static_cast<std::size_t>(n));
    double total_positive = 0.0;
    for (std::int64_t idx : indices) {
      const double yi = y_train_at(idx);
      total_positive += yi;
      values.emplace_back(x_train_at(idx, feature), yi);
    }
    std::sort(values.begin(), values.end(),
              [](const auto& a, const auto& b) {
                if (a.first != b.first) return a.first < b.first;
                return a.second < b.second;
              });
    if (values.front().first == values.back().first) return best;

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

      const SplitCandidate candidate{
          .valid = true,
          .feature = feature,
          .threshold = threshold,
          .gain = gain,
      };
      if (split_better(candidate, best)) {
        best = candidate;
      }
    }

    return best;
  }

  [[nodiscard]] SplitCandidate find_best_split(const std::vector<std::int64_t>& indices, double parent_impurity) const {
    const std::int64_t n = static_cast<std::int64_t>(indices.size());
    SplitCandidate best{};
    const bool parallel_feature_scan = feature_count_ >= 8 && n >= 1024;
    const std::int64_t workers = parallel_feature_scan ? resolved_parallel_workers(feature_count_) : 1;
    if (workers <= 1) {
      for (std::int64_t f = 0; f < feature_count_; ++f) {
        const SplitCandidate candidate = find_best_split_for_feature(indices, parent_impurity, f);
        if (split_better(candidate, best)) {
          best = candidate;
        }
      }
      return best;
    }

    const std::int64_t chunk = std::max<std::int64_t>(1, (feature_count_ + workers - 1) / workers);
    std::vector<std::future<SplitCandidate>> futures{};
    for (std::int64_t start = 0; start < feature_count_; start += chunk) {
      const std::int64_t end = std::min(feature_count_, start + chunk);
      futures.emplace_back(std::async(std::launch::async, [this, &indices, parent_impurity, start, end]() {
        SplitCandidate local{};
        for (std::int64_t f = start; f < end; ++f) {
          const SplitCandidate candidate = find_best_split_for_feature(indices, parent_impurity, f);
          if (split_better(candidate, local)) {
            local = candidate;
          }
        }
        return local;
      }));
    }
    for (auto& future : futures) {
      const SplitCandidate candidate = future.get();
      if (split_better(candidate, best)) {
        best = candidate;
      }
    }
    return best;
  }

  TreeFamilyOptions options_{};
  bool trained_{false};
  std::int64_t sample_count_{0};
  std::int64_t feature_count_{0};
  std::vector<double> x_train_host_{};
  std::vector<double> y_train_host_{};
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
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return Status::not_implemented(
          "EnsembleClassifier::learn currently has a CPU-only implementation. Provide CPU features for ensemble learn().");
    }

    StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(batch, matrix.sample_count, "EnsembleClassifier::learn");
    if (!labels_or.ok()) return labels_or.status();
    const BinaryLabelEncoding labels = labels_or.move_value();

    feature_count_ = matrix.feature_count;
    sample_count_ = matrix.sample_count;
    const std::vector<double> x_train_host =
        matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
    const std::vector<double> y_train_host =
        labels.y01.reshape({sample_count_}).to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();

    const std::int64_t learner_count = options_.learners;
    const std::int64_t subsample_count =
        std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(options_.subsample * sample_count_)));
    const std::int64_t mtry = options_.method == EnsembleMethod::RandomForest
                                  ? std::max<std::int64_t>(1, static_cast<std::int64_t>(std::sqrt(static_cast<double>(feature_count_))))
                                  : feature_count_;

    learners_.clear();
    learners_.reserve(static_cast<std::size_t>(learner_count));
    std::uint64_t base_seed = options.random.seed;
    if (!options.random.deterministic) {
      std::random_device rd;
      base_seed = (static_cast<std::uint64_t>(rd()) << 32U) ^ static_cast<std::uint64_t>(rd());
    }

    auto train_member = [&](std::int64_t learner_index) -> StatusOr<EnsembleMember> {
      std::mt19937_64 rng(splitmix64(base_seed + static_cast<std::uint64_t>(learner_index)));
      std::uniform_int_distribution<std::int64_t> row_dist(0, sample_count_ - 1);

      std::vector<std::int64_t> feature_ids(static_cast<std::size_t>(feature_count_));
      std::iota(feature_ids.begin(), feature_ids.end(), static_cast<std::int64_t>(0));
      if (mtry < feature_count_) {
        std::shuffle(feature_ids.begin(), feature_ids.end(), rng);
        feature_ids.resize(static_cast<std::size_t>(mtry));
        std::sort(feature_ids.begin(), feature_ids.end());
      }

      const std::int64_t learner_features = static_cast<std::int64_t>(feature_ids.size());
      constexpr std::int64_t max_attempts = 16;
      for (std::int64_t attempt = 0; attempt < max_attempts; ++attempt) {
        std::vector<std::int64_t> row_ids(static_cast<std::size_t>(subsample_count), 0);
        std::vector<double> y_sub_host(static_cast<std::size_t>(subsample_count), 0.0);
        double positive_count = 0.0;
        for (std::int64_t i = 0; i < subsample_count; ++i) {
          const std::int64_t row = row_dist(rng);
          row_ids[static_cast<std::size_t>(i)] = row;
          const double yv = y_train_host[static_cast<std::size_t>(row)];
          y_sub_host[static_cast<std::size_t>(i)] = yv;
          positive_count += yv;
        }
        if (positive_count <= 0.0 || positive_count >= static_cast<double>(subsample_count)) {
          continue;
        }

        std::vector<double> x_sub_host(static_cast<std::size_t>(subsample_count * learner_features), 0.0);
        for (std::int64_t i = 0; i < subsample_count; ++i) {
          const std::int64_t row = row_ids[static_cast<std::size_t>(i)];
          const std::size_t in_base = static_cast<std::size_t>(row * feature_count_);
          const std::size_t out_base = static_cast<std::size_t>(i * learner_features);
          for (std::int64_t j = 0; j < learner_features; ++j) {
            const std::int64_t f = feature_ids[static_cast<std::size_t>(j)];
            x_sub_host[out_base + static_cast<std::size_t>(j)] = x_train_host[in_base + static_cast<std::size_t>(f)];
          }
        }

        const Series x_sub = pluss::make_series(std::move(x_sub_host), {subsample_count, learner_features}, f64_cpu());
        const Series y_sub = pluss::make_series(std::move(y_sub_host), {subsample_count}, f64_cpu());
        const LabeledBatch sub_batch{
            .features = FeatureBatch{.x = x_sub, .layout = ObservationLayout::Rows},
            .y = y_sub,
        };

        auto tree = std::make_unique<TreeClassifier>(TreeFamilyOptions{});
        Status tree_status = tree->learn(sub_batch, options);
        if (!tree_status.ok()) continue;
        EnsembleMember member{};
        member.tree = std::move(tree);
        member.feature_ids = std::move(feature_ids);
        return StatusOr<EnsembleMember>(std::move(member));
      }

      return StatusOr<EnsembleMember>(Status::internal(
          "EnsembleClassifier::learn could not draw a class-diverse bootstrap sample for learner "
          + std::to_string(learner_index) + "."));
    };

    const std::int64_t workers = resolved_parallel_workers(learner_count);
    if (workers <= 1) {
      for (std::int64_t l = 0; l < learner_count; ++l) {
        StatusOr<EnsembleMember> member_or = train_member(l);
        if (!member_or.ok()) return member_or.status();
        learners_.push_back(member_or.move_value());
      }
    } else {
      for (std::int64_t start = 0; start < learner_count; start += workers) {
        const std::int64_t end = std::min(learner_count, start + workers);
        std::vector<std::future<StatusOr<EnsembleMember>>> futures{};
        futures.reserve(static_cast<std::size_t>(end - start));
        for (std::int64_t l = start; l < end; ++l) {
          futures.emplace_back(std::async(std::launch::async, [&, l]() { return train_member(l); }));
        }
        for (std::int64_t l = start; l < end; ++l) {
          StatusOr<EnsembleMember> member_or = futures[static_cast<std::size_t>(l - start)].get();
          if (!member_or.ok()) return member_or.status();
          learners_.push_back(member_or.move_value());
        }
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
    if (matrix.x_rows.platform() != pluss::pCPU) {
      return StatusOr<Series>(Status::not_implemented(
          "EnsembleClassifier::infer_scores currently has a CPU-only implementation. Provide CPU features for ensemble inference."));
    }

    const std::vector<double> x_host = matrix.x_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
    std::vector<double> probs_sum(static_cast<std::size_t>(matrix.sample_count), 0.0);

    auto infer_member = [&](std::size_t member_index) -> StatusOr<std::vector<double>> {
      const EnsembleMember& member = learners_[member_index];
      const std::int64_t learner_features = static_cast<std::int64_t>(member.feature_ids.size());
      std::vector<double> x_sub_host(static_cast<std::size_t>(matrix.sample_count * learner_features), 0.0);
      for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
        const std::size_t x_row_base = static_cast<std::size_t>(i * matrix.feature_count);
        const std::size_t out_row_base = static_cast<std::size_t>(i * learner_features);
        for (std::int64_t j = 0; j < learner_features; ++j) {
          const std::int64_t f = member.feature_ids[static_cast<std::size_t>(j)];
          x_sub_host[out_row_base + static_cast<std::size_t>(j)] = x_host[x_row_base + static_cast<std::size_t>(f)];
        }
      }
      std::vector<double> member_probs{};
      Status member_status = member.tree->predict_prob_host_rows(x_sub_host, matrix.sample_count, learner_features, member_probs);
      if (!member_status.ok()) return StatusOr<std::vector<double>>(member_status);
      return StatusOr<std::vector<double>>(std::move(member_probs));
    };

    const std::int64_t workers = resolved_parallel_workers(static_cast<std::int64_t>(learners_.size()));
    if (workers <= 1) {
      for (std::size_t mi = 0; mi < learners_.size(); ++mi) {
        StatusOr<std::vector<double>> member_probs_or = infer_member(mi);
        if (!member_probs_or.ok()) return StatusOr<Series>(member_probs_or.status());
        const auto& member_probs = member_probs_or.value();
        for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
          probs_sum[static_cast<std::size_t>(i)] += member_probs[static_cast<std::size_t>(i)];
        }
      }
    } else {
      for (std::size_t start = 0; start < learners_.size(); start += static_cast<std::size_t>(workers)) {
        const std::size_t end = std::min(learners_.size(), start + static_cast<std::size_t>(workers));
        std::vector<std::future<StatusOr<std::vector<double>>>> futures{};
        futures.reserve(end - start);
        for (std::size_t mi = start; mi < end; ++mi) {
          futures.emplace_back(std::async(std::launch::async, [&, mi]() { return infer_member(mi); }));
        }
        for (std::size_t mi = start; mi < end; ++mi) {
          StatusOr<std::vector<double>> member_probs_or = futures[mi - start].get();
          if (!member_probs_or.ok()) return StatusOr<Series>(member_probs_or.status());
          const auto& member_probs = member_probs_or.value();
          for (std::int64_t i = 0; i < matrix.sample_count; ++i) {
            probs_sum[static_cast<std::size_t>(i)] += member_probs[static_cast<std::size_t>(i)];
          }
        }
      }
    }

    const double denom = static_cast<double>(learners_.size());
    constexpr double eps = 1e-12;
    for (double& v : probs_sum) {
      v /= denom;
      v = std::clamp(v, eps, 1.0 - eps);
    }
    const Series probs = pluss::make_series(probs_sum, {matrix.sample_count}, f64_cpu());
    const Series scores = probs.log().sub(probs.mul(-1.0).add(1.0).log());
    return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "EnsembleClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    const Series out =
        labels_from_scores(scores_or.value().reshape({scores_or.value().numel()}),
                           negative_label_,
                           positive_label_,
                           options.positive_threshold,
                           batch.x.platform(),
                           label_dtype_);
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

    const InferOptions infer_options{
        .layout = options.layout,
        .missing = MissingPolicy::KeepNaN,
        .unknown_categorical = UnknownCategoryPolicy::Ignore,
        .positive_threshold = options.positive_threshold,
    };
    StatusOr<Series> pred_or = infer(batch.features, infer_options);
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

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    std::vector<Series> out{};
    out.reserve(2 + learners_.size() * 4);
    out.push_back(state_f64({trained_ ? 1.0 : 0.0,
                             static_cast<double>(sample_count_),
                             static_cast<double>(feature_count_),
                             negative_label_,
                             positive_label_,
                             static_cast<double>(static_cast<std::int64_t>(label_dtype_)),
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

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.empty()) return Status::invalid_argument("EnsembleClassifier::load_state expects metadata tensor.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 7) return Status::invalid_argument("EnsembleClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    negative_label_ = meta[3];
    positive_label_ = meta[4];
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[5]));
    const std::int64_t learner_count = static_cast<std::int64_t>(meta[6]);

    learners_.clear();
    learners_.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, learner_count)));
    std::size_t cursor = 1;
    for (std::int64_t li = 0; li < learner_count; ++li) {
      if (cursor >= state.size()) return Status::invalid_argument("EnsembleClassifier::load_state truncated state.");
      EnsembleMember member{};
      member.feature_ids = vec_i64(state[cursor++]);
      if (cursor >= state.size()) return Status::invalid_argument("EnsembleClassifier::load_state missing tree state size.");
      const auto size_vec = vec_i64(state[cursor++]);
      const std::int64_t tree_items = size_vec.empty() ? 0 : size_vec.front();
      if (tree_items <= 0) {
        learners_.push_back(std::move(member));
        continue;
      }
      std::vector<Series> tree_state{};
      tree_state.reserve(static_cast<std::size_t>(tree_items));
      for (std::int64_t ti = 0; ti < tree_items; ++ti) {
        if (cursor >= state.size()) return Status::invalid_argument("EnsembleClassifier::load_state truncated tree state.");
        tree_state.push_back(state[cursor++]);
      }
      auto tree = std::make_unique<TreeClassifier>(TreeFamilyOptions{.criterion = TreeCriterion::Gini});
      Status load_status = tree->load_state(tree_state);
      if (!load_status.ok()) return load_status;
      member.tree = std::move(tree);
      learners_.push_back(std::move(member));
    }
    return Status::success();
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
                                        double positive_threshold,
                                        Platform out_platform,
                                        ScalarType out_dtype) {
  const double t = std::clamp(positive_threshold, 1e-9, 1.0 - 1e-9);
  const double score_threshold = std::log(t / (1.0 - t));
  const Series mask = scores.ge(score_threshold).to(ScalarType::Float64);
  const Series labels = mask.mul(positive_label - negative_label).add(negative_label);
  return labels.to(out_platform, out_dtype);
}

[[nodiscard]] Series labels_from_probabilities(const Series& probs,
                                               double negative_label,
                                               double positive_label,
                                               double positive_threshold,
                                               Platform out_platform,
                                               ScalarType out_dtype) {
  const Series mask = probs.ge(positive_threshold).to(ScalarType::Float64);
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

[[nodiscard]] StatusOr<Series> solve_kernel_dual_cg(const Series& kernel,
                                                    const Series& rhs,
                                                    double lambda,
                                                    std::int64_t max_iterations,
                                                    double tolerance,
                                                    const char* where) {
  const std::int64_t n = kernel.sizes()[0];
  const Platform platform = kernel.platform();
  const Series eye = pluss::eye(n, n, f64_on(platform));
  const Series a = kernel.add(eye.mul(lambda));
  Series x = pluss::zeros({n}, f64_on(platform));
  Series b = rhs.to(platform, ScalarType::Float64).reshape({n});
  Series r = b;
  Series p = r;
  double rs = r.square().sum().reshape({1}).item<double>();
  if (!std::isfinite(rs)) {
    return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite residual norm."));
  }
  if (rs <= std::max(1e-24, tolerance * tolerance)) {
    return StatusOr<Series>(x);
  }

  const double rs0 = rs;
  const std::int64_t iters = std::max<std::int64_t>(4, max_iterations);
  constexpr double eps = 1e-18;
  for (std::int64_t iter = 0; iter < iters; ++iter) {
    const Series ap = a.matmul(p.reshape({n, 1})).reshape({n});
    const double denom = p.mul(ap).sum().reshape({1}).item<double>();
    if (!std::isfinite(denom) || std::abs(denom) <= eps) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered unstable CG denominator."));
    }
    const double alpha = rs / denom;
    x = x.add(p.mul(alpha));
    r = r.sub(ap.mul(alpha));
    const double rs_new = r.square().sum().reshape({1}).item<double>();
    if (!std::isfinite(rs_new)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite CG residual norm."));
    }
    if (std::sqrt(rs_new) <= std::max(tolerance * std::sqrt(std::max(rs0, eps)), 1e-12)) {
      break;
    }
    const double beta = rs_new / std::max(rs, eps);
    p = r.add(p.mul(beta));
    rs = rs_new;
  }
  if (!all_finite(x)) {
    return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite dual coefficients."));
  }
  return StatusOr<Series>(x);
}

[[nodiscard]] Series kernel_scores_cpu(const Series& query_rows,
                                       const Series& train_rows,
                                       const Series& alpha,
                                       KernelKind kind,
                                       double gamma) {
  const std::int64_t nq = query_rows.sizes()[0];
  const std::int64_t nt = train_rows.sizes()[0];
  const std::int64_t d = train_rows.sizes()[1];
  const auto qv = query_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const auto tv = train_rows.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();
  const auto av = alpha.to(pluss::pCPU, ScalarType::Float64).reshape({alpha.numel()}).contiguous().to_vec<double>();
  std::vector<double> out(static_cast<std::size_t>(nq), 0.0);
  for (std::int64_t qi = 0; qi < nq; ++qi) {
    const double* qptr = qv.data() + static_cast<std::size_t>(qi * d);
    double s = 0.0;
    for (std::int64_t ti = 0; ti < nt; ++ti) {
      const double* tptr = tv.data() + static_cast<std::size_t>(ti * d);
      s += kernel_eval(kind, gamma, qptr, tptr, d) * av[static_cast<std::size_t>(ti)];
    }
    out[static_cast<std::size_t>(qi)] = s;
  }
  return pluss::make_series(out, {nq}, f64_cpu());
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
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "BayesClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());

    const Series out =
        labels_from_scores(scores_or.value().reshape({scores_or.value().numel()}),
                           negative_label_,
                           positive_label_,
                           options.positive_threshold,
                           batch.x.platform(),
                           label_dtype_);
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

    const InferOptions infer_options{
        .layout = options.layout,
        .missing = MissingPolicy::KeepNaN,
        .unknown_categorical = UnknownCategoryPolicy::Ignore,
        .positive_threshold = options.positive_threshold,
    };
    StatusOr<Series> pred_or = infer(batch.features, infer_options);
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

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(sample_count_),
                   static_cast<double>(feature_count_),
                   prior0_,
                   prior1_,
                   negative_label_,
                   positive_label_,
                   static_cast<double>(static_cast<std::int64_t>(label_dtype_))}),
        mean0_,
        mean1_,
        var0_,
        var1_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 5) return Status::invalid_argument("BayesClassifier::load_state expects 5 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 8) return Status::invalid_argument("BayesClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    prior0_ = meta[3];
    prior1_ = meta[4];
    negative_label_ = meta[5];
    positive_label_ = meta[6];
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[7]));
    mean0_ = state[1];
    mean1_ = state[2];
    var0_ = state[3];
    var1_ = state[4];
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
    const Series y_pm1 = labels.y01.mul(2.0).sub(1.0).reshape({sample_count_, 1});
    const Series k_series = kernel_matrix(x_train_, x_train_, options_.kernel, gamma_);
    StatusOr<Series> alpha_or = solve_kernel_dual_cg(k_series,
                                                     y_pm1.reshape({sample_count_}),
                                                     lambda_,
                                                     options.max_iterations,
                                                     options.tolerance,
                                                     "KernelClassifier::learn");
    Series alpha = Series::undefined();
    if (alpha_or.ok()) {
      alpha = alpha_or.move_value();
    } else {
      // Fallback for indefinite kernels (for example sigmoid/polynomial edge cases).
      Series shifted = k_series.add(pluss::eye(sample_count_, sample_count_, f64_cpu()).to(x_train_.platform()).mul(lambda_));
      alpha = pluss::subsp::math::linalg::pinverse(shifted, 1e-12).matmul(y_pm1).reshape({sample_count_});
    }
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

    Series scores = Series::undefined();
    if (matrix.x_rows.platform() == pluss::pCPU && x_train_.platform() == pluss::pCPU) {
      scores = kernel_scores_cpu(matrix.x_rows, x_train_, alpha_, options_.kernel, gamma_);
    } else {
      const Series k_query = kernel_matrix(matrix.x_rows, x_train_, options_.kernel, gamma_);
      scores = k_query.matmul(alpha_.reshape({sample_count_, 1})).reshape({matrix.sample_count});
    }
    return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
  }

  [[nodiscard]] StatusOr<Series> infer(const FeatureBatch& batch,
                                       const InferOptions& options = {}) const override {
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "KernelClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());

    const Series out =
        labels_from_scores(scores_or.value().reshape({scores_or.value().numel()}),
                           negative_label_,
                           positive_label_,
                           options.positive_threshold,
                           batch.x.platform(),
                           label_dtype_);
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

    const InferOptions infer_options{
        .layout = options.layout,
        .missing = MissingPolicy::KeepNaN,
        .unknown_categorical = UnknownCategoryPolicy::Ignore,
        .positive_threshold = options.positive_threshold,
    };
    StatusOr<Series> pred_or = infer(batch.features, infer_options);
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

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(sample_count_),
                   static_cast<double>(feature_count_),
                   gamma_,
                   lambda_,
                   negative_label_,
                   positive_label_,
                   static_cast<double>(static_cast<std::int64_t>(label_dtype_))}),
        x_train_,
        alpha_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 3) return Status::invalid_argument("KernelClassifier::load_state expects 3 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 8) return Status::invalid_argument("KernelClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    sample_count_ = static_cast<std::int64_t>(meta[1]);
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    gamma_ = meta[3];
    lambda_ = meta[4];
    negative_label_ = meta[5];
    positive_label_ = meta[6];
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[7]));
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
  double negative_label_{0.0};
  double positive_label_{1.0};
  ScalarType label_dtype_{ScalarType::Float64};
};

[[nodiscard]] Series logistic_gradient(const Series& design,
                                       const Series& y01,
                                       const Series& sample_weight,
                                       const Series& weights,
                                       bool fit_intercept,
                                       double lambda) {
  const std::int64_t n = design.sizes()[0];
  const Series logits = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series probs = logits.sigmoid();
  const Series error = probs.sub(y01).mul(sample_weight);
  const double denom = sample_weight.sum().item<double>();
  Series grad = design.transpose(0, 1).matmul(error.reshape({n, 1})).div(denom).reshape({weights.numel()});

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
                                        const Series& sample_weight,
                                        const Series& weights,
                                        bool fit_intercept,
                                        double lambda) {
  const std::int64_t n = design.sizes()[0];
  const Series logits = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series loss_terms = logits.softplus().sub(y01.mul(logits));
  const double denom = sample_weight.sum().item<double>();
  double loss = loss_terms.mul(sample_weight).sum().div(denom).item<double>();

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

[[nodiscard]] StatusOr<Series> solve_spd_system_with_jitter(const Series& hessian,
                                                            const Series& rhs,
                                                            const char* where) {
  const std::int64_t p = hessian.sizes()[0];
  const Platform platform = hessian.platform();
  const Series eye = pluss::eye(p, p, f64_on(platform));
  const std::array<double, 7> jitters{0.0, 1e-12, 1e-10, 1e-8, 1e-6, 1e-4, 1e-2};
  for (double jitter : jitters) {
    try {
      Series h = hessian;
      if (jitter > 0.0) h = h.add(eye.mul(jitter));
      const Series chol = pluss::subsp::math::linalg::cholesky(h, false).to(platform, ScalarType::Float64);
      const Series delta =
          pluss::subsp::math::linalg::cholesky_solve(rhs.reshape({p, 1}), chol, false).reshape({p}).to(platform, ScalarType::Float64);
      if (all_finite(delta)) return StatusOr<Series>(delta);
    } catch (const std::exception&) {
    }
  }
  return StatusOr<Series>(Status::numerical_failure(std::string(where) + " failed to solve SPD system."));
}

[[nodiscard]] StatusOr<Series> solve_logistic_newton(const Series& design,
                                                     const Series& y01,
                                                     const Series& sample_weight,
                                                     bool fit_intercept,
                                                     double lambda,
                                                     std::int64_t max_iterations,
                                                     double tolerance,
                                                     const char* where) {
  const std::int64_t n = design.sizes()[0];
  const std::int64_t p = design.sizes()[1];
  const Platform platform = design.platform();
  const double denom = sample_weight.sum().item<double>();
  if (!(denom > 0.0) || !std::isfinite(denom)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires strictly positive sample weight sum."));
  }

  Series weights = pluss::zeros({p}, f64_on(platform));
  Series ridge_diag = pluss::zeros({p}, f64_on(platform));
  if (lambda > 0.0) {
    ridge_diag = pluss::ones({p}, f64_on(platform)).mul(lambda);
    if (fit_intercept) {
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, platform);
      ridge_diag = pluss::index_put(ridge_diag,
                                    std::vector<Series>{idx0},
                                    pluss::zeros({1}, f64_on(platform)),
                                    false);
    }
  }
  const Series eye = pluss::eye(p, p, f64_on(platform));
  double prev_loss = std::numeric_limits<double>::infinity();

  for (std::int64_t iter = 0; iter < max_iterations; ++iter) {
    const Series logits = design.matmul(weights.reshape({p, 1})).reshape({n});
    const Series probs = logits.sigmoid();
    const Series err = probs.sub(y01).mul(sample_weight);
    Series grad = design.transpose(0, 1).matmul(err.reshape({n, 1})).div(denom).reshape({p});
    if (lambda > 0.0) {
      grad = grad.add(weights.mul(ridge_diag));
    }
    if (!all_finite(grad)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite gradient."));
    }

    const Series r = probs.mul(probs.mul(-1.0).add(1.0)).mul(sample_weight).clamp_min(1e-12);
    Series hessian = design
                         .transpose(0, 1)
                         .matmul(design.mul(r.reshape({n, 1})))
                         .div(denom);
    if (lambda > 0.0) {
      hessian = hessian.add(eye.mul(ridge_diag.reshape({1, p})));
    }
    if (!all_finite(hessian)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite Hessian."));
    }

    StatusOr<Series> delta_or = solve_spd_system_with_jitter(hessian, grad, where);
    if (!delta_or.ok()) return StatusOr<Series>(delta_or.status());
    const Series delta = delta_or.move_value();
    weights = weights.sub(delta);
    if (!all_finite(weights)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " produced non-finite coefficients."));
    }

    const double loss = logistic_objective(design, y01, sample_weight, weights, fit_intercept, lambda);
    if (!std::isfinite(loss)) {
      return StatusOr<Series>(Status::numerical_failure(std::string(where) + " encountered non-finite objective."));
    }
    const double step_inf = delta.abs().max().reshape({1}).item<double>();
    if (step_inf <= tolerance || std::abs(prev_loss - loss) <= tolerance) {
      break;
    }
    prev_loss = loss;
  }

  return StatusOr<Series>(weights);
}

[[nodiscard]] Series hinge_gradient(const Series& design,
                                    const Series& y01,
                                    const Series& sample_weight,
                                    const Series& weights,
                                    bool fit_intercept,
                                    double lambda) {
  const std::int64_t n = design.sizes()[0];
  const Series scores = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series ypm = y01.mul(2.0).sub(1.0);
  const Series margin = ypm.mul(scores).mul(-1.0).add(1.0);
  const Series active = margin.gt(0.0).to(ScalarType::Float64);
  const Series coeff = ypm.mul(active).mul(-1.0).mul(sample_weight);
  const double denom = sample_weight.sum().item<double>();
  Series grad = design.transpose(0, 1).matmul(coeff.reshape({n, 1})).div(denom).reshape({weights.numel()});

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
                                     const Series& sample_weight,
                                     const Series& weights,
                                     bool fit_intercept,
                                     double lambda) {
  const std::int64_t n = design.sizes()[0];
  const Series scores = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series ypm = y01.mul(2.0).sub(1.0);
  const Series margin = ypm.mul(scores).mul(-1.0).add(1.0);
  const double denom = sample_weight.sum().item<double>();
  double loss = margin.relu().mul(sample_weight).sum().div(denom).item<double>();

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
  const Series one = pluss::ones({1}, f64_on(y.platform()));
  const Series zero = pluss::zeros({1}, f64_on(y.platform()));

  const Series tp = yb.mul(yhb).sum();
  const Series tn = yb.mul(-1.0).add(1.0).mul(yhb.mul(-1.0).add(1.0)).sum();
  const Series fp = yb.mul(-1.0).add(1.0).mul(yhb).sum();
  const Series fn = yb.mul(yhb.mul(-1.0).add(1.0)).sum();
  const Series accuracy = yb.eq(yhb).to(ScalarType::Float64).mean();
  const Series tpr_den = tp.add(fn);
  const Series tnr_den = tn.add(fp);
  const Series tpr_mask = tpr_den.gt(0.0).to(ScalarType::Float64);
  const Series tnr_mask = tnr_den.gt(0.0).to(ScalarType::Float64);
  const Series tpr = tp.div(tpr_den.clamp_min(1.0)).mul(tpr_mask);
  const Series tnr = tn.div(tnr_den.clamp_min(1.0)).mul(tnr_mask);
  const Series bal_accuracy = tpr.add(tnr).mul(0.5);
  const Series precision_den = tp.add(fp);
  const Series precision_mask = precision_den.gt(0.0).to(ScalarType::Float64);
  const Series precision = tp.div(precision_den.clamp_min(1.0)).mul(precision_mask);
  const Series recall = tpr;
  const Series f1_den = precision.add(recall);
  const Series f1_mask = f1_den.gt(0.0).to(ScalarType::Float64);
  const Series f1 = precision.mul(recall).mul(2.0).div(f1_den.clamp_min(1.0)).mul(f1_mask);
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
  const Series roc_auc = roc_auc_raw.mul(has_both).add(one.mul(0.5).mul(one.sub(has_both)));

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
    const Series precision_den = tp.add(fp);
    const Series recall_den = tp.add(fn);
    const Series precision_mask = precision_den.gt(0.0).to(ScalarType::Float64);
    const Series recall_mask = recall_den.gt(0.0).to(ScalarType::Float64);
    const Series precision = tp.div(precision_den.clamp_min(1.0)).mul(precision_mask);
    const Series recall = tp.div(recall_den.clamp_min(1.0)).mul(recall_mask);
    const Series f1_den = precision.add(recall);
    const Series f1_mask = f1_den.gt(0.0).to(ScalarType::Float64);
    const Series f1 = precision.mul(recall).mul(2.0).div(f1_den.clamp_min(1.0)).mul(f1_mask);
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
    if (options.calibration == ScoreCalibration::Platt
        && (options.calibration_iterations <= 0 || !(options.calibration_tolerance > 0.0))) {
      return Status::invalid_argument(
          "LinearClassifier::learn with ScoreCalibration::Platt requires positive calibration iterations and tolerance.");
    }

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
    StatusOr<Series> sample_weight_or =
        resolve_training_weights(batch, options, labels, "LinearClassifier::learn");
    if (!sample_weight_or.ok()) return sample_weight_or.status();
    const Series sample_weight = sample_weight_or.move_value();

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
    optim::SeptimaConstraints septima_constraints{};
    optim::SeptimaOptions septima_options{};
    optim::SeptimaSolveOptions septima_solve_options{};

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

      if (lower.defined()) septima_constraints.lbs = {lower};
      if (upper.defined()) septima_constraints.ubs = {upper};
      septima_options.adam = adam_opt;
      septima_options.project_bounds = true;
      septima_options.enable_constraint_autodiff = false;
      septima_solve_options.step = septima_options;
      septima_solve_options.max_iterations = 1;
      septima_solve_options.max_no_progress_iterations = 0;
      septima_solve_options.objective_tolerance = 0.0;
      septima_solve_options.gradient_tolerance = 0.0;
      septima_solve_options.step_tolerance = 0.0;
      septima_solve_options.constraint_tolerance = 0.0;
    }

    if (options_.objective == LinearObjective::Logistic && !use_parameter_bounds) {
      StatusOr<Series> fast_weights_or =
          solve_logistic_newton(design,
                                labels.y01,
                                sample_weight,
                                options_.fit_intercept,
                                lambda,
                                options.max_iterations,
                                options.tolerance,
                                "LinearClassifier::learn");
      if (!fast_weights_or.ok()) return fast_weights_or.status();
      weights = fast_weights_or.move_value();
    } else {
      double prev_loss = std::numeric_limits<double>::infinity();
      for (std::int64_t iter = 0; iter < options.max_iterations; ++iter) {
        Series grad = Series::undefined();
        if (options_.objective == LinearObjective::Logistic) {
          grad = logistic_gradient(design, labels.y01, sample_weight, weights, options_.fit_intercept, lambda);
        } else {
          grad = hinge_gradient(design, labels.y01, sample_weight, weights, options_.fit_intercept, lambda);
        }
        if (!all_finite(grad)) {
          return Status::numerical_failure("LinearClassifier::learn encountered non-finite gradient.");
        }

        try {
          if (use_parameter_bounds) {
            const auto fit = optim::septima(
                [&](const optim::Trace& w) -> optim::Trace {
                  return optim::sum(w * optim::fixed(grad));
                },
                weights,
                septima_solve_options,
                septima_constraints);
            weights = fit.params.front();
          } else {
            std::vector<Series> params{weights};
            optim::adam_step_inplace(params, std::vector<Series>{grad}, adam, adam_opt);
            weights = params[0];
          }
        } catch (const std::exception& ex) {
          return Status::numerical_failure(std::string("LinearClassifier::learn optimizer failure: ") + ex.what());
        }

        const double loss = options_.objective == LinearObjective::Logistic
                                ? logistic_objective(design, labels.y01, sample_weight, weights, options_.fit_intercept, lambda)
                                : hinge_objective(design, labels.y01, sample_weight, weights, options_.fit_intercept, lambda);
        if (!std::isfinite(loss)) {
          return Status::numerical_failure("LinearClassifier::learn encountered non-finite objective.");
        }
        if (std::abs(prev_loss - loss) <= options.tolerance) {
          break;
        }
        prev_loss = loss;
      }
    }

    if (!all_finite(weights)) {
      return Status::numerical_failure("LinearClassifier::learn produced non-finite coefficients.");
    }

    calibrated_scores_ = false;
    calibration_a_ = 1.0;
    calibration_b_ = 0.0;
    if (options.calibration == ScoreCalibration::Platt) {
      const Series train_scores = design.matmul(weights.reshape({weights.numel(), 1})).reshape({matrix.sample_count});
      StatusOr<Series> params_or = fit_platt_calibrator(train_scores,
                                                        labels.y01,
                                                        sample_weight,
                                                        options.calibration_iterations,
                                                        options.calibration_tolerance,
                                                        "LinearClassifier::learn");
      if (!params_or.ok()) return params_or.status();
      const Series params = params_or.move_value();
      const Series idx0 = make_index_series(std::vector<std::int64_t>{0}, params.platform());
      const Series idx1 = make_index_series(std::vector<std::int64_t>{1}, params.platform());
      calibration_a_ = params.index_select(0, idx0).reshape({1}).item<double>();
      calibration_b_ = params.index_select(0, idx1).reshape({1}).item<double>();
      calibrated_scores_ = true;
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
    Status threshold_status =
        validate_positive_threshold(options.positive_threshold, "LinearClassifier::infer");
    if (!threshold_status.ok()) return StatusOr<Series>(std::move(threshold_status));
    StatusOr<Series> scores_or = infer_scores(batch, options);
    if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
    const Series probs = probabilities_from_scores(scores_or.value().reshape({scores_or.value().numel()}));

    const Series out =
        labels_from_probabilities(probs, negative_label_, positive_label_, options.positive_threshold, batch.x.platform(), label_dtype_);
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
    const Series probs = probabilities_from_scores(scores_or.value().reshape({scores_or.value().numel()}));

    const InferOptions infer_options{
        .layout = options.layout,
        .missing = MissingPolicy::KeepNaN,
        .unknown_categorical = UnknownCategoryPolicy::Ignore,
        .positive_threshold = options.positive_threshold,
    };
    StatusOr<Series> pred_or = infer(batch.features, infer_options);
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

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0,
                   static_cast<double>(feature_count_),
                   calibrated_scores_ ? 1.0 : 0.0,
                   calibration_a_,
                   calibration_b_,
                   negative_label_,
                   positive_label_,
                   static_cast<double>(static_cast<std::int64_t>(label_dtype_))}),
        weights_,
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 2) return Status::invalid_argument("LinearClassifier::load_state expects 2 tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 8) return Status::invalid_argument("LinearClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[1]);
    calibrated_scores_ = meta[2] > 0.5;
    calibration_a_ = meta[3];
    calibration_b_ = meta[4];
    negative_label_ = meta[5];
    positive_label_ = meta[6];
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[7]));
    weights_ = state[1];
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

  [[nodiscard]] Series probabilities_from_scores(const Series& scores) const {
    if (!calibrated_scores_) return scores.sigmoid();
    return scores.mul(calibration_a_).add(calibration_b_).sigmoid();
  }

  LinearFamilyOptions options_{};
  bool trained_{false};
  std::int64_t feature_count_{0};
  Series weights_ = Series::undefined();
  bool calibrated_scores_{false};
  double calibration_a_{1.0};
  double calibration_b_{0.0};
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

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    return StatusOr<std::vector<Series>>(std::vector<Series>{
        state_f64({trained_ ? 1.0 : 0.0, static_cast<double>(features_)}),
    });
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.empty()) return Status::invalid_argument("FamilyClassifier::load_state expects metadata tensor.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 2) return Status::invalid_argument("FamilyClassifier::load_state invalid metadata.");
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

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const override {
    std::vector<Series> out{};
    out.reserve(2 + models_.size() * 2);
    out.push_back(state_f64({trained_ ? 1.0 : 0.0,
                             multiclass_ ? 1.0 : 0.0,
                             static_cast<double>(feature_count_),
                             static_cast<double>(static_cast<std::int64_t>(label_dtype_)),
                             static_cast<double>(models_.size())}));
    out.push_back(classes_);
    for (const auto& model : models_) {
      if (!model) {
        out.push_back(state_i64({0}));
        continue;
      }
      StatusOr<std::vector<Series>> child_or = model->dump_state();
      if (!child_or.ok()) return StatusOr<std::vector<Series>>(child_or.status());
      const auto& child = child_or.value();
      out.push_back(state_i64({static_cast<std::int64_t>(child.size())}));
      out.insert(out.end(), child.begin(), child.end());
    }
    return StatusOr<std::vector<Series>>(std::move(out));
  }

  Status load_state(pluss::msharp::RangeView::Series state) override {
    if (state.size() < 2) return Status::invalid_argument("OvRClassifier::load_state expects metadata + classes tensors.");
    const auto meta = vec_f64(state[0]);
    if (meta.size() < 5) return Status::invalid_argument("OvRClassifier::load_state invalid metadata.");
    trained_ = meta[0] > 0.5;
    multiclass_ = meta[1] > 0.5;
    feature_count_ = static_cast<std::int64_t>(meta[2]);
    label_dtype_ = static_cast<ScalarType>(static_cast<std::int64_t>(meta[3]));
    const std::int64_t model_count = static_cast<std::int64_t>(meta[4]);
    classes_ = state[1];

    models_.clear();
    models_.reserve(static_cast<std::size_t>(std::max<std::int64_t>(0, model_count)));
    std::size_t cursor = 2;
    for (std::int64_t mi = 0; mi < model_count; ++mi) {
      if (cursor >= state.size()) return Status::invalid_argument("OvRClassifier::load_state truncated child state.");
      const auto size_vec = vec_i64(state[cursor++]);
      const std::int64_t child_items = size_vec.empty() ? 0 : size_vec.front();
      StatusOr<std::unique_ptr<ClassifierBase>> model_or = make_binary_classifier_impl(spec_);
      if (!model_or.ok()) return model_or.status();
      std::unique_ptr<ClassifierBase> model = model_or.move_value();
      if (child_items > 0) {
        std::vector<Series> child{};
        child.reserve(static_cast<std::size_t>(child_items));
        for (std::int64_t si = 0; si < child_items; ++si) {
          if (cursor >= state.size()) return Status::invalid_argument("OvRClassifier::load_state truncated child tensors.");
          child.push_back(state[cursor++]);
        }
        Status load_status = model->load_state(child);
        if (!load_status.ok()) return load_status;
      }
      models_.push_back(std::move(model));
    }
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
