#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/prep.h"
#include "pluss/ai/lymph/selection_options.h"

namespace pluss::ai::lymph {

namespace metrics {

struct MetricReport {
  std::vector<MetricKind> requested{};
  Series values = pluss::series::undefined();
  Series accuracy = pluss::series::undefined();
  Series balanced_accuracy = pluss::series::undefined();
  Series precision = pluss::series::undefined();
  Series recall = pluss::series::undefined();
  Series sensitivity = pluss::series::undefined();
  Series f1 = pluss::series::undefined();
  Series specificity = pluss::series::undefined();
  Series geometric_mean = pluss::series::undefined();
  Series balance_quality = pluss::series::undefined();
  Series roc_auc = pluss::series::undefined();
  Series logloss = pluss::series::undefined();
  Series mcc = pluss::series::undefined();
  Series cohen_kappa = pluss::series::undefined();
  Series gini = pluss::series::undefined();
  Series average_precision = pluss::series::undefined();
  Series brier = pluss::series::undefined();
  Series mse = pluss::series::undefined();
  Series rmse = pluss::series::undefined();
  Series mae = pluss::series::undefined();
  Series r2 = pluss::series::undefined();
  Series quantile_loss = pluss::series::undefined();
};

struct SeriesCompareReport {
  Series abs_error = pluss::series::undefined();
  Series max_abs_diff = pluss::series::undefined();
  Series mae = pluss::series::undefined();
};

[[nodiscard]] inline SeriesCompareReport compare_series(const Series& lhs, const Series& rhs) {
  const Series a = lhs.to(pluss::pCPU, ScalarType::Float64).reshape({lhs.numel()});
  const Series b = rhs.to(pluss::pCPU, ScalarType::Float64).reshape({rhs.numel()});
  if (a.numel() != b.numel()) {
    ::pluss::throw_error("compare_series: shape mismatch.");
  }
  const Series abs = a.sub(b).abs();
  return SeriesCompareReport{
      .abs_error = abs,
      .max_abs_diff = abs.amax().reshape({1}),
      .mae = abs.mean().reshape({1}),
  };
}

[[nodiscard]] inline double max_abs_diff(const Series& lhs, const Series& rhs) {
  return compare_series(lhs, rhs).max_abs_diff.host<double>().front();
}

struct LinearParameters {
  Series coeff = pluss::series::undefined();
  Series intercept = pluss::series::undefined();
  bool fit_intercept = false;
};

[[nodiscard]] inline Series as_stratify_series(const pluss::table::Slate& slate);

struct CrossValidationReport {
  CrossAssessReport report{};
  MetricReport mean{};
  MetricReport stdev{};

  // One row per requested metric: name, fold mean, fold stdev.
  [[nodiscard]] pluss::table::DataFrame board() const;
};

struct FoldBatchPair {
  LabeledBatch train{};
  LabeledBatch test{};
};

} // namespace metrics

using MetricReport = metrics::MetricReport;
using SeriesCompareReport = metrics::SeriesCompareReport;
using LinearParameters = metrics::LinearParameters;
using CrossValidationReport = metrics::CrossValidationReport;
using FoldBatchPair = metrics::FoldBatchPair;
using metrics::as_stratify_series;
using metrics::compare_series;
using metrics::max_abs_diff;

namespace metrics::detail {

[[nodiscard]] inline Series cross_validate_scores(const ModelSpec& spec,
                                                  const LabeledBatch& batch,
                                                  MetricKind metric,
                                                  selection::CrossValOptions cv = {},
                                                  TrainOptions train_options = {},
                                                  AssessOptions assess_options = {}) {
  SplitOptions split{};
  split.plan = cv.plan;
  split.holdout_fraction = cv.holdout_fraction;
  split.folds = cv.folds;
  split.repeats = cv.repeats;
  split.time_gap = cv.time_gap;
  split.time_test_size = cv.time_test_size;
  split.time_max_train_size = cv.time_max_train_size;
  split.stratified = cv.stratified;
  split.groups = cv.groups;
  split.reliability_defaults = cv.reliability_defaults;
  split.shuffle = cv.shuffle;
  split.random.seed = cv.random_state;
  split.random.deterministic = cv.deterministic;
  return resolve(::pluss::ai::nodej::cross_scores(spec, batch, metric, split, train_options, assess_options),
                 "cross_validate_scores");
}

[[nodiscard]] inline double cross_validate_score(const ModelSpec& spec,
                                                 const LabeledBatch& batch,
                                                 MetricKind metric,
                                                 selection::CrossValOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  SplitOptions split{};
  split.plan = cv.plan;
  split.holdout_fraction = cv.holdout_fraction;
  split.folds = cv.folds;
  split.repeats = cv.repeats;
  split.time_gap = cv.time_gap;
  split.time_test_size = cv.time_test_size;
  split.time_max_train_size = cv.time_max_train_size;
  split.stratified = cv.stratified;
  split.groups = cv.groups;
  split.reliability_defaults = cv.reliability_defaults;
  split.shuffle = cv.shuffle;
  split.random.seed = cv.random_state;
  split.random.deterministic = cv.deterministic;
  return resolve(::pluss::ai::nodej::cross_score(spec, batch, metric, split, train_options, assess_options),
                 "cross_validate_score");
}

[[nodiscard]] inline Series cross_validate_scores(const ModelSpec& spec,
                                                  const pluss::table::DataFrame& frame,
                                                  const DataFrameBatchOptions& data_options,
                                                  MetricKind metric,
                                                  selection::CrossValOptions cv = {},
                                                  TrainOptions train_options = {},
                                                  AssessOptions assess_options = {}) {
  const LabeledBatch batch =
      resolve(labeled_batch_from_dataframe(frame, data_options), "cross_validate_scores.labeled_batch_from_dataframe");
  return cross_validate_scores(spec, batch, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline double cross_validate_score(const ModelSpec& spec,
                                                 const pluss::table::DataFrame& frame,
                                                 const DataFrameBatchOptions& data_options,
                                                 MetricKind metric,
                                                 selection::CrossValOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  const LabeledBatch batch =
      resolve(labeled_batch_from_dataframe(frame, data_options), "cross_validate_score.labeled_batch_from_dataframe");
  return cross_validate_score(spec, batch, metric, cv, train_options, assess_options);
}

} // namespace metrics::detail

[[nodiscard]] inline ClassMix class_mix(const Series& labels) {
  return resolve(::pluss::ai::nodej::class_mix(labels), "class_mix");
}

[[nodiscard]] inline WeightPlan resolve_weight_plan(const Series& labels,
                                                    const Series& batch_sample_weights,
                                                    const TrainOptions& options = {}) {
  return resolve(::pluss::ai::nodej::resolve_weight_plan(labels, batch_sample_weights, options),
                 "resolve_weight_plan");
}

[[nodiscard]] inline Series resolve_training_weights(const Series& labels,
                                                     const Series& batch_sample_weights = pluss::series::undefined(),
                                                     const TrainOptions& options = {}) {
  return resolve(::pluss::ai::nodej::resolve_training_weights(labels, batch_sample_weights, options),
                 "resolve_training_weights");
}

[[nodiscard]] inline BalancePlan plan_balance(const Series& labels,
                                              const BalanceTarget& target,
                                              RandomState random = {}) {
  return resolve(::pluss::ai::nodej::plan_balance(labels, target, random), "plan_balance");
}

[[nodiscard]] inline BalancePlan plan_balance(const LabeledBatch& batch,
                                              const BalanceTarget& target,
                                              RandomState random = {}) {
  return resolve(::pluss::ai::nodej::plan_balance(batch, target, random), "plan_balance");
}

[[nodiscard]] inline LabeledBatch balance_rows(const LabeledBatch& batch,
                                               const BalanceTarget& target,
                                               RandomState random = {}) {
  return resolve(::pluss::ai::nodej::balance_rows(batch, target, random), "balance_rows");
}

// confusion_table(...) and classification_summary(...) -> summary(...) moved
// to `namespace metrics::classification` below (single canonical spelling
// `lymph::metrics::classification::confusion_table` /
// `lymph::metrics::classification::summary`).

[[nodiscard]] inline Series classification_metric_series(const Series& truth_labels,
                                                         const Series& predicted_labels,
                                                         const std::vector<MetricKind>& metrics = {},
                                                         const ClassReportOptions& options = {}) {
  return resolve(::pluss::ai::nodej::classification_metrics(truth_labels, predicted_labels, metrics, options),
                 "classification_metric_series");
}

// Regression counterpart -- from already-computed truth/prediction vectors
// rather than a model + batch, the shape needed whenever predictions came
// from somewhere other than a plain Model::infer
// (Model::assess_series's own target_transform_ branch, detail/model_core.h,
// is the first caller).
[[nodiscard]] inline Series regression_metric_series(const Series& truth,
                                                     const Series& prediction,
                                                     const std::vector<MetricKind>& metrics = {}) {
  return resolve(::pluss::ai::nodej::regression_metrics(truth, prediction, metrics), "regression_metric_series");
}

[[nodiscard]] inline SplitOptions make_validation_split_options(const selection::CrossValOptions& options) {
  SplitOptions split{};
  split.plan = options.plan;
  split.holdout_fraction = options.holdout_fraction;
  split.folds = options.folds;
  split.repeats = options.repeats;
  split.time_gap = options.time_gap;
  split.time_test_size = options.time_test_size;
  split.time_max_train_size = options.time_max_train_size;
  split.stratified = options.stratified;
  split.groups = options.groups;
  split.reliability_defaults = options.reliability_defaults;
  split.shuffle = options.shuffle;
  split.max_parallel = options.max_parallel;
  split.random.seed = options.random_state;
  split.random.deterministic = options.deterministic;
  return split;
}

[[nodiscard]] inline TrainOptions calibrate_with_platt(TrainOptions options = {},
                                                       std::int64_t calibration_iterations = 400,
                                                       double calibration_tolerance = 1e-6) {
  options.calibration = ScoreCalibration::Platt;
  options.calibration_iterations = calibration_iterations;
  options.calibration_tolerance = calibration_tolerance;
  return options;
}

[[nodiscard]] inline TrainOptions calibrate_with_isotonic(TrainOptions options = {}) {
  options.calibration = ScoreCalibration::Isotonic;
  return options;
}

[[nodiscard]] inline TrainOptions calibrate_with_temperature(TrainOptions options = {},
                                                             std::int64_t calibration_iterations = 400,
                                                             double calibration_tolerance = 1e-6) {
  options.calibration = ScoreCalibration::Temperature;
  options.calibration_iterations = calibration_iterations;
  options.calibration_tolerance = calibration_tolerance;
  return options;
}

[[nodiscard]] inline TrainOptions without_calibration(TrainOptions options = {}) {
  options.calibration = ScoreCalibration::None;
  return options;
}

[[nodiscard]] inline MetricReport metric_report_from_series_(const Series& metrics, pluss::msharp::RangeView::Of<MetricKind> requested) {
  MetricReport report{};
  report.requested = copy_range_values(requested);
  report.values = metrics.to(pluss::pCPU, ScalarType::Float64).reshape({metrics.numel()});

  // Slice each named metric straight off `report.values` instead of
  // round-tripping through a host std::vector and rebuilding one fresh
  // 1-element Series per metric.
  const std::size_t n = std::min(static_cast<std::size_t>(report.values.numel()), requested.size());
  for (std::size_t i = 0; i < n; ++i) {
    const Series scalar = report.values.narrow_copy(0, static_cast<std::int64_t>(i), 1).reshape(std::vector<std::int64_t>{});
    switch (requested[i]) {
      case MetricKind::Accuracy: report.accuracy = scalar; break;
      case MetricKind::BalancedAccuracy: report.balanced_accuracy = scalar; break;
      case MetricKind::Precision: report.precision = scalar; break;
      case MetricKind::Recall: report.recall = scalar; break;
      case MetricKind::Sensitivity: report.sensitivity = scalar; break;
      case MetricKind::F1: report.f1 = scalar; break;
      case MetricKind::Specificity: report.specificity = scalar; break;
      case MetricKind::GeometricMean: report.geometric_mean = scalar; break;
      case MetricKind::BalanceQuality: report.balance_quality = scalar; break;
      case MetricKind::RocAuc: report.roc_auc = scalar; break;
      case MetricKind::LogLoss: report.logloss = scalar; break;
      case MetricKind::Mcc: report.mcc = scalar; break;
      case MetricKind::CohenKappa: report.cohen_kappa = scalar; break;
      case MetricKind::Gini: report.gini = scalar; break;
      case MetricKind::AveragePrecision: report.average_precision = scalar; break;
      case MetricKind::Brier: report.brier = scalar; break;
      case MetricKind::Mse: report.mse = scalar; break;
      case MetricKind::Rmse: report.rmse = scalar; break;
      case MetricKind::Mae: report.mae = scalar; break;
      case MetricKind::R2: report.r2 = scalar; break;
      case MetricKind::QuantileLoss: report.quantile_loss = scalar; break;
    }
  }
  return report;
}

namespace metrics {

[[nodiscard]] inline std::vector<MetricKind> default_classification_metrics_() {
  return {MetricKind::Accuracy, MetricKind::BalancedAccuracy, MetricKind::Precision, MetricKind::Recall, MetricKind::F1};
}

[[nodiscard]] inline std::vector<MetricKind> default_regression_metrics_() {
  return {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
}

[[nodiscard]] inline std::vector<MetricKind> default_score_metrics_() {
  return {MetricKind::RocAuc, MetricKind::AveragePrecision, MetricKind::LogLoss, MetricKind::Brier};
}

// Task-scoped metric helpers. Every entry point takes a truth `Series` and a
// prediction (or score) `Series` you already hold -- nothing here trains or
// runs a model. Raw values come off `report(...).values`; there is no separate
// `*_metric_series` spelling.
namespace classification {

// Confusion matrix over hard labels, optionally row/column normalized. The
// one canonical spelling -- `lymph::metrics::classification::confusion_table`.
[[nodiscard]] inline ConfusionTable confusion_table(const Series& truth_labels,
                                                    const Series& predicted_labels,
                                                    const ConfusionOptions& options = {}) {
  return resolve(::pluss::ai::nodej::confusion_table(truth_labels, predicted_labels, options),
                 "metrics::classification::confusion_table");
}

[[nodiscard]] inline ConfusionTable confusion_table(const Series& truth_labels,
                                                    const Series& predicted_labels,
                                                    pluss::msharp::RangeView::Of<double> label_order,
                                                    ConfusionNormalize normalize = ConfusionNormalize::None) {
  return resolve(::pluss::ai::nodej::confusion_table(truth_labels, predicted_labels, label_order, normalize),
                 "metrics::classification::confusion_table");
}

[[nodiscard]] inline ConfusionTable confusion_table(const Series& truth_labels,
                                                    const Series& predicted_labels,
                                                    std::initializer_list<double> label_order,
                                                    ConfusionNormalize normalize = ConfusionNormalize::None) {
  return classification::confusion_table(truth_labels,
                                         predicted_labels,
                                         pluss::msharp::RangeView::Of<double>(label_order),
                                         normalize);
}

// Per-class precision / recall / f1 / support plus the aggregate rollups.
[[nodiscard]] inline ClassReport summary(const Series& truth_labels,
                                         const Series& predicted_labels,
                                         const ClassReportOptions& options = {}) {
  return resolve(::pluss::ai::nodej::classification_summary(truth_labels, predicted_labels, options),
                 "metrics::classification::summary");
}

[[nodiscard]] inline ClassReport summary(const Series& truth_labels,
                                         const Series& predicted_labels,
                                         pluss::msharp::RangeView::Of<double> label_order,
                                         double zero_division = 0.0) {
  return resolve(::pluss::ai::nodej::classification_summary(truth_labels, predicted_labels, label_order, zero_division),
                 "metrics::classification::summary");
}

[[nodiscard]] inline ClassReport summary(const Series& truth_labels,
                                         const Series& predicted_labels,
                                         std::initializer_list<double> label_order,
                                         double zero_division = 0.0) {
  return classification::summary(truth_labels,
                                 predicted_labels,
                                 pluss::msharp::RangeView::Of<double>(label_order),
                                 zero_division);
}

// Requested scalar metrics as a MetricReport. `requested` empty -> the task
// default set. Read `report.values` for the raw Series, or a named slot
// (`report.f1`, `report.mcc`, ...) with `.item<double>()`.
[[nodiscard]] inline MetricReport report(const Series& truth_labels,
                                         const Series& predicted_labels,
                                         std::vector<MetricKind> requested = {},
                                         const ClassReportOptions& options = {}) {
  if (requested.empty()) requested = default_classification_metrics_();
  return ::pluss::ai::lymph::metric_report_from_series_(
      ::pluss::ai::lymph::classification_metric_series(truth_labels, predicted_labels, requested, options),
      requested);
}

// Score-based scalar metrics from a probability `Series` you already hold, with
// no model in the loop -- the score-side twin of `report`. `scores` is a [n]
// vector of positive-class probabilities (binary) or a [n, k] row-stochastic
// probability matrix (multiclass); `labels` is [n] (0/1, or class indices).
// `requested` empty -> {RocAuc, AveragePrecision, LogLoss, Brier}. Hard-label
// kinds (F1, Mcc, ...) are also accepted and scored at a 0.5 threshold /
// argmax, matching `model.reports().metric_series(...)`.
[[nodiscard]] inline MetricReport score_report(const Series& scores,
                                               const Series& labels,
                                               std::vector<MetricKind> requested = {}) {
  if (requested.empty()) requested = default_score_metrics_();
  return ::pluss::ai::lymph::metric_report_from_series_(
      resolve(::pluss::ai::nodej::score_metrics(scores, labels, requested),
              "metrics::classification::score_report"),
      requested);
}

// Operating-point accessor over a score / label pair: the full
// precision-recall-threshold `curve()`, or the single `best()` threshold for
// an objective.
class Threshold final {
 public:
  Threshold(Series scores, Series labels)
      : scores_(std::move(scores)),
        labels_(std::move(labels)) {}

  [[nodiscard]] const Series& scores() const noexcept { return scores_; }
  [[nodiscard]] const Series& labels() const noexcept { return labels_; }

  [[nodiscard]] ThresholdCurve curve() const {
    return resolve(::pluss::ai::nodej::threshold_curve(scores_, labels_),
                   "metrics::classification::Threshold::curve");
  }

  [[nodiscard]] double best(MetricKind objective = MetricKind::F1) const {
    return resolve(::pluss::ai::nodej::best_threshold(scores_, labels_, objective),
                   "metrics::classification::Threshold::best");
  }

 private:
  Series scores_ = pluss::series::undefined();
  Series labels_ = pluss::series::undefined();
};

[[nodiscard]] inline Threshold threshold(Series scores, Series labels) {
  return Threshold(std::move(scores), std::move(labels));
}

} // namespace classification

namespace regression {

// Requested scalar regression metrics as a MetricReport. `requested` empty ->
// the task default set. Read `report.values` for the raw Series.
[[nodiscard]] inline MetricReport report(const Series& truth,
                                         const Series& prediction,
                                         std::vector<MetricKind> requested = {}) {
  if (requested.empty()) requested = default_regression_metrics_();
  return ::pluss::ai::lymph::metric_report_from_series_(
      ::pluss::ai::lymph::regression_metric_series(truth, prediction, requested),
      requested);
}

} // namespace regression

} // namespace metrics

[[nodiscard]] inline std::string metric_kind_name_(MetricKind metric) {
  switch (metric) {
    case MetricKind::Accuracy: return "accuracy";
    case MetricKind::BalancedAccuracy: return "balanced_accuracy";
    case MetricKind::Precision: return "precision";
    case MetricKind::Recall: return "recall";
    case MetricKind::Sensitivity: return "sensitivity";
    case MetricKind::F1: return "f1";
    case MetricKind::Specificity: return "specificity";
    case MetricKind::GeometricMean: return "geometric_mean";
    case MetricKind::BalanceQuality: return "balance_quality";
    case MetricKind::RocAuc: return "roc_auc";
    case MetricKind::LogLoss: return "logloss";
    case MetricKind::Mcc: return "mcc";
    case MetricKind::CohenKappa: return "cohen_kappa";
    case MetricKind::Gini: return "gini";
    case MetricKind::AveragePrecision: return "average_precision";
    case MetricKind::Brier: return "brier";
    case MetricKind::Mse: return "mse";
    case MetricKind::Rmse: return "rmse";
    case MetricKind::Mae: return "mae";
    case MetricKind::R2: return "r2";
    case MetricKind::QuantileLoss: return "quantile_loss";
  }
  return "metric";
}

[[nodiscard]] inline pluss::table::DataFrame metrics::CrossValidationReport::board() const {
  std::vector<std::string> metric_names{};
  std::vector<double> mean_values{};
  std::vector<double> stdev_values{};
  metric_names.reserve(mean.requested.size());
  mean_values.reserve(mean.requested.size());
  stdev_values.reserve(mean.requested.size());

  const auto mean_vec = mean.values.to(pluss::pCPU, ScalarType::Float64).reshape({mean.values.numel()}).host<double>().array().data;
  const auto stdev_vec = stdev.values.to(pluss::pCPU, ScalarType::Float64).reshape({stdev.values.numel()}).host<double>().array().data;
  for (std::size_t i = 0; i < mean.requested.size(); ++i) {
    metric_names.push_back(metric_kind_name_(mean.requested[i]));
    mean_values.push_back(mean_vec[i]);
    stdev_values.push_back(stdev_vec[i]);
  }

  const Config f64_cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto count = static_cast<std::int64_t>(metric_names.size());
  return pluss::table::DataFrame({
      {"metric", pluss::table::Slate(pluss::make_note(metric_names))},
      {"mean", pluss::table::Slate(pluss::make_series(mean_values, {count}, f64_cfg))},
      {"stdev", pluss::table::Slate(pluss::make_series(stdev_values, {count}, f64_cfg))},
  }).title("Cross-Validation Results");
}

[[nodiscard]] inline std::vector<MetricKind> default_assess_metrics_(TaskKind task, const AssessOptions& options) {
  if (!options.metrics.empty()) return options.metrics;
  if (task == TaskKind::Regression) return {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
  if (task == TaskKind::Classification) return {MetricKind::Accuracy};
  return {};
}

[[nodiscard]] inline std::vector<std::string> cross_validate_default_feature_names_(
    const pluss::table::DataFrame& frame,
    const char* where) {
  const auto names = frame.feataxis().series().features();
  if (names.empty()) {
    ::pluss::throw_error(std::string(where) + ": input DataFrame has no Series features.");
  }
  return names;
}

[[nodiscard]] inline DataFrameBatchOptions cross_validate_default_data_options_(pluss::msharp::RangeView::Of<std::string> names) {
  DataFrameBatchOptions options{};
  options.feature_names = copy_range_values(names);
  options.layout = ObservationLayout::Rows;
  options.slate_policy = DataSlatePolicy::CastTemporalToSeries;
  options.note_policy = DataNotePolicy::Reject;
  return options;
}

[[nodiscard]] inline LabeledBatch cross_validate_labeled_from_xy_(const pluss::table::DataFrame& x,
                                                                  const Series& y,
                                                                  pluss::msharp::RangeView::Of<std::string> feature_names,
                                                                  const char* where) {
  const auto x_batch = resolve(feature_batch_from_dataframe(x, cross_validate_default_data_options_(feature_names)),
                               std::string(where) + ".feature_batch_from_dataframe");
  const Series y_aligned = y.to(x_batch.x.platform(), ScalarType::Float64).reshape({x_batch.sample_count()});
  LabeledBatch batch{
      .features = x_batch,
      .y = y_aligned,
      .sample_weights = pluss::series::undefined(),
  };
  resolve(validate_labeled(batch), std::string(where) + ".validate_labeled");
  return batch;
}

// plan_balance(labels) rejects BlendNeighbors/cleaning outright (they need
// real feature vectors to find/blend neighbors) -- these DataFrame overloads
// build the LabeledBatch every growth mode actually supports, matching the
// convenience shape cross_validate/classification_summary/confusion_table
// already offer, so reaching for the wrong overload isn't a runtime surprise.
[[nodiscard]] inline BalancePlan plan_balance(const pluss::table::DataFrame& x,
                                              const Series& y,
                                              const BalanceTarget& target,
                                              RandomState random = {}) {
  const auto names = cross_validate_default_feature_names_(x, "plan_balance(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "plan_balance(x,y)");
  return ::pluss::ai::lymph::plan_balance(batch, target, random);
}

[[nodiscard]] inline BalancePlan plan_balance(const pluss::table::DataFrame& x,
                                              const pluss::table::Slate& y,
                                              const BalanceTarget& target,
                                              RandomState random = {}) {
  return plan_balance(x, as_stratify_series(y), target, random);
}

// (frame, DataFrameBatchOptions) sibling of the (x, y) overloads above --
// the caller's own `data_options` (e.g. `note_policy = DataNotePolicy::
// Factorize`) is threaded straight into `labeled_batch_from_dataframe`
// instead of the (x, y) forms' hardcoded Reject-policy default, so a raw
// Note column can take part in balancing without a separate pre-factorize
// step. plan_balance only ever reads this one frame to decide how to
// resample it -- no train/inference persistence is involved -- so a fresh
// per-call fit (no existing/fitted category map threading) is correct here,
// unlike Model::learn_/infer's frozen-map requirement.
[[nodiscard]] inline BalancePlan plan_balance(const pluss::table::DataFrame& frame,
                                              const DataFrameBatchOptions& data_options,
                                              const BalanceTarget& target,
                                              RandomState random = {}) {
  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options),
                                     "plan_balance(frame,data_options).labeled_batch_from_dataframe");
  return ::pluss::ai::lymph::plan_balance(batch, target, random);
}

// class_mix only ever needs the label column -- pull it by name instead of
// requiring a caller to already have it as a bare Series.
[[nodiscard]] inline ClassMix class_mix(const pluss::table::DataFrame& frame, std::string_view label_feature) {
  return ::pluss::ai::lymph::class_mix(frame.features(label_feature).as_series().reshape({frame.ncases()}));
}

namespace metrics::detail {

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelSpec& spec,
                                                          const LabeledBatch& batch,
                                                          selection::CrossValOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const SplitOptions split = make_validation_split_options(cv);
  CrossAssessReport report =
      resolve(pluss::ai::nodej::cross_assess(spec, batch, split, train_options, assess_options), "cross_assess");
  const auto metrics = default_assess_metrics_(spec.task, assess_options);
  return CrossValidationReport{
      .report = report,
      .mean = metric_report_from_series_(report.mean_metrics, metrics),
      .stdev = metric_report_from_series_(report.std_metrics, metrics),
  };
}

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelSpec& spec,
                                                          const pluss::table::DataFrame& frame,
                                                          const DataFrameBatchOptions& data_options,
                                                          selection::CrossValOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options), "cross_validate.labeled_batch_from_dataframe");
  return cross_validate(spec, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelSpec& spec,
                                                          const pluss::table::DataFrame& x,
                                                          const Series& y,
                                                          selection::CrossValOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "cross_validate(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "cross_validate(x,y)");
  return cross_validate(spec, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelSpec& spec,
                                                          const pluss::table::DataFrame& x,
                                                          const pluss::table::Slate& y,
                                                          selection::CrossValOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(spec, x, as_stratify_series(y), cv, train_options, assess_options);
}

[[nodiscard]] inline Series cross_validate_scores(const ModelSpec& spec,
                                                  const pluss::table::DataFrame& x,
                                                  const Series& y,
                                                  MetricKind metric,
                                                  selection::CrossValOptions cv = {},
                                                  TrainOptions train_options = {},
                                                  AssessOptions assess_options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "cross_validate_scores(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "cross_validate_scores(x,y)");
  return cross_validate_scores(spec, batch, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline double cross_validate_score(const ModelSpec& spec,
                                                 const pluss::table::DataFrame& x,
                                                 const Series& y,
                                                 MetricKind metric,
                                                 selection::CrossValOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "cross_validate_score(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "cross_validate_score(x,y)");
  return cross_validate_score(spec, batch, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline Series cross_validate_scores(const ModelSpec& spec,
                                                  const pluss::table::DataFrame& x,
                                                  const pluss::table::Slate& y,
                                                  MetricKind metric,
                                                  selection::CrossValOptions cv = {},
                                                  TrainOptions train_options = {},
                                                  AssessOptions assess_options = {}) {
  return cross_validate_scores(spec, x, as_stratify_series(y), metric, cv, train_options, assess_options);
}

[[nodiscard]] inline double cross_validate_score(const ModelSpec& spec,
                                                 const pluss::table::DataFrame& x,
                                                 const pluss::table::Slate& y,
                                                 MetricKind metric,
                                                 selection::CrossValOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  return cross_validate_score(spec, x, as_stratify_series(y), metric, cv, train_options, assess_options);
}

} // namespace metrics::detail

[[nodiscard]] inline Series metrics::as_stratify_series(const pluss::table::Slate& slate) {
  return ::pluss::ai::lymph::prep::as_stratify_series(slate);
}

} // namespace pluss::ai::lymph
