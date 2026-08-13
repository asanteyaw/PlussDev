#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/prep.h"

namespace pluss::ai::lymph {

namespace metrics {

struct MetricReport {
  std::vector<MetricKind> requested{};
  Series values = Series::undefined();
  Series accuracy = Series::undefined();
  Series balanced_accuracy = Series::undefined();
  Series precision = Series::undefined();
  Series recall = Series::undefined();
  Series sensitivity = Series::undefined();
  Series f1 = Series::undefined();
  Series specificity = Series::undefined();
  Series geometric_mean = Series::undefined();
  Series balance_quality = Series::undefined();
  Series roc_auc = Series::undefined();
  Series logloss = Series::undefined();
  Series mse = Series::undefined();
  Series rmse = Series::undefined();
  Series mae = Series::undefined();
  Series r2 = Series::undefined();
  Series quantile_loss = Series::undefined();
};

struct SeriesCompareReport {
  Series abs_error = Series::undefined();
  Series max_abs_diff = Series::undefined();
  Series mae = Series::undefined();
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
  return compare_series(lhs, rhs).max_abs_diff.item<double>();
}

struct LinearParameters {
  Series coeff = Series::undefined();
  Series intercept = Series::undefined();
  bool fit_intercept = false;
};

[[nodiscard]] inline Series as_stratify_series(const pluss::table::Thread& thread);

struct CrossValidationOptions {
  CVPlanKind plan = CVPlanKind::CrossVal;
  double holdout_fraction = 0.2;
  std::int64_t folds = 5;
  std::int64_t repeats = 1;
  std::int64_t time_gap = 0;
  std::int64_t time_test_size = 0;
  std::int64_t time_max_train_size = 0;
  bool stratified = false;
  Series groups = Series::undefined();
  bool reliability_defaults = true;
  bool shuffle = true;
  // 0 = auto-detect (one worker per fold, capped at hardware_concurrency());
  // each fold trains an independent out-of-place clone of the model, so
  // parallelizing changes nothing about the result, only how long it takes
  // to get it. Set to 1 to force serial evaluation, or a specific count to
  // cap worker threads.
  std::int64_t max_parallel = 0;
  std::uint64_t random_state = 0;
  bool deterministic = true;
};

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
using CrossValidationOptions = metrics::CrossValidationOptions;
using CrossValidationReport = metrics::CrossValidationReport;
using FoldBatchPair = metrics::FoldBatchPair;
using metrics::as_stratify_series;
using metrics::compare_series;
using metrics::max_abs_diff;

[[nodiscard]] inline Series cross_validate_scores(const ModelSpec& spec,
                                                  const LabeledBatch& batch,
                                                  MetricKind metric,
                                                  CrossValidationOptions cv = {},
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
                                                 CrossValidationOptions cv = {},
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
                                                  CrossValidationOptions cv = {},
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
                                                 CrossValidationOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  const LabeledBatch batch =
      resolve(labeled_batch_from_dataframe(frame, data_options), "cross_validate_score.labeled_batch_from_dataframe");
  return cross_validate_score(spec, batch, metric, cv, train_options, assess_options);
}

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
                                                     const Series& batch_sample_weights = Series::undefined(),
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

[[nodiscard]] inline ConfusionTable confusion_table(const Series& truth_labels,
                                                    const Series& predicted_labels,
                                                    const ConfusionOptions& options = {}) {
  return resolve(::pluss::ai::nodej::confusion_table(truth_labels, predicted_labels, options), "confusion_table");
}

[[nodiscard]] inline ConfusionTable confusion_table(const Series& truth_labels,
                                                    const Series& predicted_labels,
                                                    pluss::msharp::RangeView::Of<double> label_order,
                                                    ConfusionNormalize normalize = ConfusionNormalize::None) {
  return resolve(::pluss::ai::nodej::confusion_table(truth_labels, predicted_labels, label_order, normalize),
                 "confusion_table");
}

[[nodiscard]] inline ConfusionTable confusion_table(const Series& truth_labels,
                                                    const Series& predicted_labels,
                                                    std::initializer_list<double> label_order,
                                                    ConfusionNormalize normalize = ConfusionNormalize::None) {
  return ::pluss::ai::lymph::confusion_table(truth_labels,
                                             predicted_labels,
                                             pluss::msharp::RangeView::Of<double>(label_order),
                                             normalize);
}

[[nodiscard]] inline ClassReport classification_summary(const Series& truth_labels,
                                                        const Series& predicted_labels,
                                                        const ClassReportOptions& options = {}) {
  return resolve(::pluss::ai::nodej::classification_summary(truth_labels, predicted_labels, options),
                 "classification_summary");
}

[[nodiscard]] inline ClassReport classification_summary(const Series& truth_labels,
                                                        const Series& predicted_labels,
                                                        pluss::msharp::RangeView::Of<double> label_order,
                                                        double zero_division = 0.0) {
  return resolve(::pluss::ai::nodej::classification_summary(truth_labels, predicted_labels, label_order, zero_division),
                 "classification_summary");
}

[[nodiscard]] inline ClassReport classification_summary(const Series& truth_labels,
                                                        const Series& predicted_labels,
                                                        std::initializer_list<double> label_order,
                                                        double zero_division = 0.0) {
  return ::pluss::ai::lymph::classification_summary(truth_labels,
                                                    predicted_labels,
                                                    pluss::msharp::RangeView::Of<double>(label_order),
                                                    zero_division);
}

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

[[nodiscard]] inline SplitOptions make_cross_validation_split_options(const CrossValidationOptions& options) {
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
    const Series scalar = report.values.narrow_copy(0, static_cast<std::int64_t>(i), 1);
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

class Classification final {
 public:
  Classification(Series truth_labels, Series predicted_labels, ClassReportOptions options = {})
      : truth_(std::move(truth_labels)),
        predicted_(std::move(predicted_labels)),
        options_(std::move(options)) {}

  [[nodiscard]] const Series& truth() const noexcept { return truth_; }
  [[nodiscard]] const Series& prediction() const noexcept { return predicted_; }
  [[nodiscard]] const ClassReportOptions& options() const noexcept { return options_; }

  [[nodiscard]] ConfusionTable confusion_table(ConfusionOptions options = {}) const {
    if (!options.label_order.defined()) options.label_order = options_.label_order;
    if (!options.sample_weights.defined()) options.sample_weights = options_.sample_weights;
    return ::pluss::ai::lymph::confusion_table(truth_, predicted_, options);
  }

  [[nodiscard]] ClassReport summary() const {
    return ::pluss::ai::lymph::classification_summary(truth_, predicted_, options_);
  }

  [[nodiscard]] ClassReport summary(ClassReportOptions options) const {
    return ::pluss::ai::lymph::classification_summary(truth_, predicted_, std::move(options));
  }

  [[nodiscard]] Series values(std::vector<MetricKind> requested = {}) const {
    if (requested.empty()) requested = default_classification_metrics_();
    return ::pluss::ai::lymph::classification_metric_series(truth_, predicted_, requested, options_);
  }

  [[nodiscard]] MetricReport report(std::vector<MetricKind> requested = {}) const {
    if (requested.empty()) requested = default_classification_metrics_();
    return ::pluss::ai::lymph::metric_report_from_series_(values(requested), requested);
  }

 private:
  Series truth_ = Series::undefined();
  Series predicted_ = Series::undefined();
  ClassReportOptions options_{};
};

class Regression final {
 public:
  Regression(Series truth, Series prediction)
      : truth_(std::move(truth)),
        prediction_(std::move(prediction)) {}

  [[nodiscard]] const Series& truth() const noexcept { return truth_; }
  [[nodiscard]] const Series& prediction() const noexcept { return prediction_; }

  [[nodiscard]] Series values(std::vector<MetricKind> requested = {}) const {
    if (requested.empty()) requested = default_regression_metrics_();
    return ::pluss::ai::lymph::regression_metric_series(truth_, prediction_, requested);
  }

  [[nodiscard]] MetricReport report(std::vector<MetricKind> requested = {}) const {
    if (requested.empty()) requested = default_regression_metrics_();
    return ::pluss::ai::lymph::metric_report_from_series_(values(requested), requested);
  }

 private:
  Series truth_ = Series::undefined();
  Series prediction_ = Series::undefined();
};

class Threshold final {
 public:
  Threshold(Series scores, Series labels)
      : scores_(std::move(scores)),
        labels_(std::move(labels)) {}

  [[nodiscard]] const Series& scores() const noexcept { return scores_; }
  [[nodiscard]] const Series& labels() const noexcept { return labels_; }

  [[nodiscard]] ThresholdCurve curve() const {
    return resolve(::pluss::ai::nodej::threshold_curve(scores_, labels_), "metrics::Threshold::curve");
  }

  [[nodiscard]] double best(MetricKind objective = MetricKind::F1) const {
    return resolve(::pluss::ai::nodej::best_threshold(scores_, labels_, objective), "metrics::Threshold::best");
  }

 private:
  Series scores_ = Series::undefined();
  Series labels_ = Series::undefined();
};

[[nodiscard]] inline Classification classification(Series truth_labels,
                                                   Series predicted_labels,
                                                   ClassReportOptions options = {}) {
  return Classification(std::move(truth_labels), std::move(predicted_labels), std::move(options));
}

[[nodiscard]] inline Regression regression(Series truth, Series prediction) {
  return Regression(std::move(truth), std::move(prediction));
}

[[nodiscard]] inline Threshold threshold(Series scores, Series labels) {
  return Threshold(std::move(scores), std::move(labels));
}

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

  const auto mean_vec = mean.values.to(pluss::pCPU, ScalarType::Float64).reshape({mean.values.numel()}).to_vec<double>();
  const auto stdev_vec = stdev.values.to(pluss::pCPU, ScalarType::Float64).reshape({stdev.values.numel()}).to_vec<double>();
  for (std::size_t i = 0; i < mean.requested.size(); ++i) {
    metric_names.push_back(metric_kind_name_(mean.requested[i]));
    mean_values.push_back(mean_vec[i]);
    stdev_values.push_back(stdev_vec[i]);
  }

  const Config f64_cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const auto count = static_cast<std::int64_t>(metric_names.size());
  return pluss::table::DataFrame({
      {"metric", pluss::table::Thread(pluss::make_note(metric_names))},
      {"mean", pluss::table::Thread(pluss::make_series(mean_values, {count}, f64_cfg))},
      {"stdev", pluss::table::Thread(pluss::make_series(stdev_values, {count}, f64_cfg))},
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
  options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
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
      .sample_weights = Series::undefined(),
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
                                              const pluss::table::Thread& y,
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

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelSpec& spec,
                                                          const LabeledBatch& batch,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const SplitOptions split = make_cross_validation_split_options(cv);
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
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options), "cross_validate.labeled_batch_from_dataframe");
  return cross_validate(spec, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelSpec& spec,
                                                          const pluss::table::DataFrame& x,
                                                          const Series& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "cross_validate(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "cross_validate(x,y)");
  return cross_validate(spec, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelSpec& spec,
                                                          const pluss::table::DataFrame& x,
                                                          const pluss::table::Thread& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(spec, x, as_stratify_series(y), cv, train_options, assess_options);
}

[[nodiscard]] inline Series cross_validate_scores(const ModelSpec& spec,
                                                  const pluss::table::DataFrame& x,
                                                  const Series& y,
                                                  MetricKind metric,
                                                  CrossValidationOptions cv = {},
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
                                                 CrossValidationOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "cross_validate_score(x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "cross_validate_score(x,y)");
  return cross_validate_score(spec, batch, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline Series cross_validate_scores(const ModelSpec& spec,
                                                  const pluss::table::DataFrame& x,
                                                  const pluss::table::Thread& y,
                                                  MetricKind metric,
                                                  CrossValidationOptions cv = {},
                                                  TrainOptions train_options = {},
                                                  AssessOptions assess_options = {}) {
  return cross_validate_scores(spec, x, as_stratify_series(y), metric, cv, train_options, assess_options);
}

[[nodiscard]] inline double cross_validate_score(const ModelSpec& spec,
                                                 const pluss::table::DataFrame& x,
                                                 const pluss::table::Thread& y,
                                                 MetricKind metric,
                                                 CrossValidationOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  return cross_validate_score(spec, x, as_stratify_series(y), metric, cv, train_options, assess_options);
}

[[nodiscard]] inline Series metrics::as_stratify_series(const pluss::table::Thread& thread) {
  return ::pluss::ai::lymph::prep::as_stratify_series(thread);
}

} // namespace pluss::ai::lymph
