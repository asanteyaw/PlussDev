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
};

struct ConfusionTable {
  Series labels = Series::undefined();
  Series counts = Series::undefined();
  ConfusionNormalize normalize = ConfusionNormalize::None;
};

struct ClassReportOptions {
  Series label_order = Series::undefined();
  // Fallback value when precision/recall denominators are zero.
  // Keep in [0, 1] for sklearn-style behavior.
  double zero_division = 0.0;
};

struct ClassReport {
  Series labels = Series::undefined();
  Series support = Series::undefined();
  Series precision = Series::undefined();
  Series recall = Series::undefined();
  Series f1 = Series::undefined();
  Series accuracy = Series::undefined();         // shape [1]
  Series micro_average = Series::undefined();    // shape [3] -> [precision, recall, f1]
  Series macro_average = Series::undefined();    // shape [3] -> [precision, recall, f1]
  Series weighted_average = Series::undefined(); // shape [3] -> [precision, recall, f1]
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

StatusOr<ClassReport> classification_report(const Series& truth_labels,
                                            const Series& predicted_labels,
                                            const ClassReportOptions& options = {});

StatusOr<ClassReport> classification_report(const Series& truth_labels,
                                            const Series& predicted_labels,
                                            pluss::msharp::RangeView::Of<double> label_order,
                                            double zero_division = 0.0);
inline StatusOr<ClassReport> classification_report(const Series& truth_labels,
                                                   const Series& predicted_labels,
                                                   std::initializer_list<double> label_order,
                                                   double zero_division = 0.0) {
  return classification_report(truth_labels,
                               predicted_labels,
                               pluss::msharp::RangeView::Of<double>(label_order),
                               zero_division);
}

StatusOr<ConfusionTable> confusion_table(const ClassifierBase& classifier,
                                         const LabeledBatch& batch,
                                         const InferOptions& infer_options = {},
                                         const ConfusionOptions& options = {});

StatusOr<ClassReport> classification_report(const ClassifierBase& classifier,
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

} // namespace pluss::ai::nodej
