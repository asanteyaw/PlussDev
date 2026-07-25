#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/prep.h"
#include "pluss/ai/lymph/metrics.h"
#include "pluss/ai/lymph/inspection.h"

namespace pluss::ai::lymph {

class WorkflowModel final {
public:
  explicit WorkflowModel(const ModelSpec& spec)
      : workflow_(resolve(make_flow(spec), "make_flow")) {
    refresh_linear_summary_();
  }

  explicit WorkflowModel(Workflow workflow)
      : workflow_(std::move(workflow)) {
    refresh_linear_summary_();
  }

  // Exposed linear summary for reference-aligned convenience.
  Series coeff = Series::undefined();
  Series intercept = Series::undefined();

  [[nodiscard]] bool trained() const noexcept { return workflow_.trained(); }
  [[nodiscard]] bool has_model() const noexcept { return workflow_.has_model(); }
  [[nodiscard]] bool has_model_spec() const noexcept { return workflow_.has_model_spec(); }
  [[nodiscard]] bool has_opaque_transforms() const noexcept { return workflow_.has_opaque_transforms(); }
  [[nodiscard]] std::int64_t stage_count() const noexcept { return workflow_.stage_count(); }
  [[nodiscard]] TaskKind task_kind() const noexcept { return workflow_.task_kind(); }
  [[nodiscard]] const std::optional<ModelSpec>& model_spec() const noexcept { return workflow_.model_spec(); }
  [[nodiscard]] const std::vector<TransformSpec>& transform_specs() const noexcept { return workflow_.transform_specs(); }
  [[nodiscard]] FlowGraphInfo graph_info() const { return resolve(workflow_.graph_info(), "graph_info"); }
  [[nodiscard]] std::vector<std::string> nested_param_keys() const { return workflow_.nested_param_keys(); }
  [[nodiscard]] double nested_param(std::string_view path) const {
    return resolve(workflow_.nested_param(path), "nested_param");
  }
  WorkflowModel& set_nested_param_(std::string_view path, double value) {
    resolve(workflow_.set_nested_param(path, value), "set_nested_param_");
    return *this;
  }
  [[nodiscard]] std::string flow_summary() const { return workflow_.flow_summary(); }
  [[nodiscard]] std::string save_blueprint() const {
    return resolve(workflow_.save_blueprint(), "save_blueprint");
  }
  WorkflowModel& load_blueprint_(std::string_view text) {
    resolve(workflow_.load_blueprint(text), "load_blueprint_");
    refresh_linear_summary_();
    return *this;
  }
  WorkflowModel& set_stage_key_(std::int64_t stage_index, std::string stage_key) {
    resolve(workflow_.set_stage_key(stage_index, std::move(stage_key)), "set_stage_key_");
    return *this;
  }
  WorkflowModel& set_stage_route_(std::int64_t stage_index, pluss::msharp::RangeView::Int feature_route) {
    resolve(workflow_.set_stage_route(stage_index, feature_route), "set_stage_route_");
    return *this;
  }
  WorkflowModel& set_stage_remainder_(std::int64_t stage_index, RouteRemainderPolicy remainder) {
    resolve(workflow_.set_stage_remainder(stage_index, remainder), "set_stage_remainder_");
    return *this;
  }

  WorkflowModel& standard_scaler(prep::StandardScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_standard_scaler(options, feature_route), "standard_scaler");
    return *this;
  }

  WorkflowModel& minmax_scaler(prep::MinMaxScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_minmax_scaler(options, feature_route), "minmax_scaler");
    return *this;
  }

  WorkflowModel& robust_scaler(prep::RobustScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_robust_scaler(options, feature_route), "robust_scaler");
    return *this;
  }

  WorkflowModel& simple_imputer(prep::SimpleImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_simple_imputer(options, feature_route), "simple_imputer");
    return *this;
  }

  WorkflowModel& neighbor_imputer(prep::NeighborImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_neighbor_imputer(options, feature_route), "neighbor_imputer");
    return *this;
  }

  WorkflowModel& iterative_imputer(prep::IterativeImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_iterative_imputer(options, feature_route), "iterative_imputer");
    return *this;
  }

  WorkflowModel& one_hot_encoder(prep::OneHotEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_one_hot_encoder(options, feature_route), "one_hot_encoder");
    return *this;
  }

  WorkflowModel& frequency_encoder(prep::FrequencyEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_frequency_encoder(options, feature_route), "frequency_encoder");
    return *this;
  }

  WorkflowModel& ordinal_encoder(prep::OrdinalEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_ordinal_encoder(options, feature_route), "ordinal_encoder");
    return *this;
  }

  WorkflowModel& label_encoder(prep::LabelEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_label_encoder(options, feature_route), "label_encoder");
    return *this;
  }

  WorkflowModel& vector_normalizer(prep::VectorNormalizerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_vector_normalizer(options, feature_route), "vector_normalizer");
    return *this;
  }

  WorkflowModel& maxabs_scaler(prep::MaxAbsScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_maxabs_scaler(options, feature_route), "maxabs_scaler");
    return *this;
  }

  WorkflowModel& quantile_map(prep::QuantileMapOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_quantile_map(options, feature_route), "quantile_map");
    return *this;
  }

  WorkflowModel& power_map(prep::PowerMapOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_power_map(options, feature_route), "power_map");
    return *this;
  }

  WorkflowModel& threshold_binarizer(prep::ThresholdBinarizerOptions options = {},
                                     pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_threshold_binarizer(options, feature_route), "threshold_binarizer");
    return *this;
  }

  WorkflowModel& polynomial_lift(prep::PolynomialLiftOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_polynomial_lift(options, feature_route), "polynomial_lift");
    return *this;
  }

  WorkflowModel& function_transformer(prep::FunctionTransformer transformer) {
    resolve(workflow_.add_function_transformer(std::move(transformer)), "function_transformer");
    return *this;
  }

  WorkflowModel& feature_transformer(prep::FeatureTransformer transformer) {
    resolve(workflow_.add_feature_transformer(std::move(transformer)), "feature_transformer");
    return *this;
  }

  WorkflowModel& learn_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    resolve(workflow_.learn(batch, options), "learn");
    refresh_linear_summary_();
    return *this;
  }

  WorkflowModel& learn_(const pluss::table::DataFrame& frame,
                        const DataFrameBatchOptions& data_options,
                        const TrainOptions& options = {}) {
    resolve(workflow_.learn(frame, data_options, options), "learn");
    feature_names_ = resolve_feature_names_from_options_(frame, data_options, "learn_");
    refresh_linear_summary_();
    return *this;
  }

  WorkflowModel& learn_(const pluss::table::DataFrame& x,
                        const Series& y,
                        const TrainOptions& options = {}) {
    const auto names = default_series_feature_names_(x, "learn_(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "learn_(x,y)");
    feature_names_ = names;
    return learn_(batch, options);
  }

  WorkflowModel& learn_(const pluss::table::DataFrame& x,
                        const pluss::table::Thread& y,
                        const TrainOptions& options = {}) {
    return learn_(x, as_stratify_series(y), options);
  }

  [[nodiscard]] WorkflowModel learn(const LabeledBatch& batch, const TrainOptions& options = {}) const {
    WorkflowModel out = clone_for_out_of_place_();
    out.learn_(batch, options);
    return out;
  }

  [[nodiscard]] WorkflowModel learn(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const TrainOptions& options = {}) const {
    WorkflowModel out = clone_for_out_of_place_();
    out.learn_(frame, data_options, options);
    return out;
  }

  [[nodiscard]] WorkflowModel learn(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const TrainOptions& options = {}) const {
    WorkflowModel out = clone_for_out_of_place_();
    out.learn_(x, y, options);
    return out;
  }

  [[nodiscard]] WorkflowModel learn(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const TrainOptions& options = {}) const {
    WorkflowModel out = clone_for_out_of_place_();
    out.learn_(x, y, options);
    return out;
  }

  [[nodiscard]] Series map(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(workflow_.map(batch, options), "map");
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& frame,
                           const DataFrameBatchOptions& data_options,
                           const InferOptions& options = {}) const {
    const auto batch = resolve(feature_batch_from_dataframe(frame, data_options), "map.feature_batch_from_dataframe");
    return map(batch, options);
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& x,
                           const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "map(x)");
    return map(x, default_data_options_(names), options);
  }

  [[nodiscard]] Series learn_map(const FeatureBatch& batch,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    Series mapped = resolve(workflow_.learn_map(batch, learn_options, infer_options), "learn_map");
    refresh_linear_summary_();
    return mapped;
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& frame,
                                 const DataFrameBatchOptions& data_options,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    const auto batch = resolve(feature_batch_from_dataframe(frame, data_options), "learn_map.feature_batch_from_dataframe");
    const Series mapped = learn_map(batch, learn_options, infer_options);
    feature_names_ = resolve_feature_names_from_options_(frame, data_options, "learn_map");
    return mapped;
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& x,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "learn_map(x)");
    return learn_map(x, default_data_options_(names), learn_options, infer_options);
  }

  [[nodiscard]] Series infer(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(workflow_.infer(batch, options), "infer");
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& frame,
                             const DataFrameBatchOptions& data_options,
                             const InferOptions& options = {}) const {
    return resolve(workflow_.infer(frame, data_options, options), "infer");
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& x,
                             const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "infer(x)");
    return resolve(workflow_.infer(x, default_data_options_(names), options), "infer");
  }

  [[nodiscard]] Series infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(workflow_.infer_scores(batch, options), "infer_scores");
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const InferOptions& options = {}) const {
    return resolve(workflow_.infer_scores(frame, data_options, options), "infer_scores");
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& x,
                                    const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "infer_scores(x)");
    return resolve(workflow_.infer_scores(x, default_data_options_(names), options), "infer_scores");
  }

  [[nodiscard]] Series assess_series(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return resolve(workflow_.assess(batch, options), "assess");
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& frame,
                                     const DataFrameBatchOptions& data_options,
                                     const AssessOptions& options = {}) const {
    return resolve(workflow_.assess(frame, data_options, options), "assess");
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const Series& y,
                                     const AssessOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "assess(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "assess(x,y)");
    return assess_series(batch, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     const AssessOptions& options = {}) const {
    return assess_series(x, as_stratify_series(y), options);
  }

  [[nodiscard]] MetricReport assess(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return as_metric_report_(assess_series(batch, options), resolved_metrics_(options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const AssessOptions& options = {}) const {
    return as_metric_report_(assess_series(frame, data_options, options), resolved_metrics_(options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const AssessOptions& options = {}) const {
    return as_metric_report_(assess_series(x, y, options), resolved_metrics_(options));
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const AssessOptions& options = {}) const {
    return as_metric_report_(assess_series(x, y, options), resolved_metrics_(options));
  }

  [[nodiscard]] PermuteImpactReport permutation_impact(const LabeledBatch& train,
                                                        const LabeledBatch& eval,
                                                        MetricKind metric = MetricKind::Accuracy,
                                                        std::int64_t repeats = 5,
                                                        RandomState random = {}) const {
    if (!workflow_.model_spec().has_value()) {
      ::pluss::throw_error("WorkflowModel::permutation_impact requires a model specification.");
    }
    return ::pluss::ai::lymph::inspection::permutation_impact(*workflow_.model_spec(),
                                                               train,
                                                               eval,
                                                               metric,
                                                               repeats,
                                                               random);
  }

  [[nodiscard]] DependenceCurve dependence_curve(const LabeledBatch& train,
                                                 const FeatureBatch& eval,
                                                 std::int64_t feature_index,
                                                 std::int64_t grid_points = 32) const {
    if (!workflow_.model_spec().has_value()) {
      ::pluss::throw_error("WorkflowModel::dependence_curve requires a model specification.");
    }
    return ::pluss::ai::lymph::inspection::dependence_curve(*workflow_.model_spec(),
                                                             train,
                                                             eval,
                                                             feature_index,
                                                             grid_points);
  }

  [[nodiscard]] DependenceSurface dependence_surface(const LabeledBatch& train,
                                                     const FeatureBatch& eval,
                                                     std::int64_t feature_a,
                                                     std::int64_t feature_b,
                                                     std::int64_t grid_points_a = 16,
                                                     std::int64_t grid_points_b = 16) const {
    if (!workflow_.model_spec().has_value()) {
      ::pluss::throw_error("WorkflowModel::dependence_surface requires a model specification.");
    }
    return ::pluss::ai::lymph::inspection::dependence_surface(*workflow_.model_spec(),
                                                               train,
                                                               eval,
                                                               feature_a,
                                                               feature_b,
                                                               grid_points_a,
                                                               grid_points_b);
  }

  [[nodiscard]] LinearParameters linear_parameters() const {
    const auto& spec_opt = workflow_.model_spec();
    if (!spec_opt.has_value()) {
      ::pluss::throw_error("linear_parameters: workflow has no model specification.");
    }
    if (spec_opt->family != ModelFamily::Linear) {
      ::pluss::throw_error("linear_parameters: only available for linear models.");
    }

    const auto* linear_options = std::get_if<LinearFamilyOptions>(&spec_opt->options);
    if (linear_options == nullptr) {
      ::pluss::throw_error("linear_parameters: model options do not contain LinearFamilyOptions.");
    }
    const bool fit_intercept = linear_options->fit_intercept;

    const std::vector<Series> state = resolve(workflow_.dump_model_state(), "dump_model_state");
    if (state.size() < 2 || !state[1].defined()) {
      ::pluss::throw_error("linear_parameters: missing coefficient state.");
    }

    const Series beta = state[1].reshape({state[1].numel()});
    if (beta.numel() == 0) {
      ::pluss::throw_error("linear_parameters: coefficient vector is empty.");
    }

    // Slicing directly off `beta` (whatever device/dtype it already has)
    // instead of round-tripping through a host std::vector to drop the
    // intercept element and rebuilding two fresh CPU Series from it.
    LinearParameters out{};
    out.fit_intercept = fit_intercept;
    if (fit_intercept) {
      out.intercept = beta.narrow_copy(0, 0, 1);
      out.coeff = beta.narrow_copy(0, 1, beta.numel() - 1);
    } else {
      out.intercept = scalar_series_(0.0);
      out.coeff = beta;
    }
    return out;
  }

  [[nodiscard]] std::vector<double> coefficients() const {
    if (!coeff.defined()) return {};
    return coeff.to(pluss::pCPU, ScalarType::Float64).reshape({coeff.numel()}).to_vec<double>();
  }

  [[nodiscard]] double intercept_term() const {
    if (!intercept.defined() || intercept.numel() == 0) return 0.0;
    return intercept.to(pluss::pCPU, ScalarType::Float64).reshape({intercept.numel()}).item<double>();
  }

  void save(const std::string& path,
            const LabeledBatch& train_batch,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    resolve(io::save(path, workflow_, train_batch, train_options, std::move(metadata)), "save");
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const Series& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x_train, "save(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x_train, y_train, names, "save(x,y)");
    resolve(io::save(path, workflow_, batch, train_options, std::move(metadata)), "save");
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const pluss::table::Thread& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    save(path, x_train, as_stratify_series(y_train), train_options, std::move(metadata));
  }

  [[nodiscard]] static WorkflowModel restore(const io::FlowBundle& bundle) {
    return WorkflowModel(resolve(io::restore(bundle), "restore"));
  }

  [[nodiscard]] static WorkflowModel load(const std::string& path) {
    return WorkflowModel(resolve(io::load_and_restore(path), "load_and_restore"));
  }

  [[nodiscard]] std::vector<std::vector<Series>> dump_transform_states() const {
    return resolve(workflow_.dump_transform_states(), "dump_transform_states");
  }

  [[nodiscard]] std::vector<Series> dump_model_state() const {
    return resolve(workflow_.dump_model_state(), "dump_model_state");
  }

  WorkflowModel& load_transform_states_(pluss::msharp::RangeView::Of<std::vector<Series>> states) {
    resolve(workflow_.load_transform_states(states), "load_transform_states");
    refresh_linear_summary_();
    return *this;
  }

  WorkflowModel& load_model_state_(pluss::msharp::RangeView::Series state) {
    resolve(workflow_.load_model_state(state), "load_model_state");
    refresh_linear_summary_();
    return *this;
  }

  WorkflowModel& mark_trained_(bool trained = true) {
    workflow_.mark_trained(trained);
    if (!trained) {
      coeff = Series::undefined();
      intercept = Series::undefined();
    } else {
      refresh_linear_summary_();
    }
    return *this;
  }

  [[nodiscard]] const Workflow& workflow() const noexcept { return workflow_; }
  [[nodiscard]] Workflow& workflow() noexcept { return workflow_; }

private:
  [[nodiscard]] static DataFrameBatchOptions default_data_options_(pluss::msharp::RangeView::Of<std::string> feature_names) {
    DataFrameBatchOptions options{};
    options.feature_names = copy_range_values(feature_names);
    options.layout = ObservationLayout::Rows;
    options.thread_policy = DataThreadPolicy::CastTemporalToSeries;
    options.note_policy = DataNotePolicy::Reject;
    return options;
  }

  [[nodiscard]] static std::vector<std::string> default_series_feature_names_(const pluss::table::DataFrame& frame,
                                                                               const char* where) {
    const auto names = frame.feataxis().series().features();
    if (names.empty()) {
      ::pluss::throw_error(std::string(where) + ": input DataFrame has no Series features.");
    }
    return names;
  }

  [[nodiscard]] static std::vector<std::string> resolve_feature_names_from_options_(
      const pluss::table::DataFrame& frame,
      const DataFrameBatchOptions& options,
      const char* where) {
    std::vector<std::string> names = options.feature_names.empty() ? frame.features() : options.feature_names;
    if (options.label_feature.has_value()) {
      names.erase(std::remove(names.begin(), names.end(), *options.label_feature), names.end());
    }
    if (options.sample_weight_feature.has_value()) {
      names.erase(std::remove(names.begin(), names.end(), *options.sample_weight_feature), names.end());
    }
    if (names.empty()) {
      ::pluss::throw_error(std::string(where) + ": resolved zero feature names.");
    }
    return names;
  }

  [[nodiscard]] static LabeledBatch labeled_from_xy_(const pluss::table::DataFrame& x,
                                                     const Series& y,
                                                     pluss::msharp::RangeView::Of<std::string> feature_names,
                                                     const char* where) {
    const auto x_batch =
        resolve(feature_batch_from_dataframe(x, default_data_options_(feature_names)), std::string(where) + ".feature_batch_from_dataframe");
    const Series y_aligned = y.to(x_batch.x.platform(), ScalarType::Float64).reshape({x_batch.sample_count()});
    LabeledBatch batch{
        .features = x_batch,
        .y = y_aligned,
        .sample_weights = Series::undefined(),
    };
    resolve(validate_labeled(batch), std::string(where) + ".validate_labeled");
    return batch;
  }

  [[nodiscard]] static Status append_transform_from_spec_(Workflow& flow, const TransformSpec& spec) {
    switch (spec.kind) {
      case TransformKind::StandardScaler:
        return flow.add_standard_scaler(std::get<prep::StandardScalerOptions>(spec.options), spec.feature_route);
      case TransformKind::MinMaxScaler:
        return flow.add_minmax_scaler(std::get<prep::MinMaxScalerOptions>(spec.options), spec.feature_route);
      case TransformKind::RobustScaler:
        return flow.add_robust_scaler(std::get<prep::RobustScalerOptions>(spec.options), spec.feature_route);
      case TransformKind::SimpleImputer:
        return flow.add_simple_imputer(std::get<prep::SimpleImputerOptions>(spec.options), spec.feature_route);
      case TransformKind::NeighborImputer:
        return flow.add_neighbor_imputer(std::get<prep::NeighborImputerOptions>(spec.options), spec.feature_route);
      case TransformKind::IterativeImputer:
        return flow.add_iterative_imputer(std::get<prep::IterativeImputerOptions>(spec.options), spec.feature_route);
      case TransformKind::OneHotEncoder:
        return flow.add_one_hot_encoder(std::get<prep::OneHotEncoderOptions>(spec.options), spec.feature_route);
      case TransformKind::FrequencyEncoder:
        return flow.add_frequency_encoder(std::get<prep::FrequencyEncoderOptions>(spec.options), spec.feature_route);
      case TransformKind::OrdinalEncoder:
        return flow.add_ordinal_encoder(std::get<prep::OrdinalEncoderOptions>(spec.options), spec.feature_route);
      case TransformKind::LabelEncoder:
        return flow.add_label_encoder(std::get<prep::LabelEncoderOptions>(spec.options), spec.feature_route);
      case TransformKind::VectorNormalizer:
        return flow.add_vector_normalizer(std::get<prep::VectorNormalizerOptions>(spec.options), spec.feature_route);
      case TransformKind::MaxAbsScaler:
        return flow.add_maxabs_scaler(std::get<prep::MaxAbsScalerOptions>(spec.options), spec.feature_route);
      case TransformKind::QuantileMap:
        return flow.add_quantile_map(std::get<prep::QuantileMapOptions>(spec.options), spec.feature_route);
      case TransformKind::PowerMap:
        return flow.add_power_map(std::get<prep::PowerMapOptions>(spec.options), spec.feature_route);
      case TransformKind::ThresholdBinarizer:
        return flow.add_threshold_binarizer(std::get<prep::ThresholdBinarizerOptions>(spec.options), spec.feature_route);
      case TransformKind::PolynomialLift:
        return flow.add_polynomial_lift(std::get<prep::PolynomialLiftOptions>(spec.options), spec.feature_route);
    }
    return Status::internal("append_transform_from_spec: unknown transform kind.");
  }

  [[nodiscard]] WorkflowModel clone_for_out_of_place_() const {
    if (workflow_.has_opaque_transforms()) {
      ::pluss::throw_error("learn(out-of-place): workflow has opaque transforms that cannot be cloned.");
    }
    if (!workflow_.model_spec().has_value() && workflow_.has_model()) {
      ::pluss::throw_error("learn(out-of-place): workflow model has no clonable ModelSpec.");
    }

    ::pluss::ai::nodej::Workflow cloned =
        workflow_.model_spec().has_value() ? resolve(make_flow(*workflow_.model_spec()), "make_flow(clone)")
                                           : ::pluss::ai::nodej::Workflow{};
    for (const auto& spec : workflow_.transform_specs()) {
      resolve(append_transform_from_spec_(cloned, spec), "append_transform_from_spec(clone)");
    }

    if (workflow_.trained()) {
      resolve(cloned.load_transform_states(resolve(workflow_.dump_transform_states(), "dump_transform_states(clone)")),
              "load_transform_states(clone)");
      if (cloned.has_model()) {
        resolve(cloned.load_model_state(resolve(workflow_.dump_model_state(), "dump_model_state(clone)")),
                "load_model_state(clone)");
      }
      cloned.mark_trained(true);
    }
    WorkflowModel out(std::move(cloned));
    out.feature_names_ = feature_names_;
    return out;
  }

  [[nodiscard]] static Series scalar_series_(double value) {
    return pluss::make_series(std::vector<double>{value}, {1}, Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
  }

  [[nodiscard]] static Series vector_series_(std::vector<double> values) {
    const auto n = static_cast<std::int64_t>(values.size());
    return pluss::make_series(
        std::move(values),
        {n},
        Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU));
  }

  [[nodiscard]] std::vector<MetricKind> resolved_metrics_(const AssessOptions& options) const {
    if (!options.metrics.empty()) return options.metrics;
    if (workflow_.task_kind() == TaskKind::Regression) return {MetricKind::Mse, MetricKind::Rmse, MetricKind::R2};
    if (workflow_.task_kind() == TaskKind::Classification) return {MetricKind::Accuracy};
    return {};
  }

  [[nodiscard]] static MetricReport as_metric_report_(const Series& metrics, pluss::msharp::RangeView::Of<MetricKind> requested) {
    MetricReport report{};
    report.requested = copy_range_values(requested);
    report.values = metrics.to(pluss::pCPU, ScalarType::Float64).reshape({metrics.numel()});

    // Slice each named metric straight off the already-materialized
    // `report.values` Series instead of round-tripping through a host
    // std::vector and rebuilding one fresh 1-element Series per metric.
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

  void refresh_linear_summary_() {
    coeff = Series::undefined();
    intercept = Series::undefined();
    // linear_parameters() throws for every non-linear model spec (tree,
    // ensemble, kernel, bayes, neighbor, cluster, decompose...) — the common
    // case, and this runs on every single learn() call, including once per
    // fold/candidate inside cross-validation and hyperparameter tuning. Check
    // the cheap, deterministic precondition ourselves first so that common
    // case is a plain early return instead of a thrown-and-caught exception.
    const auto& spec_opt = workflow_.model_spec();
    if (!spec_opt.has_value() || spec_opt->family != ModelFamily::Linear) return;
    try {
      const LinearParameters p = linear_parameters();
      coeff = p.coeff;
      intercept = p.intercept;
    } catch (...) {}
  }

  Workflow workflow_{};
  std::vector<std::string> feature_names_{};
};
[[nodiscard]] inline SplitOptions effective_workflow_split_options_(CrossValidationOptions cv, TaskKind task) {
  SplitOptions split = make_cross_validation_split_options(cv);
  if (split.reliability_defaults && task == TaskKind::Classification
      && !split.stratified
      && (split.plan == CVPlanKind::Holdout || split.plan == CVPlanKind::CrossVal)) {
    split.stratified = true;
  }
  return split;
}

[[nodiscard]] inline SplitPlan workflow_split_plan_(const LabeledBatch& batch,
                                                    const SplitOptions& split,
                                                    TaskKind task,
                                                    const char* where) {
  if (split.plan == CVPlanKind::Holdout) {
    const auto holdout = (split.stratified && task == TaskKind::Classification)
                             ? resolve(pluss::ai::nodej::strat_holdout_split(batch.y, split),
                                       std::string(where) + ".strat_holdout_split")
                             : resolve(pluss::ai::nodej::holdout_split(batch.features.sample_count(), split),
                                       std::string(where) + ".holdout_split");
    SplitPlan plan{};
    plan.folds.push_back(holdout);
    return plan;
  }
  if (split.plan == CVPlanKind::CrossVal) {
    if (split.stratified && task == TaskKind::Classification) {
      return resolve(pluss::ai::nodej::stratified_crossval_plan(batch.y, split),
                     std::string(where) + ".stratified_crossval_plan");
    }
    return resolve(pluss::ai::nodej::crossval_plan(batch.features.sample_count(), split),
                   std::string(where) + ".crossval_plan");
  }
  if (split.plan == CVPlanKind::StratifiedCrossVal) {
    if (task != TaskKind::Classification) {
      ::pluss::throw_error(std::string(where) + ": StratifiedCrossVal requires classification task.");
    }
    return resolve(pluss::ai::nodej::stratified_crossval_plan(batch.y, split),
                   std::string(where) + ".stratified_crossval_plan");
  }
  if (split.plan == CVPlanKind::TSCrossVal) {
    return resolve(pluss::ai::nodej::ts_crossval_plan(batch.features.sample_count(), split),
                   std::string(where) + ".ts_crossval_plan");
  }
  if (split.plan == CVPlanKind::GroupCrossVal) {
    if (!split.groups.defined()) {
      ::pluss::throw_error(std::string(where) + ": GroupCrossVal requires CrossValidationOptions.groups.");
    }
    if (split.groups.numel() != batch.features.sample_count()) {
      ::pluss::throw_error(std::string(where) + ": GroupCrossVal requires groups length to match sample count.");
    }
    return resolve(pluss::ai::nodej::group_crossval_plan(split.groups, split),
                   std::string(where) + ".group_crossval_plan");
  }
  ::pluss::throw_error(std::string(where) + ": only Holdout/CrossVal/StratifiedCrossVal/TSCrossVal/GroupCrossVal are currently supported.");
}

[[nodiscard]] inline std::vector<FoldBatchPair> materialize_fold_batches_(const LabeledBatch& batch,
                                                                          const SplitPlan& plan,
                                                                          const char* where) {
  std::vector<FoldBatchPair> out{};
  out.reserve(plan.folds.size());
  for (const auto& fold : plan.folds) {
    out.push_back(FoldBatchPair{
        .train = resolve(pluss::ai::nodej::slice_labeled_batch(batch, fold.train_indices),
                         std::string(where) + ".slice_labeled_batch(train)"),
        .test = resolve(pluss::ai::nodej::slice_labeled_batch(batch, fold.test_indices),
                        std::string(where) + ".slice_labeled_batch(test)"),
    });
  }
  return out;
}

[[nodiscard]] inline CrossValidationReport cross_validate_batches_(const WorkflowModel& model,
                                                                   const std::vector<FoldBatchPair>& fold_batches,
                                                                   const SplitPlan& plan,
                                                                   std::int64_t max_parallel,
                                                                   TrainOptions train_options,
                                                                   AssessOptions assess_options,
                                                                   const char* where) {
  if (!model.workflow().has_model()) {
    ::pluss::throw_error(std::string(where) + ": workflow has no model.");
  }
  if (fold_batches.size() != plan.folds.size()) {
    ::pluss::throw_error(std::string(where) + ": fold batch count must match split plan fold count.");
  }

  const auto spec_opt = model.workflow().model_spec();
  const TaskKind task = spec_opt.has_value() ? spec_opt->task : model.workflow().task_kind();
  const std::vector<MetricKind> metrics = default_assess_metrics_(task, assess_options);
  if (metrics.empty()) {
    ::pluss::throw_error(std::string(where) + ": no assessment metrics requested.");
  }
  if (plan.folds.empty()) {
    ::pluss::throw_error(std::string(where) + ": empty fold plan.");
  }

  CrossAssessReport report{};
  report.metrics = metrics;
  report.folds.resize(plan.folds.size());
  assess_options.metrics = metrics;

  const std::size_t fold_n = plan.folds.size();
  const auto evaluate_fold = [&](std::size_t fold_index) -> FoldAssessResult {
    const auto& fold = plan.folds[fold_index];
    const auto& fold_batch = fold_batches[fold_index];
    const WorkflowModel trained = model.learn(fold_batch.train, train_options);
    const Series fold_metrics =
        trained.assess_series(fold_batch.test, assess_options)
            .to(ScalarType::Float64)
            .reshape({static_cast<std::int64_t>(metrics.size())});
    return FoldAssessResult{
        .metrics = fold_metrics,
        .train_indices = fold.train_indices,
        .test_indices = fold.test_indices,
    };
  };

  std::size_t worker_n = 1;
  if (max_parallel == 0) {
    worker_n = static_cast<std::size_t>(std::max<unsigned int>(1, std::thread::hardware_concurrency()));
  } else if (max_parallel > 0) {
    worker_n = static_cast<std::size_t>(max_parallel);
  }
  worker_n = std::max<std::size_t>(1, std::min<std::size_t>(worker_n, fold_n));

  if (worker_n <= 1 || fold_n <= 1) {
    for (std::size_t i = 0; i < fold_n; ++i) {
      report.folds[i] = evaluate_fold(i);
    }
  } else {
    for (std::size_t begin = 0; begin < fold_n; begin += worker_n) {
      const std::size_t end = std::min<std::size_t>(begin + worker_n, fold_n);
      std::vector<std::future<FoldAssessResult>> futures{};
      futures.reserve(end - begin);
      for (std::size_t i = begin; i < end; ++i) {
        futures.push_back(std::async(std::launch::async, evaluate_fold, i));
      }
      for (std::size_t i = begin; i < end; ++i) {
        report.folds[i] = futures[i - begin].get();
      }
    }
  }

  const std::int64_t fold_count = static_cast<std::int64_t>(report.folds.size());
  const std::int64_t metric_count = static_cast<std::int64_t>(metrics.size());
  Series fold_matrix = report.folds.front().metrics.reshape({1, metric_count});
  for (std::size_t i = 1; i < report.folds.size(); ++i) {
    fold_matrix = fold_matrix.unite({report.folds[i].metrics.reshape({1, metric_count})}, 0);
  }
  fold_matrix = fold_matrix.reshape({fold_count, metric_count});
  report.mean_metrics = fold_matrix.mean(0, false).reshape({metric_count});
  const Series centered = fold_matrix.sub(report.mean_metrics.reshape({1, metric_count}));
  report.std_metrics = centered.square().mean(0, false).sqrt().reshape({metric_count});

  return CrossValidationReport{
      .report = report,
      .mean = metric_report_from_series_(report.mean_metrics, metrics),
      .stdev = metric_report_from_series_(report.std_metrics, metrics),
  };
}

[[nodiscard]] inline CrossValidationReport cross_validate_plan_(const WorkflowModel& model,
                                                                const LabeledBatch& batch,
                                                                const SplitPlan& plan,
                                                                std::int64_t max_parallel,
                                                                TrainOptions train_options,
                                                                AssessOptions assess_options,
                                                                const char* where) {
  resolve(validate_labeled(batch), std::string(where) + ".validate_labeled");
  const std::vector<FoldBatchPair> fold_batches = materialize_fold_batches_(batch, plan, where);
  return cross_validate_batches_(model, fold_batches, plan, max_parallel, train_options, assess_options, where);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const WorkflowModel& model,
                                                          const LabeledBatch& batch,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  resolve(validate_labeled(batch), "cross_validate(model).validate_labeled");
  if (!model.workflow().has_model()) {
    ::pluss::throw_error("cross_validate(model,...): workflow has no model.");
  }

  const auto spec_opt = model.workflow().model_spec();
  const TaskKind task = spec_opt.has_value() ? spec_opt->task : model.workflow().task_kind();
  const SplitOptions split = effective_workflow_split_options_(cv, task);
  const SplitPlan plan = workflow_split_plan_(batch, split, task, "cross_validate(model)");
  return cross_validate_plan_(model, batch, plan, split.max_parallel, train_options, assess_options, "cross_validate(model)");
}

[[nodiscard]] inline CrossValidationReport cross_validate(const WorkflowModel& model,
                                                          const pluss::table::DataFrame& frame,
                                                          const DataFrameBatchOptions& data_options,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options), "cross_validate(model).labeled_batch_from_dataframe");
  return cross_validate(model, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const WorkflowModel& model,
                                                          const pluss::table::DataFrame& x,
                                                          const Series& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "cross_validate(model,x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "cross_validate(model,x,y)");
  return cross_validate(model, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const WorkflowModel& model,
                                                          const pluss::table::DataFrame& x,
                                                          const pluss::table::Thread& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(model, x, as_stratify_series(y), cv, train_options, assess_options);
}

[[nodiscard]] inline Series cross_validate_scores(const WorkflowModel& model,
                                                  const LabeledBatch& batch,
                                                  MetricKind metric,
                                                  CrossValidationOptions cv = {},
                                                  TrainOptions train_options = {},
                                                  AssessOptions assess_options = {}) {
  if (!model.workflow().model_spec().has_value()) {
    ::pluss::throw_error("cross_validate_scores(model,...): workflow model has no model specification.");
  }
  return cross_validate_scores(*model.workflow().model_spec(), batch, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline double cross_validate_score(const WorkflowModel& model,
                                                 const LabeledBatch& batch,
                                                 MetricKind metric,
                                                 CrossValidationOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  if (!model.workflow().model_spec().has_value()) {
    ::pluss::throw_error("cross_validate_score(model,...): workflow model has no model specification.");
  }
  return cross_validate_score(*model.workflow().model_spec(), batch, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline Series cross_validate_scores(const WorkflowModel& model,
                                                  const pluss::table::DataFrame& x,
                                                  const Series& y,
                                                  MetricKind metric,
                                                  CrossValidationOptions cv = {},
                                                  TrainOptions train_options = {},
                                                  AssessOptions assess_options = {}) {
  if (!model.workflow().model_spec().has_value()) {
    ::pluss::throw_error("cross_validate_scores(model,x,y): workflow model has no model specification.");
  }
  return cross_validate_scores(*model.workflow().model_spec(), x, y, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline double cross_validate_score(const WorkflowModel& model,
                                                 const pluss::table::DataFrame& x,
                                                 const Series& y,
                                                 MetricKind metric,
                                                 CrossValidationOptions cv = {},
                                                 TrainOptions train_options = {},
                                                 AssessOptions assess_options = {}) {
  if (!model.workflow().model_spec().has_value()) {
    ::pluss::throw_error("cross_validate_score(model,x,y): workflow model has no model specification.");
  }
  return cross_validate_score(*model.workflow().model_spec(), x, y, metric, cv, train_options, assess_options);
}

[[nodiscard]] inline ConfusionTable confusion_table(const WorkflowModel& model,
                                                    const LabeledBatch& batch,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  const Series predicted = model.infer(batch.features, infer_options);
  return ::pluss::ai::lymph::confusion_table(batch.y, predicted, options);
}

[[nodiscard]] inline ClassReport classification_summary(const WorkflowModel& model,
                                                        const LabeledBatch& batch,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  const Series predicted = model.infer(batch.features, infer_options);
  return ::pluss::ai::lymph::classification_summary(batch.y, predicted, options);
}

[[nodiscard]] inline Series classification_metric_series(const WorkflowModel& model,
                                                         const LabeledBatch& batch,
                                                         const AssessOptions& options = {}) {
  return model.assess_series(batch, options);
}

[[nodiscard]] inline Series classification_metric_series(const WorkflowModel& model,
                                                         const pluss::table::DataFrame& x,
                                                         const Series& y,
                                                         const AssessOptions& options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "classification_metric_series(model,x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "classification_metric_series(model,x,y)");
  return classification_metric_series(model, batch, options);
}

[[nodiscard]] inline Series classification_metric_series(const WorkflowModel& model,
                                                         const pluss::table::DataFrame& x,
                                                         const pluss::table::Thread& y,
                                                         const AssessOptions& options = {}) {
  return classification_metric_series(model, x, as_stratify_series(y), options);
}

[[nodiscard]] inline ConfusionTable confusion_table(const WorkflowModel& model,
                                                    const pluss::table::DataFrame& x,
                                                    const Series& y,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "confusion_table(model,x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "confusion_table(model,x,y)");
  return ::pluss::ai::lymph::confusion_table(model, batch, infer_options, options);
}

[[nodiscard]] inline ClassReport classification_summary(const WorkflowModel& model,
                                                        const pluss::table::DataFrame& x,
                                                        const Series& y,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "classification_summary(model,x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "classification_summary(model,x,y)");
  return ::pluss::ai::lymph::classification_summary(model, batch, infer_options, options);
}


} // namespace pluss::ai::lymph
