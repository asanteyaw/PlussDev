#pragma once

#include "pluss/ai/nodej/builders.h"
#include "pluss/ai/nodej/split.h"
#include "MSharp/RangeView.h"

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace pluss::ai::nodej {

struct FoldAssessResult {
  Series metrics = Series::undefined();
  Series train_indices = Series::undefined();
  Series test_indices = Series::undefined();
};

struct CrossAssessReport {
  std::vector<FoldAssessResult> folds{};
  std::vector<MetricKind> metrics{};
  Series mean_metrics = Series::undefined();
  Series std_metrics = Series::undefined();
};

StatusOr<CrossAssessReport> cross_assess(const ModelSpec& spec,
                                         const LabeledBatch& batch,
                                         const SplitOptions& split = {},
                                         const TrainOptions& train_options = {},
                                         const AssessOptions& assess_options = {});

StatusOr<Series> cross_scores(const ModelSpec& spec,
                              const LabeledBatch& batch,
                              MetricKind metric,
                              const SplitOptions& split = {},
                              const TrainOptions& train_options = {},
                              const AssessOptions& assess_options = {});

StatusOr<double> cross_score(const ModelSpec& spec,
                             const LabeledBatch& batch,
                             MetricKind metric,
                             const SplitOptions& split = {},
                             const TrainOptions& train_options = {},
                             const AssessOptions& assess_options = {});

enum class ConfusionNormalize : std::uint8_t {
  None = 0,
  ByTruth = 1,
  ByPrediction = 2,
  Global = 3,
};

struct ConfusionOptions {
  ConfusionNormalize normalize = ConfusionNormalize::None;
  Series label_order = Series::undefined();
  Series sample_weights = Series::undefined();
};

struct ConfusionTable {
  Series labels = Series::undefined();
  Series counts = Series::undefined();
  ConfusionNormalize normalize = ConfusionNormalize::None;
};

struct ClassReportOptions {
  Series label_order = Series::undefined();
  // Fallback value when precision/recall denominators are zero.
  // Keep in [0, 1] for reference-style behavior.
  double zero_division = 0.0;
  Series sample_weights = Series::undefined();
  double balance_quality_alpha = 0.1;
  bool square_balance_quality = true;
};

struct ClassReport {
  Series labels = Series::undefined();
  Series support = Series::undefined();
  Series precision = Series::undefined();
  Series recall = Series::undefined();
  Series f1 = Series::undefined();
  Series specificity = Series::undefined();
  Series geometric_mean = Series::undefined();
  Series balance_quality = Series::undefined();
  Series accuracy = Series::undefined();         // shape [1]
  Series mcc = Series::undefined();              // shape [1] -- Matthews correlation coefficient
  Series cohen_kappa = Series::undefined();      // shape [1] -- unweighted Cohen's kappa
  Series micro_average = Series::undefined();    // shape [3] -> [precision, recall, f1]
  Series macro_average = Series::undefined();    // shape [3] -> [precision, recall, f1]
  Series weighted_average = Series::undefined(); // shape [3] -> [precision, recall, f1]
  Series micro_balance = Series::undefined();     // shape [3] -> [specificity, geometric_mean, balance_quality]
  Series macro_balance = Series::undefined();     // shape [3] -> [specificity, geometric_mean, balance_quality]
  Series weighted_balance = Series::undefined();  // shape [3] -> [specificity, geometric_mean, balance_quality]
};

StatusOr<ConfusionTable> confusion_table(const Series& truth_labels,
                                         const Series& predicted_labels,
                                         const ConfusionOptions& options = {});

StatusOr<ConfusionTable> confusion_table(const Series& truth_labels,
                                         const Series& predicted_labels,
                                         pluss::msharp::RangeView::Of<double> label_order,
                                         ConfusionNormalize normalize = ConfusionNormalize::None);
inline StatusOr<ConfusionTable> confusion_table(const Series& truth_labels,
                                                const Series& predicted_labels,
                                                std::initializer_list<double> label_order,
                                                ConfusionNormalize normalize = ConfusionNormalize::None) {
  return confusion_table(truth_labels,
                         predicted_labels,
                         pluss::msharp::RangeView::Of<double>(label_order),
                         normalize);
}

StatusOr<ClassReport> classification_summary(const Series& truth_labels,
                                             const Series& predicted_labels,
                                             const ClassReportOptions& options = {});

StatusOr<ClassReport> classification_summary(const Series& truth_labels,
                                             const Series& predicted_labels,
                                             pluss::msharp::RangeView::Of<double> label_order,
                                             double zero_division = 0.0);
inline StatusOr<ClassReport> classification_summary(const Series& truth_labels,
                                                    const Series& predicted_labels,
                                                    std::initializer_list<double> label_order,
                                                    double zero_division = 0.0) {
  return classification_summary(truth_labels,
                                predicted_labels,
                                pluss::msharp::RangeView::Of<double>(label_order),
                                zero_division);
}

StatusOr<ConfusionTable> confusion_table(const ClassifierBase& classifier,
                                         const LabeledBatch& batch,
                                         const InferOptions& infer_options = {},
                                         const ConfusionOptions& options = {});

StatusOr<ClassReport> classification_summary(const ClassifierBase& classifier,
                                             const LabeledBatch& batch,
                                             const InferOptions& infer_options = {},
                                             const ClassReportOptions& options = {});

StatusOr<Series> classification_metrics(const Series& truth_labels,
                                        const Series& predicted_labels,
                                        const std::vector<MetricKind>& metrics = {},
                                        const ClassReportOptions& options = {});

StatusOr<Series> classification_metrics(const ClassifierBase& classifier,
                                        const LabeledBatch& batch,
                                        const AssessOptions& options = {});

// Regression counterpart to classification_metrics above, from already-computed
// truth/prediction vectors rather than a model + batch -- the shape a caller
// needs whenever predictions came from somewhere other than a plain
// RegressorBase::infer (e.g. after inverse-transforming them back out of a
// rescaled label space). An empty `metrics` list defaults to {Mse, Rmse, R2},
// matching every other regression assess() default in this file.
StatusOr<Series> regression_metrics(const Series& truth,
                                    const Series& prediction,
                                    const std::vector<MetricKind>& metrics = {});

namespace detail {

// Everything a binary score-vs-label curve (ROC, precision-recall, ...) needs
// falls out of one descending stable sort plus two cumulative sums -- this is
// that shared shape, factored out of what were previously two independent
// copies of the same ~30 lines (family_classifier.cpp's inline RocAuc branch
// and its ovr_auc_binary01 helper). threshold_curve() below is a third
// consumer, not a fourth copy.
struct BinaryScoreCurve {
  Series score_sorted = Series::undefined();          // descending, stable order
  Series true_positive_cumulative = Series::undefined();
  Series false_positive_cumulative = Series::undefined();
  Series is_threshold_boundary = Series::undefined();  // last position of each distinct score
  Series positive_count = Series::undefined();         // shape [1]
  Series negative_count = Series::undefined();         // shape [1]
};

StatusOr<BinaryScoreCurve> binary_score_curve(const Series& scores, const Series& labels01);

} // namespace detail

struct ThresholdCurve {
  Series thresholds = Series::undefined();  // every achievable decision threshold, descending
  Series precision = Series::undefined();
  Series recall = Series::undefined();
  Series f1 = Series::undefined();
};

// One sort + two cumulative sums computes precision/recall/F1 at every
// achievable threshold in a single O(n log n) pass, by sweeping cumulative
// true/false positives rather than re-scoring the model once per candidate
// cutoff.
StatusOr<ThresholdCurve> threshold_curve(const Series& scores, const Series& labels);

// The threshold that maximizes `objective` (Precision, Recall/Sensitivity, or
// F1 -- anything else falls back to F1) over threshold_curve's full sweep.
StatusOr<double> best_threshold(const Series& scores,
                                const Series& labels,
                                MetricKind objective = MetricKind::F1);

// Uninterpolated average precision (area under the precision-recall curve) for
// a binary 0/1 label vector and its positive-class scores:
// AP = Sum (R_n - R_{n-1}) * P_n over the distinct-threshold boundary points,
// the same step-sum sklearn's average_precision_score uses. 0.0 when there are
// no positives. Built on the shared binary_score_curve sort + cumulative sums.
StatusOr<double> average_precision(const Series& scores, const Series& labels);

// Score/probability-based classification metrics from a prediction Series the
// caller already holds -- no model, no inference. `scores` is either a [n]
// vector of positive-class probabilities (binary) or a [n, k] row-stochastic
// probability matrix (multiclass); `labels` is [n], holding 0/1 (binary) or
// class indices 0..k-1 (multiclass). Returns one value per entry of
// `requested`, in order. RocAuc / Gini / AveragePrecision / Brier / LogLoss
// are computed straight from `scores`; any hard-label MetricKind (F1, Mcc, ...)
// is scored against a 0.5-threshold (binary) or argmax (multiclass) prediction,
// matching the model-backed reports path. Regression MetricKinds are rejected.
// Multiclass math is the macro one-vs-rest average, mirroring
// classifier assess().
StatusOr<Series> score_metrics(const Series& scores,
                               const Series& labels,
                               const std::vector<MetricKind>& requested);

} // namespace pluss::ai::nodej
