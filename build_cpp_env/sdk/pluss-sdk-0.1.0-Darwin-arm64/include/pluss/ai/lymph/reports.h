#pragma once

#include "pluss/ai/lymph/model.h"
#include "pluss/ai/lymph/selection.h"
#include "pluss/ai/lymph/workflow.h"

#include <utility>

// reports.h -- reporting family for model-backed classification summaries.
//
// The underlying implementations still live beside ModelCore because they
// need access to fitted category maps and staged inference internals. This
// header is the public family boundary: callers consume a public Model and
// receive report values, while Model itself does not grow reporting methods.

namespace pluss::ai::lymph::reports {

using Class = ClassReport;
using Compare = selection::CompareReport;
using CrossVal = metrics::CrossValidationReport;
using Metric = metrics::MetricReport;
using SeriesCompare = metrics::SeriesCompareReport;
using Tune = TuneReport;

[[nodiscard]] inline ConfusionTable confusion_table(const models::Model& model,
                                                    const LabeledBatch& batch,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  return detail::confusion_table(detail::model_core(model), batch, infer_options, options);
}

[[nodiscard]] inline ConfusionTable confusion_table(const models::Model& model,
                                                    const pluss::table::DataFrame& x,
                                                    const Series& y,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  return detail::confusion_table(detail::model_core(model), x, y, infer_options, options);
}

[[nodiscard]] inline ConfusionTable confusion_table(const models::Model& model,
                                                    const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& data_options,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  return detail::confusion_table(detail::model_core(model), frame, data_options, infer_options, options);
}

[[nodiscard]] inline ClassReport classification_summary(const models::Model& model,
                                                        const LabeledBatch& batch,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  return detail::classification_summary(detail::model_core(model), batch, infer_options, options);
}

[[nodiscard]] inline ClassReport classification_summary(const models::Model& model,
                                                        const pluss::table::DataFrame& x,
                                                        const Series& y,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  return detail::classification_summary(detail::model_core(model), x, y, infer_options, options);
}

[[nodiscard]] inline ClassReport classification_summary(const models::Model& model,
                                                        const pluss::table::DataFrame& frame,
                                                        const DataFrameBatchOptions& data_options,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  return detail::classification_summary(detail::model_core(model), frame, data_options, infer_options, options);
}

[[nodiscard]] inline Series classification_metric_series(const models::Model& model,
                                                         const LabeledBatch& batch,
                                                         const AssessOptions& options = {}) {
  return detail::classification_metric_series(detail::model_core(model), batch, options);
}

[[nodiscard]] inline Series classification_metric_series(const models::Model& model,
                                                         const pluss::table::DataFrame& x,
                                                         const Series& y,
                                                         const AssessOptions& options = {}) {
  return detail::classification_metric_series(detail::model_core(model), x, y, options);
}

[[nodiscard]] inline Series classification_metric_series(const models::Model& model,
                                                         const pluss::table::DataFrame& x,
                                                         const pluss::table::Thread& y,
                                                         const AssessOptions& options = {}) {
  return detail::classification_metric_series(detail::model_core(model), x, y, options);
}

[[nodiscard]] inline Series classification_metric_series(const models::Model& model,
                                                         const pluss::table::DataFrame& frame,
                                                         const DataFrameBatchOptions& data_options,
                                                         const AssessOptions& options = {}) {
  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options),
                                     "reports::classification_metric_series(frame,data_options)");
  return reports::classification_metric_series(model, batch, options);
}

[[nodiscard]] inline ConfusionTable confusion_table(const workflow::Workflow& workflow,
                                                    const workflow::Payload& payload,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  const detail::WorkflowLabeledInput input =
      detail::workflow_labeled_input_(workflow, payload, "reports::confusion_table(workflow,payload)");
  if (input.frame.has_value()) {
    return reports::confusion_table(workflow.model(), *input.frame, input.data_options, infer_options, options);
  }
  return reports::confusion_table(workflow.model(), *input.batch, infer_options, options);
}

[[nodiscard]] inline ClassReport classification_summary(const workflow::Workflow& workflow,
                                                        const workflow::Payload& payload,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  const detail::WorkflowLabeledInput input =
      detail::workflow_labeled_input_(workflow, payload, "reports::classification_summary(workflow,payload)");
  if (input.frame.has_value()) {
    return reports::classification_summary(workflow.model(), *input.frame, input.data_options, infer_options, options);
  }
  return reports::classification_summary(workflow.model(), *input.batch, infer_options, options);
}

[[nodiscard]] inline Series classification_metric_series(const workflow::Workflow& workflow,
                                                         const workflow::Payload& payload,
                                                         const AssessOptions& options = {}) {
  const detail::WorkflowLabeledInput input =
      detail::workflow_labeled_input_(workflow, payload, "reports::classification_metric_series(workflow,payload)");
  if (input.frame.has_value()) {
    return reports::classification_metric_series(workflow.model(), *input.frame, input.data_options, options);
  }
  return reports::classification_metric_series(workflow.model(), *input.batch, options);
}

[[nodiscard]] inline ThresholdCurve threshold_curve(const models::Model& model,
                                                    const LabeledBatch& batch,
                                                    const InferOptions& infer_options = {}) {
  return detail::threshold_curve(detail::model_core(model), batch, infer_options);
}

[[nodiscard]] inline ThresholdCurve threshold_curve(const models::Model& model,
                                                    const pluss::table::DataFrame& x,
                                                    const Series& y,
                                                    const InferOptions& infer_options = {}) {
  return detail::threshold_curve(detail::model_core(model), x, y, infer_options);
}

[[nodiscard]] inline ThresholdCurve threshold_curve(const models::Model& model,
                                                    const pluss::table::DataFrame& x,
                                                    const pluss::table::Thread& y,
                                                    const InferOptions& infer_options = {}) {
  return detail::threshold_curve(detail::model_core(model), x, y, infer_options);
}

[[nodiscard]] inline ThresholdCurve threshold_curve(const models::Model& model,
                                                    const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& data_options,
                                                    const InferOptions& infer_options = {}) {
  return detail::threshold_curve(detail::model_core(model), frame, data_options, infer_options);
}

[[nodiscard]] inline ThresholdCurve threshold_curve(const workflow::Workflow& workflow,
                                                    const workflow::Payload& payload,
                                                    const InferOptions& infer_options = {}) {
  const detail::WorkflowLabeledInput input =
      detail::workflow_labeled_input_(workflow, payload, "reports::threshold_curve(workflow,payload)");
  if (input.frame.has_value()) {
    return reports::threshold_curve(workflow.model(), *input.frame, input.data_options, infer_options);
  }
  return reports::threshold_curve(workflow.model(), *input.batch, infer_options);
}

[[nodiscard]] inline double best_threshold(const models::Model& model,
                                           const LabeledBatch& batch,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  return detail::best_threshold(detail::model_core(model), batch, objective, infer_options);
}

[[nodiscard]] inline double best_threshold(const models::Model& model,
                                           const pluss::table::DataFrame& x,
                                           const Series& y,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  return detail::best_threshold(detail::model_core(model), x, y, objective, infer_options);
}

[[nodiscard]] inline double best_threshold(const models::Model& model,
                                           const pluss::table::DataFrame& x,
                                           const pluss::table::Thread& y,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  return detail::best_threshold(detail::model_core(model), x, y, objective, infer_options);
}

[[nodiscard]] inline double best_threshold(const models::Model& model,
                                           const pluss::table::DataFrame& frame,
                                           const DataFrameBatchOptions& data_options,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  return detail::best_threshold(detail::model_core(model), frame, data_options, objective, infer_options);
}

[[nodiscard]] inline double best_threshold(const workflow::Workflow& workflow,
                                           const workflow::Payload& payload,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  const detail::WorkflowLabeledInput input =
      detail::workflow_labeled_input_(workflow, payload, "reports::best_threshold(workflow,payload)");
  if (input.frame.has_value()) {
    return reports::best_threshold(workflow.model(), *input.frame, input.data_options, objective, infer_options);
  }
  return reports::best_threshold(workflow.model(), *input.batch, objective, infer_options);
}

class ModelReports final {
 public:
  explicit ModelReports(models::Model model) : model_(std::move(model)) {}

  [[nodiscard]] const models::Model& model() const noexcept { return model_; }

  [[nodiscard]] ConfusionTable confusion_table(const LabeledBatch& batch,
                                               const InferOptions& infer_options = {},
                                               const ConfusionOptions& options = {}) const {
    return reports::confusion_table(model_, batch, infer_options, options);
  }

  [[nodiscard]] ConfusionTable confusion_table(const pluss::table::DataFrame& x,
                                               const Series& y,
                                               const InferOptions& infer_options = {},
                                               const ConfusionOptions& options = {}) const {
    return reports::confusion_table(model_, x, y, infer_options, options);
  }

  [[nodiscard]] ConfusionTable confusion_table(const pluss::table::DataFrame& frame,
                                               const DataFrameBatchOptions& data_options,
                                               const InferOptions& infer_options = {},
                                               const ConfusionOptions& options = {}) const {
    return reports::confusion_table(model_, frame, data_options, infer_options, options);
  }

  [[nodiscard]] ClassReport classification_summary(const LabeledBatch& batch,
                                                   const InferOptions& infer_options = {},
                                                   const ClassReportOptions& options = {}) const {
    return reports::classification_summary(model_, batch, infer_options, options);
  }

  [[nodiscard]] ClassReport classification_summary(const pluss::table::DataFrame& x,
                                                   const Series& y,
                                                   const InferOptions& infer_options = {},
                                                   const ClassReportOptions& options = {}) const {
    return reports::classification_summary(model_, x, y, infer_options, options);
  }

  [[nodiscard]] ClassReport classification_summary(const pluss::table::DataFrame& frame,
                                                   const DataFrameBatchOptions& data_options,
                                                   const InferOptions& infer_options = {},
                                                   const ClassReportOptions& options = {}) const {
    return reports::classification_summary(model_, frame, data_options, infer_options, options);
  }

  [[nodiscard]] Series metric_series(const LabeledBatch& batch,
                                     const AssessOptions& options = {}) const {
    return reports::classification_metric_series(model_, batch, options);
  }

  [[nodiscard]] Series metric_series(const pluss::table::DataFrame& x,
                                     const Series& y,
                                     const AssessOptions& options = {}) const {
    return reports::classification_metric_series(model_, x, y, options);
  }

  [[nodiscard]] Series metric_series(const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     const AssessOptions& options = {}) const {
    return reports::classification_metric_series(model_, x, y, options);
  }

  [[nodiscard]] ThresholdCurve threshold_curve(const LabeledBatch& batch,
                                               const InferOptions& infer_options = {}) const {
    return reports::threshold_curve(model_, batch, infer_options);
  }

  [[nodiscard]] ThresholdCurve threshold_curve(const pluss::table::DataFrame& x,
                                               const Series& y,
                                               const InferOptions& infer_options = {}) const {
    return reports::threshold_curve(model_, x, y, infer_options);
  }

  [[nodiscard]] ThresholdCurve threshold_curve(const pluss::table::DataFrame& frame,
                                               const DataFrameBatchOptions& data_options,
                                               const InferOptions& infer_options = {}) const {
    return reports::threshold_curve(model_, frame, data_options, infer_options);
  }

  [[nodiscard]] double best_threshold(const LabeledBatch& batch,
                                      MetricKind objective = MetricKind::F1,
                                      const InferOptions& infer_options = {}) const {
    return reports::best_threshold(model_, batch, objective, infer_options);
  }

  [[nodiscard]] double best_threshold(const pluss::table::DataFrame& x,
                                      const Series& y,
                                      MetricKind objective = MetricKind::F1,
                                      const InferOptions& infer_options = {}) const {
    return reports::best_threshold(model_, x, y, objective, infer_options);
  }

  [[nodiscard]] double best_threshold(const pluss::table::DataFrame& frame,
                                      const DataFrameBatchOptions& data_options,
                                      MetricKind objective = MetricKind::F1,
                                      const InferOptions& infer_options = {}) const {
    return reports::best_threshold(model_, frame, data_options, objective, infer_options);
  }

 private:
  models::Model model_;
};

using Model = ModelReports;

} // namespace pluss::ai::lymph::reports

namespace pluss::ai::lymph {

using ModelReports = reports::ModelReports;

} // namespace pluss::ai::lymph

namespace pluss::ai::lymph {

[[nodiscard]] inline reports::ModelReports models::Model::reports() const {
  return reports::ModelReports(*this);
}

} // namespace pluss::ai::lymph
