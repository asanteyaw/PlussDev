#include "pluss/ai/nodej/online.h"
#include "pluss/ai/nodej/balance.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace pluss::ai::nodej::online {
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

[[nodiscard]] bool training_balance_requested(const TrainOptions& options) {
  return options.balance.kind != BalanceTargetKind::Keep
      || options.balance.clean != BalanceCleanKind::None;
}

[[nodiscard]] TrainOptions without_training_balance(TrainOptions options) {
  options.balance = BalanceTarget{};
  return options;
}

[[nodiscard]] StatusOr<LabeledBatch> resolve_online_training_batch(const LabeledBatch& batch,
                                                                   const TrainOptions& options,
                                                                   const char* where) {
  if (!training_balance_requested(options)) return StatusOr<LabeledBatch>(batch);
  StatusOr<LabeledBatch> balanced_or = ::pluss::ai::nodej::balance_rows(batch, options.balance, options.random);
  if (!balanced_or.ok()) {
    Status status = balanced_or.status();
    status.message = std::string(where) + " balance: " + status.message;
    return StatusOr<LabeledBatch>(std::move(status));
  }
  return balanced_or;
}

[[nodiscard]] bool has_nan(const Series& x) {
  return x.isnan().any().item<bool>();
}

struct DenseFeatureMatrix {
  Series x_rows = pluss::series::undefined();
  std::int64_t sample_count = 0;
  std::int64_t feature_count = 0;
  Platform original_platform = pluss::pCPU;
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

  Series x = batch.x.to(ScalarType::Float64);
  if (batch.layout == ObservationLayout::Columns) x = x.transpose(0, 1);
  if (x.sizes().size() != 2 || x.sizes()[0] != out.sample_count || x.sizes()[1] != out.feature_count) {
    return StatusOr<DenseFeatureMatrix>(
        Status::shape_mismatch(std::string(where) + " encountered inconsistent feature matrix shape."));
  }
  out.x_rows = std::move(x);
  return StatusOr<DenseFeatureMatrix>(std::move(out));
}

[[nodiscard]] Series prepend_intercept(const Series& x_rows) {
  const Series ones = pluss::series::ones({x_rows.sizes()[0], 1}, f64_on(x_rows.platform()));
  return ones.unite({x_rows}, 1);
}

struct BinaryLabelEncoding {
  Series y01 = pluss::series::undefined();
  double negative = 0.0;
  double positive = 1.0;
  ScalarType label_dtype = ScalarType::Float64;
};

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

  const Series classes = y64.unique(true).sort(0, false, false);
  if (classes.numel() != 2) {
    return StatusOr<BinaryLabelEncoding>(
        Status::invalid_argument(std::string(where) + " currently requires exactly two distinct classes."));
  }

  const Series idx0 = pluss::series::arange(0, 1, i64_on(classes.platform()));
  const Series idx1 = pluss::series::arange(1, 2, i64_on(classes.platform()));
  const double negative = classes.index_select(0, idx0).reshape({1}).host<double>().front();
  const double positive = classes.index_select(0, idx1).reshape({1}).host<double>().front();
  const Series y01 = y64.eq(positive).to(ScalarType::Float64);

  BinaryLabelEncoding out{};
  out.y01 = y01;
  out.negative = negative;
  out.positive = positive;
  out.label_dtype = batch.y.scalar_type();
  return StatusOr<BinaryLabelEncoding>(std::move(out));
}

[[nodiscard]] StatusOr<Series> map_known_binary_labels(const Series& y_raw,
                                                       std::int64_t expected_samples,
                                                       double negative_label,
                                                       double positive_label,
                                                       const char* where) {
  if (!y_raw.defined()) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " requires a defined label vector."));
  }
  if (y_raw.numel() != expected_samples) {
    return StatusOr<Series>(Status::shape_mismatch(std::string(where) + " label count must match sample count."));
  }
  const Series y64 = y_raw.to(ScalarType::Float64).reshape({expected_samples});
  if (has_nan(y64)) {
    return StatusOr<Series>(Status::invalid_argument(std::string(where) + " does not support NaN labels."));
  }
  const Series is_neg = y64.eq(negative_label).to(ScalarType::Float64);
  const Series is_pos = y64.eq(positive_label).to(ScalarType::Float64);
  const Series valid = is_neg.add(is_pos);
  if (!valid.eq(1.0).all().item<bool>()) {
    return StatusOr<Series>(
        Status::invalid_argument(std::string(where) + " labels must match classes seen during learn()."));
  }
  return StatusOr<Series>(is_pos.reshape({expected_samples}));
}

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
      const Series idx0 = pluss::series::arange(0, 1, i64_on(ridge.platform()));
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
    }
    grad = grad.add(ridge.mul(lambda));
  }
  return grad;
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
      const Series idx0 = pluss::series::arange(0, 1, i64_on(ridge.platform()));
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
    }
    grad = grad.add(ridge.mul(lambda));
  }
  return grad;
}

[[nodiscard]] Series mse_gradient(const Series& design,
                                  const Series& y,
                                  const Series& weights,
                                  bool fit_intercept,
                                  double lambda) {
  const std::int64_t n = design.sizes()[0];
  const Series pred = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series error = pred.sub(y);
  Series grad = design.transpose(0, 1).matmul(error.reshape({n, 1})).mul(2.0 / static_cast<double>(n)).reshape({weights.numel()});

  if (lambda > 0.0) {
    Series ridge = weights;
    if (fit_intercept) {
      const Series idx0 = pluss::series::arange(0, 1, i64_on(ridge.platform()));
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
    }
    grad = grad.add(ridge.mul(lambda));
  }
  return grad;
}

[[nodiscard]] Series robust_gradient(const Series& design,
                                     const Series& y,
                                     const Series& weights,
                                     bool fit_intercept,
                                     double lambda,
                                     double robust_transition) {
  const std::int64_t n = design.sizes()[0];
  const Series pred = design.matmul(weights.reshape({weights.numel(), 1})).reshape({n});
  const Series residual = pred.sub(y);
  const Series abs_residual = residual.abs();
  const Series clipped_grad = abs_residual.le(robust_transition).where(
      residual,
      residual.sign().mul(robust_transition));
  Series grad = design.transpose(0, 1).matmul(clipped_grad.reshape({n, 1})).div(static_cast<double>(n)).reshape({weights.numel()});

  if (lambda > 0.0) {
    Series ridge = weights;
    if (fit_intercept) {
      const Series idx0 = pluss::series::arange(0, 1, i64_on(ridge.platform()));
      ridge = pluss::index_put(ridge, std::vector<Series>{idx0}, pluss::series::zeros({1}, f64_on(ridge.platform())), false);
    }
    grad = grad.add(ridge.mul(lambda));
  }
  return grad;
}

[[nodiscard]] Series labels_from_scores(const Series& scores,
                                        double negative_label,
                                        double positive_label,
                                        Platform out_platform,
                                        ScalarType out_dtype) {
  const Series mask = scores.ge(0.0).to(ScalarType::Float64);
  const Series labels = mask.mul(positive_label - negative_label).add(negative_label);
  return labels.to(out_platform, out_dtype);
}

[[nodiscard]] StatusOr<Series> binary_metrics(const Series& truth01,
                                              const Series& labels01,
                                              const Series& probs,
                                              const std::vector<MetricKind>& requested) {
  const Series y = truth01.to(ScalarType::Float64).reshape({truth01.numel()});
  const Series yh = labels01.to(ScalarType::Float64).reshape({labels01.numel()});
  const Series p = probs.to(ScalarType::Float64).reshape({probs.numel()});
  if (y.numel() == 0 || yh.numel() != y.numel() || p.numel() != y.numel()) {
    return StatusOr<Series>(Status::shape_mismatch("binary assess requires equal non-empty vectors."));
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
  const Series zero = pluss::series::zeros_like(tp, ScalarType::Float64);
  const Series recall = tp.add(fn).gt(0.0).where(tp.div(tp.add(fn)), zero);
  const Series precision = tp.add(fp).gt(0.0).where(tp.div(tp.add(fp)), zero);
  const Series f1 = precision.add(recall).gt(0.0).where(
      precision.mul(recall).mul(2.0).div(precision.add(recall)),
      zero);
  const Series specificity = tn.add(fp).gt(0.0).where(tn.div(tn.add(fp)), zero);
  const Series balanced = recall.add(specificity).mul(0.5);
  const Series geometric = recall.mul(specificity).clamp_min(0.0).sqrt();
  const Series balance_quality = geometric.square().mul(recall.sub(specificity).mul(0.1).add(1.0)).clamp_min(0.0);
  const Series logloss = yb.mul(clipped.log()).add(yb.mul(-1.0).add(1.0).mul(clipped.mul(-1.0).add(1.0).log())).mul(-1.0).mean();
  const Series brier = p.sub(yb).square().mean().reshape({1});
  const Series mcc_num = tp.mul(tn).sub(fp.mul(fn));
  const Series mcc_den = tp.add(fp).mul(tp.add(fn)).mul(tn.add(fp)).mul(tn.add(fn)).clamp_min(0.0).sqrt();
  const Series mcc = mcc_den.gt(0.0).where(mcc_num.div(mcc_den.clamp_min(1.0)), zero).reshape({1});
  const Series n_total = tp.add(tn).add(fp).add(fn);
  const Series kappa_pe =
      tp.add(fp).mul(tp.add(fn)).add(tn.add(fn).mul(tn.add(fp))).div(n_total.mul(n_total).clamp_min(1.0));
  const Series kappa_den = kappa_pe.mul(-1.0).add(1.0);
  const Series kappa_den_safe = kappa_den.add(kappa_den.gt(0.0).where(zero, zero.add(1.0)));  // +1 only when den==0
  const Series cohen_kappa =
      kappa_den.gt(0.0).where(accuracy.reshape({1}).sub(kappa_pe).div(kappa_den_safe), zero).reshape({1});

  std::vector<Series> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Accuracy:
        out.push_back(accuracy.reshape({1}));
        break;
      case MetricKind::BalancedAccuracy:
        out.push_back(balanced.reshape({1}));
        break;
      case MetricKind::Precision:
        out.push_back(precision.reshape({1}));
        break;
      case MetricKind::Recall:
      case MetricKind::Sensitivity:
        out.push_back(recall.reshape({1}));
        break;
      case MetricKind::F1:
        out.push_back(f1.reshape({1}));
        break;
      case MetricKind::Specificity:
        out.push_back(specificity.reshape({1}));
        break;
      case MetricKind::GeometricMean:
        out.push_back(geometric.reshape({1}));
        break;
      case MetricKind::BalanceQuality:
        out.push_back(balance_quality.reshape({1}));
        break;
      case MetricKind::LogLoss:
        out.push_back(logloss.reshape({1}));
        break;
      case MetricKind::Brier:
        out.push_back(brier.reshape({1}));
        break;
      case MetricKind::Mcc:
        out.push_back(mcc.reshape({1}));
        break;
      case MetricKind::CohenKappa:
        out.push_back(cohen_kappa.reshape({1}));
        break;
      default:
        return StatusOr<Series>(Status::invalid_argument(
            "Unsupported classification metric for online linear classifier "
            "(Gini / AveragePrecision need the batch classifier's score sweep)."));
    }
  }
  if (out.empty()) return StatusOr<Series>(pluss::make_series(std::vector<double>{}, {0}, f64_on(y.platform())));
  Series merged = out.front();
  for (std::size_t i = 1; i < out.size(); ++i) merged = merged.unite({out[i]}, 0);
  return StatusOr<Series>(merged.reshape({static_cast<std::int64_t>(out.size())}).to(y.platform(), ScalarType::Float64));
}

[[nodiscard]] StatusOr<Series> regression_metrics(const Series& truth,
                                                  const Series& pred,
                                                  const std::vector<MetricKind>& requested) {
  const Series y = truth.to(ScalarType::Float64).reshape({truth.numel()});
  const Series yp = pred.to(ScalarType::Float64).reshape({pred.numel()});
  if (y.numel() == 0 || yp.numel() != y.numel()) {
    return StatusOr<Series>(Status::shape_mismatch("regression assess requires equal non-empty vectors."));
  }

  const Series err = yp.sub(y);
  const Series mse = err.square().mean();
  const Series rmse = mse.sqrt();
  const Series mae = err.abs().mean();
  const Series y_mean = y.mean();
  const Series sse = err.square().sum();
  const Series sst = y.sub(y_mean).square().sum();
  const Series one = pluss::series::ones({1}, f64_on(y.platform()));
  const Series zero = pluss::series::zeros({1}, f64_on(y.platform()));
  const Series r2 = sst.gt(0.0).where(one.sub(sse.div(sst)), zero);

  std::vector<Series> out{};
  out.reserve(requested.size());
  for (MetricKind metric : requested) {
    switch (metric) {
      case MetricKind::Mse:
        out.push_back(mse.reshape({1}));
        break;
      case MetricKind::Rmse:
        out.push_back(rmse.reshape({1}));
        break;
      case MetricKind::Mae:
        out.push_back(mae.reshape({1}));
        break;
      case MetricKind::R2:
        out.push_back(r2.reshape({1}));
        break;
      default:
        return StatusOr<Series>(Status::invalid_argument("Unsupported regression metric for online linear regressor."));
    }
  }
  if (out.empty()) return StatusOr<Series>(pluss::make_series(std::vector<double>{}, {0}, f64_on(y.platform())));
  Series merged = out.front();
  for (std::size_t i = 1; i < out.size(); ++i) merged = merged.unite({out[i]}, 0);
  return StatusOr<Series>(merged.reshape({static_cast<std::int64_t>(out.size())}).to(y.platform(), ScalarType::Float64));
}

[[nodiscard]] Status validate_classifier_online_options(const OnlineLinearOptions& options) {
  if (options.family.objective != LinearObjective::Logistic && options.family.objective != LinearObjective::Hinge) {
    return Status::invalid_argument("OnlineLinearClassifier requires LinearObjective::Logistic or LinearObjective::Hinge.");
  }
  if (options.family.regularization != RegularizationKind::None && options.family.regularization != RegularizationKind::L2) {
    return Status::invalid_argument("OnlineLinearClassifier supports RegularizationKind::None or RegularizationKind::L2.");
  }
  if (!std::isfinite(options.learning_rate) || options.learning_rate <= 0.0) {
    return Status::invalid_argument("OnlineLinearOptions.learning_rate must be finite and > 0.");
  }
  if (!std::isfinite(options.family.regularization_strength) || options.family.regularization_strength < 0.0) {
    return Status::invalid_argument("LinearFamilyOptions.regularization_strength must be finite and >= 0.");
  }
  return Status::success();
}

[[nodiscard]] Status validate_regressor_online_options(const OnlineLinearOptions& options) {
  if (options.family.objective != LinearObjective::LeastSquares
      && options.family.objective != LinearObjective::RobustLeast) {
    return Status::invalid_argument("OnlineLinearRegressor requires LinearObjective::LeastSquares or LinearObjective::RobustLeast.");
  }
  if (options.family.regularization != RegularizationKind::None && options.family.regularization != RegularizationKind::L2) {
    return Status::invalid_argument("OnlineLinearRegressor supports RegularizationKind::None or RegularizationKind::L2.");
  }
  if (!std::isfinite(options.learning_rate) || options.learning_rate <= 0.0) {
    return Status::invalid_argument("OnlineLinearOptions.learning_rate must be finite and > 0.");
  }
  if (!std::isfinite(options.family.regularization_strength) || options.family.regularization_strength < 0.0) {
    return Status::invalid_argument("LinearFamilyOptions.regularization_strength must be finite and >= 0.");
  }
  if (!(options.family.robust_transition > 0.0) || !std::isfinite(options.family.robust_transition)) {
    return Status::invalid_argument("LinearFamilyOptions.robust_transition must be finite and > 0.");
  }
  return Status::success();
}

} // namespace

OnlineLinearClassifier::OnlineLinearClassifier(const OnlineLinearOptions& options) : options_(options) {}

bool OnlineLinearClassifier::trained() const noexcept {
  return trained_;
}

std::int64_t OnlineLinearClassifier::seen_samples() const noexcept {
  return seen_samples_;
}

Status OnlineLinearClassifier::learn(const LabeledBatch& batch, const TrainOptions& options) {
  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return data_status;
  Status opt_status = validate_classifier_online_options(options_);
  if (!opt_status.ok()) return opt_status;

  StatusOr<LabeledBatch> fit_batch_or = resolve_online_training_batch(batch, options, "OnlineLinearClassifier::learn");
  if (!fit_batch_or.ok()) return fit_batch_or.status();
  const LabeledBatch& fit_batch = fit_batch_or.value();
  const TrainOptions fit_options = without_training_balance(options);

  StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(fit_batch.features, "OnlineLinearClassifier::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseFeatureMatrix matrix = matrix_or.move_value();
  if (has_nan(matrix.x_rows)) {
    return Status::invalid_argument("OnlineLinearClassifier::learn does not support NaN features.");
  }

  StatusOr<BinaryLabelEncoding> labels_or = encode_binary_labels(fit_batch, matrix.sample_count, "OnlineLinearClassifier::learn");
  if (!labels_or.ok()) return labels_or.status();
  const BinaryLabelEncoding labels = labels_or.move_value();

  Series design = matrix.x_rows;
  if (options_.family.fit_intercept) design = prepend_intercept(design);
  weights_ = pluss::series::zeros({design.sizes()[1]}, f64_on(design.platform()));

  feature_count_ = matrix.feature_count;
  negative_label_ = labels.negative;
  positive_label_ = labels.positive;
  label_dtype_ = labels.label_dtype;
  seen_samples_ = 0;
  updates_ = 0;
  trained_ = true;

  const std::int64_t bootstrap_steps = std::max<std::int64_t>(1, options.max_iterations);
  for (std::int64_t i = 0; i < bootstrap_steps; ++i) {
    Status s = advance(fit_batch, fit_options);
    if (!s.ok()) return s;
  }
  return Status::success();
}

Status OnlineLinearClassifier::advance(const LabeledBatch& batch, const TrainOptions& options) {
  (void)options;
  if (!trained_) return Status::precondition("OnlineLinearClassifier::advance requires learn() first.");

  StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "OnlineLinearClassifier::advance");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseFeatureMatrix matrix = matrix_or.move_value();
  if (matrix.feature_count != feature_count_) {
    return Status::shape_mismatch("Feature count mismatch between online state and incoming batch.");
  }
  if (has_nan(matrix.x_rows)) {
    return Status::invalid_argument("OnlineLinearClassifier::advance does not support NaN features.");
  }

  StatusOr<Series> y01_or =
      map_known_binary_labels(batch.y, matrix.sample_count, negative_label_, positive_label_, "OnlineLinearClassifier::advance");
  if (!y01_or.ok()) return y01_or.status();
  const Series y01 = y01_or.move_value();

  Series design = matrix.x_rows;
  if (options_.family.fit_intercept) design = prepend_intercept(design);
  const double lambda = options_.family.regularization == RegularizationKind::L2 ? options_.family.regularization_strength : 0.0;
  Series grad = options_.family.objective == LinearObjective::Logistic
                    ? logistic_gradient(design, y01, weights_, options_.family.fit_intercept, lambda)
                    : hinge_gradient(design, y01, weights_, options_.family.fit_intercept, lambda);
  if (!all_finite(grad)) return Status::numerical_failure("OnlineLinearClassifier::advance produced non-finite gradient.");

  const double rate_scale = options_.decaying_rate ? std::sqrt(1.0 + static_cast<double>(updates_)) : 1.0;
  const double lr = options_.learning_rate / rate_scale;
  weights_ = weights_.sub(grad.mul(lr));
  if (!all_finite(weights_)) return Status::numerical_failure("OnlineLinearClassifier::advance produced non-finite coefficients.");

  seen_samples_ += matrix.sample_count;
  updates_ += 1;
  return Status::success();
}

StatusOr<Series> OnlineLinearClassifier::infer_scores(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  if (!trained_) return StatusOr<Series>(Status::precondition("OnlineLinearClassifier must be learned before infer_scores()."));

  StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "OnlineLinearClassifier::infer_scores");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseFeatureMatrix matrix = matrix_or.move_value();
  if (matrix.feature_count != feature_count_) {
    return StatusOr<Series>(Status::shape_mismatch("Feature count mismatch between online state and inference batch."));
  }
  if (has_nan(matrix.x_rows)) {
    return StatusOr<Series>(Status::invalid_argument("OnlineLinearClassifier::infer_scores does not support NaN features."));
  }

  Series design = matrix.x_rows;
  if (options_.family.fit_intercept) design = prepend_intercept(design);
  const Series scores = design.matmul(weights_.reshape({weights_.numel(), 1})).reshape({matrix.sample_count});
  if (!all_finite(scores)) {
    return StatusOr<Series>(Status::numerical_failure("OnlineLinearClassifier::infer_scores produced non-finite scores."));
  }
  return StatusOr<Series>(scores.to(matrix.original_platform, ScalarType::Float64));
}

StatusOr<Series> OnlineLinearClassifier::infer(const FeatureBatch& batch, const InferOptions& options) const {
  StatusOr<Series> scores_or = infer_scores(batch, options);
  if (!scores_or.ok()) return StatusOr<Series>(scores_or.status());
  return StatusOr<Series>(labels_from_scores(scores_or.value(), negative_label_, positive_label_, batch.x.platform(), label_dtype_));
}

StatusOr<Series> OnlineLinearClassifier::assess(const LabeledBatch& batch, const AssessOptions& options) const {
  if (!trained_) return StatusOr<Series>(Status::precondition("OnlineLinearClassifier must be learned before assess()."));
  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return StatusOr<Series>(data_status);

  StatusOr<Series> y01_or = map_known_binary_labels(
      batch.y, batch.features.sample_count(), negative_label_, positive_label_, "OnlineLinearClassifier::assess");
  if (!y01_or.ok()) return StatusOr<Series>(y01_or.status());
  const Series y01 = y01_or.move_value();

  StatusOr<Series> score_or = infer_scores(batch.features);
  if (!score_or.ok()) return StatusOr<Series>(score_or.status());
  StatusOr<Series> pred_or = infer(batch.features);
  if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());

  const Series pred01 = pred_or.value().to(ScalarType::Float64).eq(positive_label_).to(ScalarType::Float64);
  std::vector<MetricKind> metrics = options.metrics;
  if (metrics.empty()) metrics = {MetricKind::Accuracy};
  return binary_metrics(y01, pred01, score_or.value().sigmoid(), metrics);
}

OnlineLinearRegressor::OnlineLinearRegressor(const OnlineLinearOptions& options) : options_(options) {}

bool OnlineLinearRegressor::trained() const noexcept {
  return trained_;
}

std::int64_t OnlineLinearRegressor::seen_samples() const noexcept {
  return seen_samples_;
}

Status OnlineLinearRegressor::learn(const LabeledBatch& batch, const TrainOptions& options) {
  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return data_status;
  Status opt_status = validate_regressor_online_options(options_);
  if (!opt_status.ok()) return opt_status;

  StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "OnlineLinearRegressor::learn");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseFeatureMatrix matrix = matrix_or.move_value();
  if (has_nan(matrix.x_rows)) {
    return Status::invalid_argument("OnlineLinearRegressor::learn does not support NaN features.");
  }
  const Series y = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
  if (y.numel() != matrix.sample_count) {
    return Status::shape_mismatch("OnlineLinearRegressor::learn label count must match sample count.");
  }
  if (has_nan(y)) {
    return Status::invalid_argument("OnlineLinearRegressor::learn does not support NaN labels.");
  }

  Series design = matrix.x_rows;
  if (options_.family.fit_intercept) design = prepend_intercept(design);
  weights_ = pluss::series::zeros({design.sizes()[1]}, f64_on(design.platform()));

  feature_count_ = matrix.feature_count;
  seen_samples_ = 0;
  updates_ = 0;
  trained_ = true;

  const std::int64_t bootstrap_steps = std::max<std::int64_t>(1, options.max_iterations);
  for (std::int64_t i = 0; i < bootstrap_steps; ++i) {
    Status s = advance(batch, options);
    if (!s.ok()) return s;
  }
  return Status::success();
}

Status OnlineLinearRegressor::advance(const LabeledBatch& batch, const TrainOptions& options) {
  (void)options;
  if (!trained_) return Status::precondition("OnlineLinearRegressor::advance requires learn() first.");

  StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch.features, "OnlineLinearRegressor::advance");
  if (!matrix_or.ok()) return matrix_or.status();
  DenseFeatureMatrix matrix = matrix_or.move_value();
  if (matrix.feature_count != feature_count_) {
    return Status::shape_mismatch("Feature count mismatch between online state and incoming batch.");
  }
  if (has_nan(matrix.x_rows)) {
    return Status::invalid_argument("OnlineLinearRegressor::advance does not support NaN features.");
  }

  const Series y = batch.y.to(ScalarType::Float64).reshape({batch.y.numel()});
  if (y.numel() != matrix.sample_count) {
    return Status::shape_mismatch("OnlineLinearRegressor::advance label count must match sample count.");
  }
  if (has_nan(y)) {
    return Status::invalid_argument("OnlineLinearRegressor::advance does not support NaN labels.");
  }

  Series design = matrix.x_rows;
  if (options_.family.fit_intercept) design = prepend_intercept(design);
  const double lambda = options_.family.regularization == RegularizationKind::L2 ? options_.family.regularization_strength : 0.0;
  const Series grad = options_.family.objective == LinearObjective::RobustLeast
                          ? robust_gradient(design,
                                            y,
                                            weights_,
                                            options_.family.fit_intercept,
                                            lambda,
                                            options_.family.robust_transition)
                          : mse_gradient(design, y, weights_, options_.family.fit_intercept, lambda);
  if (!all_finite(grad)) return Status::numerical_failure("OnlineLinearRegressor::advance produced non-finite gradient.");

  const double rate_scale = options_.decaying_rate ? std::sqrt(1.0 + static_cast<double>(updates_)) : 1.0;
  const double lr = options_.learning_rate / rate_scale;
  weights_ = weights_.sub(grad.mul(lr));
  if (!all_finite(weights_)) return Status::numerical_failure("OnlineLinearRegressor::advance produced non-finite coefficients.");

  seen_samples_ += matrix.sample_count;
  updates_ += 1;
  return Status::success();
}

StatusOr<Series> OnlineLinearRegressor::infer(const FeatureBatch& batch, const InferOptions& options) const {
  (void)options;
  if (!trained_) return StatusOr<Series>(Status::precondition("OnlineLinearRegressor must be learned before infer()."));

  StatusOr<DenseFeatureMatrix> matrix_or = to_dense_rows(batch, "OnlineLinearRegressor::infer");
  if (!matrix_or.ok()) return StatusOr<Series>(matrix_or.status());
  DenseFeatureMatrix matrix = matrix_or.move_value();
  if (matrix.feature_count != feature_count_) {
    return StatusOr<Series>(Status::shape_mismatch("Feature count mismatch between online state and inference batch."));
  }
  if (has_nan(matrix.x_rows)) {
    return StatusOr<Series>(Status::invalid_argument("OnlineLinearRegressor::infer does not support NaN features."));
  }

  Series design = matrix.x_rows;
  if (options_.family.fit_intercept) design = prepend_intercept(design);
  const Series pred = design.matmul(weights_.reshape({weights_.numel(), 1})).reshape({matrix.sample_count});
  if (!all_finite(pred)) {
    return StatusOr<Series>(Status::numerical_failure("OnlineLinearRegressor::infer produced non-finite predictions."));
  }
  return StatusOr<Series>(pred.to(matrix.original_platform, ScalarType::Float64));
}

StatusOr<Series> OnlineLinearRegressor::assess(const LabeledBatch& batch, const AssessOptions& options) const {
  if (!trained_) return StatusOr<Series>(Status::precondition("OnlineLinearRegressor must be learned before assess()."));
  Status data_status = validate_labeled(batch);
  if (!data_status.ok()) return StatusOr<Series>(data_status);

  StatusOr<Series> pred_or = infer(batch.features);
  if (!pred_or.ok()) return StatusOr<Series>(pred_or.status());
  std::vector<MetricKind> metrics = options.metrics;
  if (metrics.empty()) metrics = {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
  return regression_metrics(batch.y, pred_or.value(), metrics);
}

StatusOr<OnlineLinearClassifier> make_online_linear_classifier(const OnlineLinearOptions& options) {
  Status status = validate_classifier_online_options(options);
  if (!status.ok()) return StatusOr<OnlineLinearClassifier>(status);
  return StatusOr<OnlineLinearClassifier>(OnlineLinearClassifier(options));
}

StatusOr<OnlineLinearRegressor> make_online_linear_regressor(const OnlineLinearOptions& options) {
  Status status = validate_regressor_online_options(options);
  if (!status.ok()) return StatusOr<OnlineLinearRegressor>(status);
  return StatusOr<OnlineLinearRegressor>(OnlineLinearRegressor(options));
}

} // namespace pluss::ai::nodej::online
