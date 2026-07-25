#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/prep.h"
#include "pluss/ai/lymph/workflow_model.h"

namespace pluss::ai::lymph {

class Workflow final {
public:
  std::optional<WorkflowModel> model{};
  std::vector<TransformSpec> stages{};

  [[nodiscard]] bool has_model() const noexcept { return model.has_value(); }
  [[nodiscard]] bool trained() const { ensure_model_("trained"); return model->trained(); }
  [[nodiscard]] std::int64_t stage_count() const { ensure_model_("stage_count"); return model->stage_count(); }
  [[nodiscard]] TaskKind task_kind() const { ensure_model_("task_kind"); return model->task_kind(); }
  [[nodiscard]] bool has_model_spec() const { ensure_model_("has_model_spec"); return model->has_model_spec(); }
  [[nodiscard]] bool has_opaque_transforms() const { ensure_model_("has_opaque_transforms"); return model->has_opaque_transforms(); }
  [[nodiscard]] const std::optional<ModelSpec>& model_spec() const {
    ensure_model_("model_spec");
    return model->model_spec();
  }
  [[nodiscard]] const std::vector<TransformSpec>& transform_specs() const {
    ensure_model_("transform_specs");
    return model->transform_specs();
  }
  [[nodiscard]] FlowGraphInfo graph_info() const {
    ensure_model_("graph_info");
    return model->graph_info();
  }
  [[nodiscard]] std::vector<std::string> nested_param_keys() const {
    ensure_model_("nested_param_keys");
    return model->nested_param_keys();
  }
  [[nodiscard]] double nested_param(std::string_view path) const {
    ensure_model_("nested_param");
    return model->nested_param(path);
  }
  Workflow& set_nested_param_(std::string_view path, double value) {
    ensure_model_("set_nested_param_");
    model->set_nested_param_(path, value);
    return *this;
  }
  [[nodiscard]] std::string flow_summary() const {
    ensure_model_("flow_summary");
    return model->flow_summary();
  }
  [[nodiscard]] std::string save_blueprint() const {
    ensure_model_("save_blueprint");
    return model->save_blueprint();
  }
  Workflow& load_blueprint_(std::string_view text) {
    ensure_model_("load_blueprint_");
    model->load_blueprint_(text);
    stages = model->transform_specs();
    return *this;
  }
  Workflow& set_stage_key_(std::int64_t stage_index, std::string stage_key) {
    ensure_model_("set_stage_key_");
    model->set_stage_key_(stage_index, std::move(stage_key));
    stages = model->transform_specs();
    return *this;
  }
  Workflow& set_stage_route_(std::int64_t stage_index, pluss::msharp::RangeView::Int feature_route) {
    ensure_model_("set_stage_route_");
    model->set_stage_route_(stage_index, feature_route);
    stages = model->transform_specs();
    return *this;
  }
  Workflow& set_stage_remainder_(std::int64_t stage_index, RouteRemainderPolicy remainder) {
    ensure_model_("set_stage_remainder_");
    model->set_stage_remainder_(stage_index, remainder);
    stages = model->transform_specs();
    return *this;
  }

  [[nodiscard]] const WorkflowModel& workflow_model() const {
    ensure_model_("workflow_model");
    return *model;
  }

  [[nodiscard]] WorkflowModel& workflow_model() {
    ensure_model_("workflow_model");
    return *model;
  }

  [[nodiscard]] Series coeff() const {
    ensure_model_("coeff");
    return model->coeff;
  }

  [[nodiscard]] Series intercept() const {
    ensure_model_("intercept");
    return model->intercept;
  }

  Workflow& learn_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    ensure_model_("learn_");
    model->learn_(batch, options);
    return *this;
  }

  Workflow& learn_(const pluss::table::DataFrame& frame,
                   const DataFrameBatchOptions& data_options,
                   const TrainOptions& options = {}) {
    ensure_model_("learn_");
    model->learn_(frame, data_options, options);
    return *this;
  }

  Workflow& learn_(const pluss::table::DataFrame& x,
                   const Series& y,
                   const TrainOptions& options = {}) {
    ensure_model_("learn_");
    model->learn_(x, y, options);
    return *this;
  }

  Workflow& learn_(const pluss::table::DataFrame& x,
                   const pluss::table::Thread& y,
                   const TrainOptions& options = {}) {
    ensure_model_("learn_");
    model->learn_(x, y, options);
    return *this;
  }

  [[nodiscard]] Workflow learn(const LabeledBatch& batch, const TrainOptions& options = {}) const {
    ensure_model_("learn");
    return from_model_(model->learn(batch, options));
  }

  [[nodiscard]] Workflow learn(const pluss::table::DataFrame& frame,
                               const DataFrameBatchOptions& data_options,
                               const TrainOptions& options = {}) const {
    ensure_model_("learn");
    return from_model_(model->learn(frame, data_options, options));
  }

  [[nodiscard]] Workflow learn(const pluss::table::DataFrame& x,
                              const Series& y,
                              const TrainOptions& options = {}) const {
    ensure_model_("learn");
    return from_model_(model->learn(x, y, options));
  }

  [[nodiscard]] Workflow learn(const pluss::table::DataFrame& x,
                              const pluss::table::Thread& y,
                              const TrainOptions& options = {}) const {
    ensure_model_("learn");
    return from_model_(model->learn(x, y, options));
  }

  Workflow& function_transformer(prep::FunctionTransformer transformer) {
    ensure_model_("function_transformer");
    model->function_transformer(std::move(transformer));
    return *this;
  }

  Workflow& feature_transformer(prep::FeatureTransformer transformer) {
    ensure_model_("feature_transformer");
    model->feature_transformer(std::move(transformer));
    return *this;
  }

  [[nodiscard]] Series infer(const FeatureBatch& batch, const InferOptions& options = {}) const {
    ensure_model_("infer");
    return model->infer(batch, options);
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& x, const InferOptions& options = {}) const {
    ensure_model_("infer");
    return model->infer(x, options);
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& frame,
                             const DataFrameBatchOptions& data_options,
                             const InferOptions& options = {}) const {
    ensure_model_("infer");
    return model->infer(frame, data_options, options);
  }

  [[nodiscard]] Series infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const {
    ensure_model_("infer_scores");
    return model->infer_scores(batch, options);
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& x, const InferOptions& options = {}) const {
    ensure_model_("infer_scores");
    return model->infer_scores(x, options);
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const InferOptions& options = {}) const {
    ensure_model_("infer_scores");
    return model->infer_scores(frame, data_options, options);
  }

  [[nodiscard]] Series map(const FeatureBatch& batch, const InferOptions& options = {}) const {
    ensure_model_("map");
    return model->map(batch, options);
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& x, const InferOptions& options = {}) const {
    ensure_model_("map");
    return model->map(x, options);
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& frame,
                           const DataFrameBatchOptions& data_options,
                           const InferOptions& options = {}) const {
    ensure_model_("map");
    return model->map(frame, data_options, options);
  }

  [[nodiscard]] Series learn_map(const FeatureBatch& batch,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    ensure_model_("learn_map");
    return model->learn_map(batch, learn_options, infer_options);
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& x,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    ensure_model_("learn_map");
    return model->learn_map(x, learn_options, infer_options);
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& frame,
                                 const DataFrameBatchOptions& data_options,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    ensure_model_("learn_map");
    return model->learn_map(frame, data_options, learn_options, infer_options);
  }

  [[nodiscard]] Series assess_series(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    ensure_model_("assess_series");
    return model->assess_series(batch, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const Series& y,
                                     const AssessOptions& options = {}) const {
    ensure_model_("assess_series");
    return model->assess_series(x, y, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     const AssessOptions& options = {}) const {
    ensure_model_("assess_series");
    return model->assess_series(x, y, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& frame,
                                     const DataFrameBatchOptions& data_options,
                                     const AssessOptions& options = {}) const {
    ensure_model_("assess_series");
    return model->assess_series(frame, data_options, options);
  }

  [[nodiscard]] MetricReport assess(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    ensure_model_("assess");
    return model->assess(batch, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const AssessOptions& options = {}) const {
    ensure_model_("assess");
    return model->assess(x, y, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const AssessOptions& options = {}) const {
    ensure_model_("assess");
    return model->assess(x, y, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const AssessOptions& options = {}) const {
    ensure_model_("assess");
    return model->assess(frame, data_options, options);
  }

  [[nodiscard]] ConfusionTable confusion_table(const LabeledBatch& batch,
                                               const InferOptions& infer_options = {},
                                               const ConfusionOptions& options = {}) const {
    ensure_model_("confusion_table");
    return ::pluss::ai::lymph::confusion_table(*model, batch, infer_options, options);
  }

  [[nodiscard]] ConfusionTable confusion_table(const pluss::table::DataFrame& x,
                                               const Series& y,
                                               const InferOptions& infer_options = {},
                                               const ConfusionOptions& options = {}) const {
    ensure_model_("confusion_table");
    return ::pluss::ai::lymph::confusion_table(*model, x, y, infer_options, options);
  }

  [[nodiscard]] ConfusionTable confusion_table(const pluss::table::DataFrame& x,
                                               const pluss::table::Thread& y,
                                               const InferOptions& infer_options = {},
                                               const ConfusionOptions& options = {}) const {
    ensure_model_("confusion_table");
    return ::pluss::ai::lymph::confusion_table(*model, x, as_stratify_series(y), infer_options, options);
  }

  [[nodiscard]] ClassReport classification_summary(const LabeledBatch& batch,
                                                   const InferOptions& infer_options = {},
                                                   const ClassReportOptions& options = {}) const {
    ensure_model_("classification_summary");
    return ::pluss::ai::lymph::classification_summary(*model, batch, infer_options, options);
  }

  [[nodiscard]] ClassReport classification_summary(const pluss::table::DataFrame& x,
                                                   const Series& y,
                                                   const InferOptions& infer_options = {},
                                                   const ClassReportOptions& options = {}) const {
    ensure_model_("classification_summary");
    return ::pluss::ai::lymph::classification_summary(*model, x, y, infer_options, options);
  }

  [[nodiscard]] ClassReport classification_summary(const pluss::table::DataFrame& x,
                                                   const pluss::table::Thread& y,
                                                   const InferOptions& infer_options = {},
                                                   const ClassReportOptions& options = {}) const {
    ensure_model_("classification_summary");
    return ::pluss::ai::lymph::classification_summary(*model, x, as_stratify_series(y), infer_options, options);
  }

  [[nodiscard]] Series classification_metric_series(const LabeledBatch& batch,
                                                    const AssessOptions& options = {}) const {
    ensure_model_("classification_metric_series");
    return ::pluss::ai::lymph::classification_metric_series(*model, batch, options);
  }

  [[nodiscard]] Series classification_metric_series(const pluss::table::DataFrame& x,
                                                    const Series& y,
                                                    const AssessOptions& options = {}) const {
    ensure_model_("classification_metric_series");
    return ::pluss::ai::lymph::classification_metric_series(*model, x, y, options);
  }

  [[nodiscard]] Series classification_metric_series(const pluss::table::DataFrame& x,
                                                    const pluss::table::Thread& y,
                                                    const AssessOptions& options = {}) const {
    ensure_model_("classification_metric_series");
    return ::pluss::ai::lymph::classification_metric_series(*model, x, as_stratify_series(y), options);
  }

  [[nodiscard]] PermuteImpactReport permutation_impact(const LabeledBatch& train,
                                                        const LabeledBatch& eval,
                                                        MetricKind metric = MetricKind::Accuracy,
                                                        std::int64_t repeats = 5,
                                                        RandomState random = {}) const {
    ensure_model_("permutation_impact");
    return model->permutation_impact(train, eval, metric, repeats, random);
  }

  [[nodiscard]] DependenceCurve dependence_curve(const LabeledBatch& train,
                                                 const FeatureBatch& eval,
                                                 std::int64_t feature_index,
                                                 std::int64_t grid_points = 32) const {
    ensure_model_("dependence_curve");
    return model->dependence_curve(train, eval, feature_index, grid_points);
  }

  [[nodiscard]] DependenceSurface dependence_surface(const LabeledBatch& train,
                                                     const FeatureBatch& eval,
                                                     std::int64_t feature_a,
                                                     std::int64_t feature_b,
                                                     std::int64_t grid_points_a = 16,
                                                     std::int64_t grid_points_b = 16) const {
    ensure_model_("dependence_surface");
    return model->dependence_surface(train, eval, feature_a, feature_b, grid_points_a, grid_points_b);
  }

  [[nodiscard]] std::vector<std::vector<Series>> dump_transform_states() const {
    ensure_model_("dump_transform_states");
    return model->dump_transform_states();
  }

  [[nodiscard]] std::vector<Series> dump_model_state() const {
    ensure_model_("dump_model_state");
    return model->dump_model_state();
  }

  Workflow& load_transform_states_(pluss::msharp::RangeView::Of<std::vector<Series>> states) {
    ensure_model_("load_transform_states_");
    model->load_transform_states_(states);
    return *this;
  }

  Workflow& load_model_state_(pluss::msharp::RangeView::Series state) {
    ensure_model_("load_model_state_");
    model->load_model_state_(state);
    return *this;
  }

  Workflow& mark_trained_(bool trained = true) {
    ensure_model_("mark_trained_");
    model->mark_trained_(trained);
    return *this;
  }

  void save(const std::string& path,
            const LabeledBatch& train_batch,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    ensure_model_("save");
    model->save(path, train_batch, train_options, std::move(metadata));
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const Series& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    ensure_model_("save");
    model->save(path, x_train, y_train, train_options, std::move(metadata));
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const pluss::table::Thread& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    ensure_model_("save");
    model->save(path, x_train, y_train, train_options, std::move(metadata));
  }

  [[nodiscard]] static Workflow load(const std::string& path) {
    return from_model_(WorkflowModel::load(path));
  }

  [[nodiscard]] static Workflow restore(const io::FlowBundle& bundle) {
    return from_model_(WorkflowModel::restore(bundle));
  }

private:
  void ensure_model_(const char* where) const {
    if (!model.has_value()) {
      ::pluss::throw_error(std::string("Workflow::") + where + ": construct as Workflow{ .model = ..., .stages = {...} }.");
    }
    const_cast<Workflow*>(this)->replay_transforms_();
  }

  [[nodiscard]] static Workflow from_model_(WorkflowModel model_value) {
    Workflow out;
    out.model.emplace(std::move(model_value));
    if (out.model.has_value()) {
      out.stages = out.model->transform_specs();
    }
    return out;
  }

  void replay_transforms_() {
    if (!model.has_value() || stages.empty()) return;
    const std::int64_t bound_count = model->stage_count();
    std::size_t begin = 0;
    if (bound_count > 0) {
      begin = static_cast<std::size_t>(std::min<std::int64_t>(bound_count, static_cast<std::int64_t>(stages.size())));
    }
    for (std::size_t i = begin; i < stages.size(); ++i) {
      apply_transform_(*model, stages[i]);
    }
  }

  static void apply_transform_(WorkflowModel& model, const TransformSpec& spec) {
    switch (spec.kind) {
      case TransformKind::StandardScaler:
        model.standard_scaler(std::get<prep::StandardScalerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::MinMaxScaler:
        model.minmax_scaler(std::get<prep::MinMaxScalerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::RobustScaler:
        model.robust_scaler(std::get<prep::RobustScalerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::SimpleImputer:
        model.simple_imputer(std::get<prep::SimpleImputerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::NeighborImputer:
        model.neighbor_imputer(std::get<prep::NeighborImputerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::IterativeImputer:
        model.iterative_imputer(std::get<prep::IterativeImputerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::OneHotEncoder:
        model.one_hot_encoder(std::get<prep::OneHotEncoderOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::FrequencyEncoder:
        model.frequency_encoder(std::get<prep::FrequencyEncoderOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::OrdinalEncoder:
        model.ordinal_encoder(std::get<prep::OrdinalEncoderOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::LabelEncoder:
        model.label_encoder(std::get<prep::LabelEncoderOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::VectorNormalizer:
        model.vector_normalizer(std::get<prep::VectorNormalizerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::MaxAbsScaler:
        model.maxabs_scaler(std::get<prep::MaxAbsScalerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::QuantileMap:
        model.quantile_map(std::get<prep::QuantileMapOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::PowerMap:
        model.power_map(std::get<prep::PowerMapOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::ThresholdBinarizer:
        model.threshold_binarizer(std::get<prep::ThresholdBinarizerOptions>(spec.options), spec.feature_route);
        return;
      case TransformKind::PolynomialLift:
        model.polynomial_lift(std::get<prep::PolynomialLiftOptions>(spec.options), spec.feature_route);
        return;
    }
  }

};

using TrainedHandle = Workflow;
using AnyTrainedModel = Workflow;

[[nodiscard]] inline Workflow make_pipeline(WorkflowModel model,
                                            pluss::msharp::RangeView::Of<TransformSpec> stages = {}) {
  Workflow out{};
  out.model.emplace(std::move(model));
  out.stages = copy_range_values(stages);
  return out;
}

[[nodiscard]] inline Workflow make_pipeline(const ModelSpec& spec,
                                            pluss::msharp::RangeView::Of<TransformSpec> stages = {}) {
  return make_pipeline(WorkflowModel(spec), stages);
}

[[nodiscard]] inline Workflow make_pipeline(WorkflowModel model,
                                            std::initializer_list<TransformSpec> stages) {
  return make_pipeline(std::move(model), pluss::msharp::RangeView::Of<TransformSpec>(stages));
}

[[nodiscard]] inline Workflow make_pipeline(const ModelSpec& spec,
                                            std::initializer_list<TransformSpec> stages) {
  return make_pipeline(spec, pluss::msharp::RangeView::Of<TransformSpec>(stages));
}

[[nodiscard]] inline Workflow pipeline(WorkflowModel model,
                                       pluss::msharp::RangeView::Of<TransformSpec> stages = {}) {
  return make_pipeline(std::move(model), stages);
}

[[nodiscard]] inline Workflow pipeline(const ModelSpec& spec,
                                       pluss::msharp::RangeView::Of<TransformSpec> stages = {}) {
  return make_pipeline(spec, stages);
}

[[nodiscard]] inline Workflow pipeline(WorkflowModel model,
                                       std::initializer_list<TransformSpec> stages) {
  return make_pipeline(std::move(model), stages);
}

[[nodiscard]] inline Workflow pipeline(const ModelSpec& spec,
                                       std::initializer_list<TransformSpec> stages) {
  return make_pipeline(spec, stages);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const Workflow& workflow,
                                                          const LabeledBatch& batch,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(workflow.workflow_model(), batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const Workflow& workflow,
                                                          const pluss::table::DataFrame& x,
                                                          const Series& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(workflow.workflow_model(), x, y, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const Workflow& workflow,
                                                          const pluss::table::DataFrame& x,
                                                          const pluss::table::Thread& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(workflow.workflow_model(), x, y, cv, train_options, assess_options);
}

[[nodiscard]] inline Series cross_validate_scores(const Workflow& workflow,
                                                  const LabeledBatch& batch,
                                                  MetricKind metric,
                                                  CrossValidationOptions cv = {},
                                                  TrainOptions train_options = {},
                                                  AssessOptions assess_options = {}) {
  const auto spec_opt = workflow.workflow_model().workflow().model_spec();
  if (!spec_opt.has_value()) {
    ::pluss::throw_error("cross_validate_scores(workflow,...): workflow has no model specification.");
  }
  return cross_validate_scores(*spec_opt, batch, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline double cross_validate_score(const Workflow& workflow,
                                                 const LabeledBatch& batch,
                                                 MetricKind metric,
                                                 CrossValidationOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  const auto spec_opt = workflow.workflow_model().workflow().model_spec();
  if (!spec_opt.has_value()) {
    ::pluss::throw_error("cross_validate_score(workflow,...): workflow has no model specification.");
  }
  return cross_validate_score(*spec_opt, batch, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline Series cross_validate_scores(const Workflow& workflow,
                                                  const pluss::table::DataFrame& x,
                                                  const Series& y,
                                                  MetricKind metric,
                                                  CrossValidationOptions cv = {},
                                                  TrainOptions train_options = {},
                                                  AssessOptions assess_options = {}) {
  const auto spec_opt = workflow.workflow_model().workflow().model_spec();
  if (!spec_opt.has_value()) {
    ::pluss::throw_error("cross_validate_scores(workflow,x,y): workflow has no model specification.");
  }
  return cross_validate_scores(*spec_opt, x, y, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline double cross_validate_score(const Workflow& workflow,
                                                 const pluss::table::DataFrame& x,
                                                 const Series& y,
                                                 MetricKind metric,
                                                 CrossValidationOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  const auto spec_opt = workflow.workflow_model().workflow().model_spec();
  if (!spec_opt.has_value()) {
    ::pluss::throw_error("cross_validate_score(workflow,x,y): workflow has no model specification.");
  }
  return cross_validate_score(*spec_opt, x, y, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline Series cross_validate_scores(const Workflow& workflow,
                                                  const pluss::table::DataFrame& x,
                                                  const pluss::table::Thread& y,
                                                  MetricKind metric,
                                                  CrossValidationOptions cv = {},
                                                  TrainOptions train_options = {},
                                                  AssessOptions assess_options = {}) {
  return cross_validate_scores(workflow, x, as_stratify_series(y), metric, cv, train_options, assess_options);
}

[[nodiscard]] inline double cross_validate_score(const Workflow& workflow,
                                                 const pluss::table::DataFrame& x,
                                                 const pluss::table::Thread& y,
                                                 MetricKind metric,
                                                 CrossValidationOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  return cross_validate_score(workflow, x, as_stratify_series(y), metric, cv, train_options, assess_options);
}


} // namespace pluss::ai::lymph
