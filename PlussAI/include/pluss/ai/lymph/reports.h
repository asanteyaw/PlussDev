#pragma once

#include "pluss/ai/lymph/model.h"

// reports.h -- reporting family for model-backed classification summaries.
//
// The underlying implementations still live beside ModelCore because they
// need access to fitted category maps and staged inference internals. This
// header is the public family boundary: callers consume a public Model and
// receive report values, while Model itself does not grow reporting methods.

namespace pluss::ai::lymph::reports {

[[nodiscard]] inline ConfusionTable confusion_table(const Model& model,
                                                    const LabeledBatch& batch,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  return detail::confusion_table(detail::model_core(model), batch, infer_options, options);
}

[[nodiscard]] inline ConfusionTable confusion_table(const Model& model,
                                                    const pluss::table::DataFrame& x,
                                                    const Series& y,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  return detail::confusion_table(detail::model_core(model), x, y, infer_options, options);
}

[[nodiscard]] inline ConfusionTable confusion_table(const Model& model,
                                                    const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& data_options,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  return detail::confusion_table(detail::model_core(model), frame, data_options, infer_options, options);
}

[[nodiscard]] inline ClassReport classification_summary(const Model& model,
                                                        const LabeledBatch& batch,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  return detail::classification_summary(detail::model_core(model), batch, infer_options, options);
}

[[nodiscard]] inline ClassReport classification_summary(const Model& model,
                                                        const pluss::table::DataFrame& x,
                                                        const Series& y,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  return detail::classification_summary(detail::model_core(model), x, y, infer_options, options);
}

[[nodiscard]] inline ClassReport classification_summary(const Model& model,
                                                        const pluss::table::DataFrame& frame,
                                                        const DataFrameBatchOptions& data_options,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  return detail::classification_summary(detail::model_core(model), frame, data_options, infer_options, options);
}

[[nodiscard]] inline Series classification_metric_series(const Model& model,
                                                         const LabeledBatch& batch,
                                                         const AssessOptions& options = {}) {
  return detail::classification_metric_series(detail::model_core(model), batch, options);
}

[[nodiscard]] inline Series classification_metric_series(const Model& model,
                                                         const pluss::table::DataFrame& x,
                                                         const Series& y,
                                                         const AssessOptions& options = {}) {
  return detail::classification_metric_series(detail::model_core(model), x, y, options);
}

[[nodiscard]] inline Series classification_metric_series(const Model& model,
                                                         const pluss::table::DataFrame& x,
                                                         const pluss::table::Thread& y,
                                                         const AssessOptions& options = {}) {
  return detail::classification_metric_series(detail::model_core(model), x, y, options);
}

[[nodiscard]] inline ThresholdCurve threshold_curve(const Model& model,
                                                    const LabeledBatch& batch,
                                                    const InferOptions& infer_options = {}) {
  return detail::threshold_curve(detail::model_core(model), batch, infer_options);
}

[[nodiscard]] inline ThresholdCurve threshold_curve(const Model& model,
                                                    const pluss::table::DataFrame& x,
                                                    const Series& y,
                                                    const InferOptions& infer_options = {}) {
  return detail::threshold_curve(detail::model_core(model), x, y, infer_options);
}

[[nodiscard]] inline ThresholdCurve threshold_curve(const Model& model,
                                                    const pluss::table::DataFrame& x,
                                                    const pluss::table::Thread& y,
                                                    const InferOptions& infer_options = {}) {
  return detail::threshold_curve(detail::model_core(model), x, y, infer_options);
}

[[nodiscard]] inline ThresholdCurve threshold_curve(const Model& model,
                                                    const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& data_options,
                                                    const InferOptions& infer_options = {}) {
  return detail::threshold_curve(detail::model_core(model), frame, data_options, infer_options);
}

[[nodiscard]] inline double best_threshold(const Model& model,
                                           const LabeledBatch& batch,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  return detail::best_threshold(detail::model_core(model), batch, objective, infer_options);
}

[[nodiscard]] inline double best_threshold(const Model& model,
                                           const pluss::table::DataFrame& x,
                                           const Series& y,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  return detail::best_threshold(detail::model_core(model), x, y, objective, infer_options);
}

[[nodiscard]] inline double best_threshold(const Model& model,
                                           const pluss::table::DataFrame& x,
                                           const pluss::table::Thread& y,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  return detail::best_threshold(detail::model_core(model), x, y, objective, infer_options);
}

[[nodiscard]] inline double best_threshold(const Model& model,
                                           const pluss::table::DataFrame& frame,
                                           const DataFrameBatchOptions& data_options,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  return detail::best_threshold(detail::model_core(model), frame, data_options, objective, infer_options);
}

} // namespace pluss::ai::lymph::reports
