#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/prep.h"
#include "pluss/ai/lymph/metrics.h"
#include "pluss/ai/lymph/inspection.h"
#include "pluss/ai/lymph/stage.h"
#include "pluss/ai/lymph/transform.h"

namespace pluss::ai::lymph::detail {

class ModelCore final {
public:
  explicit ModelCore(const ModelSpec& spec)
      : workflow_(resolve(::pluss::ai::nodej::make_flow(spec), "make_flow")) {
    refresh_linear_summary_();
  }

  explicit ModelCore(::pluss::ai::nodej::Workflow workflow)
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
  ModelCore& set_nested_param_(std::string_view path, double value) {
    resolve(workflow_.set_nested_param(path, value), "set_nested_param_");
    return *this;
  }
  [[nodiscard]] std::string flow_summary() const { return workflow_.flow_summary(); }
  [[nodiscard]] std::string save_blueprint() const {
    return resolve(workflow_.save_blueprint(), "save_blueprint");
  }
  ModelCore& load_blueprint_(std::string_view text) {
    resolve(workflow_.load_blueprint(text), "load_blueprint_");
    refresh_linear_summary_();
    return *this;
  }
  ModelCore& set_stage_key_(std::int64_t stage_index, std::string stage_key) {
    resolve(workflow_.set_stage_key(stage_index, std::move(stage_key)), "set_stage_key_");
    return *this;
  }
  ModelCore& set_stage_route_(std::int64_t stage_index, pluss::msharp::RangeView::Int feature_route) {
    resolve(workflow_.set_stage_route(stage_index, feature_route), "set_stage_route_");
    return *this;
  }
  ModelCore& set_stage_remainder_(std::int64_t stage_index, RouteRemainderPolicy remainder) {
    resolve(workflow_.set_stage_remainder(stage_index, remainder), "set_stage_remainder_");
    return *this;
  }

  ModelCore& standard_scaler(prep::StandardScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_standard_scaler(options, feature_route), "standard_scaler");
    return *this;
  }

  ModelCore& minmax_scaler(prep::MinMaxScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_minmax_scaler(options, feature_route), "minmax_scaler");
    return *this;
  }

  ModelCore& robust_scaler(prep::RobustScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_robust_scaler(options, feature_route), "robust_scaler");
    return *this;
  }

  ModelCore& simple_imputer(prep::SimpleImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_simple_imputer(options, feature_route), "simple_imputer");
    return *this;
  }

  ModelCore& neighbor_imputer(prep::NeighborImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_neighbor_imputer(options, feature_route), "neighbor_imputer");
    return *this;
  }

  ModelCore& iterative_imputer(prep::IterativeImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_iterative_imputer(options, feature_route), "iterative_imputer");
    return *this;
  }

  ModelCore& one_hot_encoder(prep::OneHotEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_one_hot_encoder(options, feature_route), "one_hot_encoder");
    return *this;
  }

  ModelCore& frequency_encoder(prep::FrequencyEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_frequency_encoder(options, feature_route), "frequency_encoder");
    return *this;
  }

  ModelCore& ordinal_encoder(prep::OrdinalEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_ordinal_encoder(options, feature_route), "ordinal_encoder");
    return *this;
  }

  ModelCore& label_encoder(prep::LabelEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_label_encoder(options, feature_route), "label_encoder");
    return *this;
  }

  ModelCore& vector_normalizer(prep::VectorNormalizerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_vector_normalizer(options, feature_route), "vector_normalizer");
    return *this;
  }

  ModelCore& maxabs_scaler(prep::MaxAbsScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_maxabs_scaler(options, feature_route), "maxabs_scaler");
    return *this;
  }

  ModelCore& quantile_map(prep::QuantileMapOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_quantile_map(options, feature_route), "quantile_map");
    return *this;
  }

  ModelCore& power_map(prep::PowerMapOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_power_map(options, feature_route), "power_map");
    return *this;
  }

  ModelCore& threshold_binarizer(prep::ThresholdBinarizerOptions options = {},
                                     pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_threshold_binarizer(options, feature_route), "threshold_binarizer");
    return *this;
  }

  ModelCore& polynomial_lift(prep::PolynomialLiftOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    resolve(workflow_.add_polynomial_lift(options, feature_route), "polynomial_lift");
    return *this;
  }

  ModelCore& function_transformer(prep::FunctionTransformer transformer) {
    resolve(workflow_.add_function_transformer(std::move(transformer)), "function_transformer");
    return *this;
  }

  ModelCore& feature_transformer(prep::FeatureTransformer transformer) {
    resolve(workflow_.add_feature_transformer(std::move(transformer)), "feature_transformer");
    return *this;
  }

  // Model::append(Stage, ColumnRouter) -- the one method the redesign
  // replaces the 16 builders above with (kept, unmodified, alongside this
  // one per LYMPH_REDESIGN_PLAN.md phase 2: nothing above breaks while this
  // gets proven out). Deliberately does not resolve `router` here: .append()
  // is always called before any DataFrame exists (at model-building time),
  // so resolution is deferred to whichever learn_(...) overload first sees
  // real column names -- see materialize_pending_stages_() below. A bare
  // learn_(LabeledBatch) has no column names to resolve against at all and
  // throws if any stage is still pending.
  ModelCore& append(Stage stage, ColumnRouter router) {
    pending_stages_.emplace_back(std::move(stage), std::move(router));
    return *this;
  }

  ModelCore& learn_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    if (!pending_stages_.empty()) {
      ::pluss::throw_error(
          "learn_: " + std::to_string(pending_stages_.size()) +
          " append()'d stage(s) still pending -- a bare LabeledBatch carries no column names to "
          "resolve their ColumnRouter against. Use the (frame, DataFrameBatchOptions) or (x, y) "
          "overload instead.");
    }
    resolve(workflow_.learn(apply_target_transform_forward_(batch), options), "learn");
    refresh_linear_summary_();
    return *this;
  }

  ModelCore& learn_(const pluss::table::DataFrame& frame,
                        const DataFrameBatchOptions& data_options,
                        const TrainOptions& options = {}) {
    category_maps_.clear();
    const auto names = resolve_feature_names_from_options_(frame, data_options, "learn_");
    materialize_pending_stages_(frame, names);
    const LabeledBatch batch = resolve(
        labeled_batch_from_dataframe(frame, data_options, nullptr, &category_maps_),
        "learn_.labeled_batch_from_dataframe");
    resolve(workflow_.learn(apply_target_transform_forward_(batch), options), "learn");
    feature_names_ = names;
    refresh_linear_summary_();
    return *this;
  }

  ModelCore& learn_(const pluss::table::DataFrame& x,
                        const Series& y,
                        const TrainOptions& options = {}) {
    category_maps_.clear();
    const auto names = default_series_feature_names_(x, "learn_(x,y)");
    materialize_pending_stages_(x, names);
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "learn_(x,y)", nullptr, &category_maps_);
    feature_names_ = names;
    return learn_(batch, options);
  }

  ModelCore& learn_(const pluss::table::DataFrame& x,
                        const pluss::table::Thread& y,
                        const TrainOptions& options = {}) {
    return learn_(x, as_stratify_series(y), options);
  }

  [[nodiscard]] ModelCore learn(const LabeledBatch& batch, const TrainOptions& options = {}) const {
    ModelCore out = clone_for_out_of_place_();
    out.learn_(batch, options);
    return out;
  }

  [[nodiscard]] ModelCore learn(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const TrainOptions& options = {}) const {
    ModelCore out = clone_for_out_of_place_();
    out.learn_(frame, data_options, options);
    return out;
  }

  [[nodiscard]] ModelCore learn(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const TrainOptions& options = {}) const {
    ModelCore out = clone_for_out_of_place_();
    out.learn_(x, y, options);
    return out;
  }

  [[nodiscard]] ModelCore learn(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const TrainOptions& options = {}) const {
    ModelCore out = clone_for_out_of_place_();
    out.learn_(x, y, options);
    return out;
  }

  // Folds a TargetTransform onto this model as optional model state.
  // Chainable, so it composes with
  // .append(...) the same way: `model.append(stage, router).target_transform
  // (transform::log1p())`. Regression only -- there is no meaningful
  // inverse-transform of a predicted *class* -- and every learn_/infer/
  // assess_series overload funnels through the one chokepoint each already
  // shares, so this applies uniformly regardless of which overload a caller
  // reaches for.
  ModelCore& target_transform(TargetTransform transform) {
    if (!transform.forward || !transform.inverse) {
      ::pluss::throw_error("ModelCore::target_transform: transform.forward and transform.inverse must both be set.");
    }
    if (task_kind() != TaskKind::Regression) {
      ::pluss::throw_error(
          "ModelCore::target_transform: only meaningful for TaskKind::Regression models "
          "(there is no invertible mapping back from a predicted class through a target transform).");
    }
    target_transform_ = std::move(transform);
    return *this;
  }

  [[nodiscard]] bool has_target_transform() const noexcept { return target_transform_.has_value(); }

  [[nodiscard]] const TargetTransform& target_transform() const {
    if (!target_transform_.has_value()) {
      ::pluss::throw_error("ModelCore::target_transform: no transform has been set.");
    }
    return *target_transform_;
  }

  [[nodiscard]] Series map(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(workflow_.map(batch, options), "map");
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& frame,
                           const DataFrameBatchOptions& data_options,
                           const InferOptions& options = {}) const {
    const auto batch = resolve(
        feature_batch_from_dataframe(frame, data_options, &category_maps_, nullptr),
        "map.feature_batch_from_dataframe");
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
    category_maps_.clear();
    const auto batch = resolve(
        feature_batch_from_dataframe(frame, data_options, nullptr, &category_maps_),
        "learn_map.feature_batch_from_dataframe");
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
    const Series predicted = resolve(workflow_.infer(batch, options), "infer");
    return target_transform_.has_value() ? target_transform_->inverse(predicted) : predicted;
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& frame,
                             const DataFrameBatchOptions& data_options,
                             const InferOptions& options = {}) const {
    const auto batch = resolve(
        feature_batch_from_dataframe(frame, data_options, &category_maps_, nullptr),
        "infer.feature_batch_from_dataframe");
    return infer(batch, options);
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& x,
                             const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "infer(x)");
    return infer(x, default_data_options_(names), options);
  }

  [[nodiscard]] Series infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return resolve(workflow_.infer_scores(batch, options), "infer_scores");
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const InferOptions& options = {}) const {
    const auto batch = resolve(
        feature_batch_from_dataframe(frame, data_options, &category_maps_, nullptr),
        "infer_scores.feature_batch_from_dataframe");
    return infer_scores(batch, options);
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& x,
                                    const InferOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "infer_scores(x)");
    return infer_scores(x, default_data_options_(names), options);
  }

  [[nodiscard]] Series assess_series(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    if (target_transform_.has_value()) {
      // Score against the ORIGINAL (untransformed) y -- infer(...) above
      // already inverse-transforms, so calling workflow_.assess(batch, ...)
      // directly here would silently compare transformed-space predictions
      // against batch.y, which is in the original space. Keep the fix folded
      // into the chokepoint every assess()/assess_series() overload shares.
      const Series predicted = infer(batch.features, InferOptions{});
      return regression_metric_series(batch.y, predicted, resolved_metrics_(options));
    }
    return resolve(workflow_.assess(batch, options), "assess");
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& frame,
                                     const DataFrameBatchOptions& data_options,
                                     const AssessOptions& options = {}) const {
    const LabeledBatch batch = resolve(
        labeled_batch_from_dataframe(frame, data_options, &category_maps_, nullptr),
        "assess.labeled_batch_from_dataframe");
    return assess_series(batch, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const Series& y,
                                     const AssessOptions& options = {}) const {
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x, "assess(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x, y, names, "assess(x,y)", &category_maps_, nullptr);
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
      ::pluss::throw_error("ModelCore::permutation_impact requires a model specification.");
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
      ::pluss::throw_error("ModelCore::dependence_curve requires a model specification.");
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
      ::pluss::throw_error("ModelCore::dependence_surface requires a model specification.");
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
    throw_if_target_transform_unsaveable_();
    resolve(io::save(path, workflow_, train_batch, train_options, std::move(metadata), category_maps_), "save");
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const Series& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    throw_if_target_transform_unsaveable_();
    const auto names = !feature_names_.empty() ? feature_names_ : default_series_feature_names_(x_train, "save(x,y)");
    const LabeledBatch batch = labeled_from_xy_(x_train, y_train, names, "save(x,y)", &category_maps_, nullptr);
    resolve(io::save(path, workflow_, batch, train_options, std::move(metadata), category_maps_), "save");
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const pluss::table::Thread& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    save(path, x_train, as_stratify_series(y_train), train_options, std::move(metadata));
  }

  // (frame, DataFrameBatchOptions) sibling -- see confusion_table's twin
  // comment further down for why this exists alongside the (x, y) form. Also
  // reuses the model's own frozen category_maps_ (as `existing_categories`)
  // to encode the saved reference train_batch, so it stays consistent with
  // the codes the model was actually trained on rather than a fresh refit.
  void save(const std::string& path,
            const pluss::table::DataFrame& frame,
            const DataFrameBatchOptions& data_options,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    throw_if_target_transform_unsaveable_();
    const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options, &category_maps_, nullptr),
                                       "save(frame,data_options).labeled_batch_from_dataframe");
    resolve(io::save(path, workflow_, batch, train_options, std::move(metadata), category_maps_), "save");
  }

  [[nodiscard]] static ModelCore restore(const io::FlowBundle& bundle) {
    ModelCore out(resolve(io::restore(bundle), "restore"));
    out.category_maps_ = bundle.category_maps;
    return out;
  }

  [[nodiscard]] static ModelCore load(const std::string& path) {
    const io::FlowBundle bundle = resolve(io::load(path), "load");
    return restore(bundle);
  }

  [[nodiscard]] std::vector<std::vector<Series>> dump_transform_states() const {
    return resolve(workflow_.dump_transform_states(), "dump_transform_states");
  }

  [[nodiscard]] std::vector<Series> dump_model_state() const {
    return resolve(workflow_.dump_model_state(), "dump_model_state");
  }

  ModelCore& load_transform_states_(pluss::msharp::RangeView::Of<std::vector<Series>> states) {
    resolve(workflow_.load_transform_states(states), "load_transform_states");
    refresh_linear_summary_();
    return *this;
  }

  ModelCore& load_model_state_(pluss::msharp::RangeView::Series state) {
    resolve(workflow_.load_model_state(state), "load_model_state");
    refresh_linear_summary_();
    return *this;
  }

  ModelCore& mark_trained_(bool trained = true) {
    workflow_.mark_trained(trained);
    if (!trained) {
      coeff = Series::undefined();
      intercept = Series::undefined();
    } else {
      refresh_linear_summary_();
    }
    return *this;
  }

  [[nodiscard]] const ::pluss::ai::nodej::Workflow& workflow() const noexcept { return workflow_; }
  [[nodiscard]] ::pluss::ai::nodej::Workflow& workflow() noexcept { return workflow_; }

  // Out-of-place copy that dumps + reloads trained transform/model state
  // instead of retraining -- cheap even for an already-fitted model. Throws
  // under the same conditions as the out-of-place learn(...) path (opaque
  // transforms, or a trained model with no clonable ModelSpec).
  [[nodiscard]] ModelCore clone() const { return clone_for_out_of_place_(); }

private:
  // target_transform_ support -- see the public .target_transform(...)
  // chainable setter further up for the field's role.
  void check_target_transform_inverse_(const Series& y) const {
    if (!target_transform_->check_inverse) return;
    const Series roundtrip = target_transform_->inverse(target_transform_->forward(y));
    const double diff = max_abs_diff(roundtrip, y);
    if (!(diff <= target_transform_->check_inverse_tolerance)) {
      ::pluss::throw_error("ModelCore::target_transform: transform \"" + target_transform_->name +
                           "\" does not round-trip (forward then inverse max abs diff " + std::to_string(diff) +
                           " exceeds tolerance " + std::to_string(target_transform_->check_inverse_tolerance) +
                           "). Check it is actually invertible over this y's range.");
    }
  }

  [[nodiscard]] LabeledBatch apply_target_transform_forward_(LabeledBatch batch) const {
    if (!target_transform_.has_value()) return batch;
    check_target_transform_inverse_(batch.y);
    batch.y = target_transform_->forward(batch.y);
    return batch;
  }

  // target_transform_ closures aren't serializable (same limitation
  // prep::FunctionTransformer already has) -- fail loudly at save() rather
  // than silently dropping the transform on a save/load round-trip.
  void throw_if_target_transform_unsaveable_() const {
    if (target_transform_.has_value()) {
      ::pluss::throw_error(
          "ModelCore::save: target_transform is not persistable yet (forward/inverse closures aren't "
          "serializable) -- save before calling .target_transform(...), or reattach the same transform "
          "again after .load(...).");
    }
  }

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
                                                     const char* where,
                                                     const CategoryMaps* existing_categories = nullptr,
                                                     CategoryMaps* fitted_categories_out = nullptr) {
    const auto x_batch =
        resolve(feature_batch_from_dataframe(x, default_data_options_(feature_names), existing_categories, fitted_categories_out),
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

  // Resolves every (Stage, ColumnRouter) queued by append() against `frame`/
  // `ordered_names` -- the first real column-name context any learn_(...)
  // overload sees -- and folds each into workflow_ via the same
  // append_transform_from_spec_() dispatch clone_for_out_of_place_() below
  // already uses, so materializing a pending stage is not a second code
  // path, just a different TransformSpec source. Small, fixed-size
  // bookkeeping (a handful of prep stages, never a per-row/per-feature
  // loop), so this is plain control flow, not something with a
  // Series/vectorization angle -- the actual per-row work still happens
  // entirely inside NodeJ's Flow/TransformerBase once materialized,
  // unchanged by any of this.
  void materialize_pending_stages_(const pluss::table::DataFrame& frame, const std::vector<std::string>& ordered_names) {
    for (auto& [stage, router] : pending_stages_) {
      const TransformSpec spec{
          .kind = stage.kind,
          .options = stage.options,
          .feature_route = router.resolve(frame, ordered_names),
          .remainder = router.remainder,
      };
      resolve(append_transform_from_spec_(workflow_, spec), "append(pending stage)");
    }
    pending_stages_.clear();
  }

  [[nodiscard]] static Status append_transform_from_spec_(::pluss::ai::nodej::Workflow& flow, const TransformSpec& spec) {
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

  [[nodiscard]] ModelCore clone_for_out_of_place_() const {
    ModelCore out(resolve(workflow_.clone(), "Flow::clone"));
    out.feature_names_ = feature_names_;
    out.category_maps_ = category_maps_;
    // Un-materialized append()'d stages travel with the clone too -- the
    // non-mutating learn(...) siblings clone first, then call the matching
    // mutating learn_(...) overload on the clone, which is exactly where
    // these get resolved and folded in.
    out.pending_stages_ = pending_stages_;
    out.target_transform_ = target_transform_;
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

  ::pluss::ai::nodej::Workflow workflow_{};
  std::vector<std::string> feature_names_{};
  // Fitted at learn() time for any DataNotePolicy::Factorize column;
  // reused (not refit) by every later infer()/map()/assess()/save() call so
  // a raw text column keeps encoding to the same codes throughout this
  // model's lifetime, including after save()/load(). See nodej/data.h's
  // CategoryMaps doc comment for the fit/reuse contract.
  CategoryMaps category_maps_{};
  // append()'d (Stage, ColumnRouter) pairs not yet folded into workflow_ --
  // cleared by materialize_pending_stages_() the first time a DataFrame-
  // based learn_(...) overload runs. See append()'s own doc comment.
  std::vector<std::pair<Stage, ColumnRouter>> pending_stages_{};
  std::optional<TargetTransform> target_transform_{};
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

// Turns a filled-in CrossAssessReport (one FoldAssessResult per fold already
// computed, by any dispatch strategy) into fold mean/stdev + the final
// CrossValidationReport -- the tail every cross-validation caller needs,
// factored out so cross_validate_batches_ and selection::compare's flattened
// candidate-fold scheduler don't each derive it separately.
[[nodiscard]] inline CrossValidationReport cross_validation_report_from_folds_(CrossAssessReport report) {
  const std::vector<MetricKind>& metrics = report.metrics;
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

[[nodiscard]] inline CrossValidationReport cross_validate_batches_(const ModelCore& model,
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
    const ModelCore trained = model.learn(fold_batch.train, train_options);
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

  return cross_validation_report_from_folds_(std::move(report));
}

[[nodiscard]] inline CrossValidationReport cross_validate_plan_(const ModelCore& model,
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

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelCore& model,
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

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelCore& model,
                                                          const pluss::table::DataFrame& frame,
                                                          const DataFrameBatchOptions& data_options,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const LabeledBatch batch = resolve(labeled_batch_from_dataframe(frame, data_options), "cross_validate(model).labeled_batch_from_dataframe");
  return cross_validate(model, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelCore& model,
                                                          const pluss::table::DataFrame& x,
                                                          const Series& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "cross_validate(model,x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "cross_validate(model,x,y)");
  return cross_validate(model, batch, cv, train_options, assess_options);
}

[[nodiscard]] inline CrossValidationReport cross_validate(const ModelCore& model,
                                                          const pluss::table::DataFrame& x,
                                                          const pluss::table::Thread& y,
                                                          CrossValidationOptions cv = {},
                                                          TrainOptions train_options = {},
                                                          AssessOptions assess_options = {}) {
  return cross_validate(model, x, as_stratify_series(y), cv, train_options, assess_options);
}

[[nodiscard]] inline Series cross_validate_scores(const ModelCore& model,
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

[[nodiscard]] inline double cross_validate_score(const ModelCore& model,
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

[[nodiscard]] inline Series cross_validate_scores(const ModelCore& model,
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

[[nodiscard]] inline double cross_validate_score(const ModelCore& model,
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

[[nodiscard]] inline ConfusionTable confusion_table(const ModelCore& model,
                                                    const LabeledBatch& batch,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  const Series predicted = model.infer(batch.features, infer_options);
  return ::pluss::ai::lymph::confusion_table(batch.y, predicted, options);
}

[[nodiscard]] inline ClassReport classification_summary(const ModelCore& model,
                                                        const LabeledBatch& batch,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  const Series predicted = model.infer(batch.features, infer_options);
  return ::pluss::ai::lymph::classification_summary(batch.y, predicted, options);
}

[[nodiscard]] inline Series classification_metric_series(const ModelCore& model,
                                                         const LabeledBatch& batch,
                                                         const AssessOptions& options = {}) {
  return model.assess_series(batch, options);
}

[[nodiscard]] inline Series classification_metric_series(const ModelCore& model,
                                                         const pluss::table::DataFrame& x,
                                                         const Series& y,
                                                         const AssessOptions& options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "classification_metric_series(model,x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "classification_metric_series(model,x,y)");
  return classification_metric_series(model, batch, options);
}

[[nodiscard]] inline Series classification_metric_series(const ModelCore& model,
                                                         const pluss::table::DataFrame& x,
                                                         const pluss::table::Thread& y,
                                                         const AssessOptions& options = {}) {
  return classification_metric_series(model, x, as_stratify_series(y), options);
}

[[nodiscard]] inline ConfusionTable confusion_table(const ModelCore& model,
                                                    const pluss::table::DataFrame& x,
                                                    const Series& y,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "confusion_table(model,x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "confusion_table(model,x,y)");
  return confusion_table(model, batch, infer_options, options);
}

// (frame, DataFrameBatchOptions) sibling of the (x, y) overload above --
// routes through `model.infer(frame, data_options, ...)`, which (unlike the
// (x, y) form, which has no DataFrameBatchOptions to carry a note_policy on)
// correctly reuses the model's own frozen `category_maps_` for any
// DataNotePolicy::Factorize column, so a confusion matrix built this way on
// held-out data stays code-consistent with how the model was trained.
[[nodiscard]] inline ConfusionTable confusion_table(const ModelCore& model,
                                                    const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& data_options,
                                                    const InferOptions& infer_options = {},
                                                    const ConfusionOptions& options = {}) {
  if (!data_options.label_feature.has_value()) {
    ::pluss::throw_error("confusion_table(model,frame,data_options): data_options.label_feature is required.");
  }
  const Series y = frame.features(*data_options.label_feature).as_series().reshape({frame.ncases()});
  const Series predicted = model.infer(frame, data_options, infer_options);
  return ::pluss::ai::lymph::confusion_table(y, predicted, options);
}

[[nodiscard]] inline ClassReport classification_summary(const ModelCore& model,
                                                        const pluss::table::DataFrame& x,
                                                        const Series& y,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "classification_summary(model,x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "classification_summary(model,x,y)");
  return classification_summary(model, batch, infer_options, options);
}

// (frame, DataFrameBatchOptions) sibling -- see confusion_table's twin above
// for why this exists alongside the (x, y) form.
[[nodiscard]] inline ClassReport classification_summary(const ModelCore& model,
                                                        const pluss::table::DataFrame& frame,
                                                        const DataFrameBatchOptions& data_options,
                                                        const InferOptions& infer_options = {},
                                                        const ClassReportOptions& options = {}) {
  if (!data_options.label_feature.has_value()) {
    ::pluss::throw_error("classification_summary(model,frame,data_options): data_options.label_feature is required.");
  }
  const Series y = frame.features(*data_options.label_feature).as_series().reshape({frame.ncases()});
  const Series predicted = model.infer(frame, data_options, infer_options);
  return ::pluss::ai::lymph::classification_summary(y, predicted, options);
}

// infer_scores() isn't documented as uniformly [0,1]-valued across model
// families: EnsembleClassifier's GradientBoost/AdaBoost branches return a raw,
// unbounded logit-scale margin (see family_classifier.cpp), and every other
// call site that needs a real probability from them (EnsembleClassifier::
// assess(), for one) applies `.sigmoid()` first -- not doing that here would
// have threshold_curve/best_threshold return a "best threshold" that's
// literally outside the [0,1] range every classifier family's InferOptions::
// positive_threshold requires, which just throws the next time anyone uses
// it. Scores already inside [0,1] pass through untouched (the common case);
// anything that escapes that range gets sigmoid-mapped into [0,1] first,
// matching the same escape hatch other framework call sites already use.
[[nodiscard]] inline Series as_probability_like_(const Series& scores) {
  const Series s = scores.to(ScalarType::Float64);
  if (s.numel() == 0) return s;
  const bool out_of_range = s.lt(0.0).any().item<bool>() || s.gt(1.0).any().item<bool>();
  return out_of_range ? s.sigmoid() : s;
}

// One sort + two cumulative sums over the model's scores computes
// precision/recall/F1 at every achievable threshold in one pass -- see
// nodej::threshold_curve (NodeJ/metrics/threshold_curve.cpp) for the
// underlying O(n log n) sweep this replaces a naive re-score-per-candidate
// loop with.
[[nodiscard]] inline ThresholdCurve threshold_curve(const ModelCore& model,
                                                    const LabeledBatch& batch,
                                                    const InferOptions& infer_options = {}) {
  const Series scores = as_probability_like_(model.infer_scores(batch.features, infer_options));
  return resolve(::pluss::ai::nodej::threshold_curve(scores, batch.y), "threshold_curve");
}

[[nodiscard]] inline ThresholdCurve threshold_curve(const ModelCore& model,
                                                    const pluss::table::DataFrame& x,
                                                    const Series& y,
                                                    const InferOptions& infer_options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "threshold_curve(model,x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "threshold_curve(model,x,y)");
  return threshold_curve(model, batch, infer_options);
}

[[nodiscard]] inline ThresholdCurve threshold_curve(const ModelCore& model,
                                                    const pluss::table::DataFrame& x,
                                                    const pluss::table::Thread& y,
                                                    const InferOptions& infer_options = {}) {
  return threshold_curve(model, x, as_stratify_series(y), infer_options);
}

// (frame, DataFrameBatchOptions) sibling -- see confusion_table's twin above
// for why this exists alongside the (x, y) form.
[[nodiscard]] inline ThresholdCurve threshold_curve(const ModelCore& model,
                                                    const pluss::table::DataFrame& frame,
                                                    const DataFrameBatchOptions& data_options,
                                                    const InferOptions& infer_options = {}) {
  if (!data_options.label_feature.has_value()) {
    ::pluss::throw_error("threshold_curve(model,frame,data_options): data_options.label_feature is required.");
  }
  const Series y = frame.features(*data_options.label_feature).as_series().reshape({frame.ncases()});
  const Series scores = as_probability_like_(model.infer_scores(frame, data_options, infer_options));
  return resolve(::pluss::ai::nodej::threshold_curve(scores, y), "threshold_curve");
}

// The threshold maximizing `objective` (Precision, Recall/Sensitivity, or F1
// -- anything else falls back to F1) over threshold_curve's full sweep.
[[nodiscard]] inline double best_threshold(const ModelCore& model,
                                           const LabeledBatch& batch,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  const Series scores = as_probability_like_(model.infer_scores(batch.features, infer_options));
  return resolve(::pluss::ai::nodej::best_threshold(scores, batch.y, objective), "best_threshold");
}

[[nodiscard]] inline double best_threshold(const ModelCore& model,
                                           const pluss::table::DataFrame& x,
                                           const Series& y,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  const auto names = cross_validate_default_feature_names_(x, "best_threshold(model,x,y)");
  const LabeledBatch batch = cross_validate_labeled_from_xy_(x, y, names, "best_threshold(model,x,y)");
  return best_threshold(model, batch, objective, infer_options);
}

[[nodiscard]] inline double best_threshold(const ModelCore& model,
                                           const pluss::table::DataFrame& x,
                                           const pluss::table::Thread& y,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  return best_threshold(model, x, as_stratify_series(y), objective, infer_options);
}

// (frame, DataFrameBatchOptions) sibling -- see confusion_table's twin above
// for why this exists alongside the (x, y) form.
[[nodiscard]] inline double best_threshold(const ModelCore& model,
                                           const pluss::table::DataFrame& frame,
                                           const DataFrameBatchOptions& data_options,
                                           MetricKind objective = MetricKind::F1,
                                           const InferOptions& infer_options = {}) {
  if (!data_options.label_feature.has_value()) {
    ::pluss::throw_error("best_threshold(model,frame,data_options): data_options.label_feature is required.");
  }
  const Series y = frame.features(*data_options.label_feature).as_series().reshape({frame.ncases()});
  const Series scores = as_probability_like_(model.infer_scores(frame, data_options, infer_options));
  return resolve(::pluss::ai::nodej::best_threshold(scores, y, objective), "best_threshold");
}

} // namespace pluss::ai::lymph::detail
