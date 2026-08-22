#pragma once

#include "pluss/ai/lymph/detail/model_core.h"
#include "pluss/ai/lymph/factories.h"
#include "MSharp/IntrusivePtr.h"

// model.h -- Model (LYMPH_REDESIGN_PLAN.md phase 2.5 + 3 + 6).
//
// Phase 2.5: `Model` becomes the public Lymph handle instead of a plain
// detail::ModelCore alias. It follows the same cheap-copy shape as table
// DataFrame/Thread: an intrusive ref-counted implementation pointer and
// copy-on-write detachment before mutating operations. Copying an untrained
// or trained Model is therefore cheap and ergonomic at the call site; the
// first mutator on a shared handle pays the explicit detail::ModelCore::clone()
// cost, preserving the old "do not accidentally mutate somebody else's
// model" semantics without forcing std::move into normal user code.
//
// Public factories live under `lymph::model`, grouped by model family:
// `model::linear::linear_regression(...)`, `model::tree::...`, etc. The
// family-root shortcuts (`model::linear_regression(...)`) and older plural
// compatibility shortcuts (`models::linear_regression(...)`) forward to those
// grouped factories. `lymph::Model` remains the ergonomic type shortcut;
// `lymph::model::Model` is the explicit family-local spelling of the same
// handle type.

namespace pluss::ai::lymph {

namespace model {
class Model;
class ModelPrep;
} // namespace model

using Model = model::Model;
using ModelPrep = model::ModelPrep;

namespace reports {
class ModelReports;
} // namespace reports

namespace detail {

struct ModelImpl final : pluss::msharp::RefCounted {
  explicit ModelImpl(ModelCore in_core) : core(std::move(in_core)) {}

  ModelCore core;
};

[[nodiscard]] inline const ModelCore& model_core(const Model& model) noexcept;

} // namespace detail

namespace model {

class Model final {
  friend const detail::ModelCore& detail::model_core(const Model& model) noexcept;

 public:
  explicit Model(const ModelSpec& spec)
      : impl_(pluss::msharp::make_intrusive<detail::ModelImpl>(detail::ModelCore(spec))) {}

  explicit Model(::pluss::ai::nodej::Workflow workflow)
      : impl_(pluss::msharp::make_intrusive<detail::ModelImpl>(detail::ModelCore(std::move(workflow)))) {}

  Model(detail::ModelCore core)
      : impl_(pluss::msharp::make_intrusive<detail::ModelImpl>(std::move(core))) {}

  Model(const Model&) noexcept = default;
  Model& operator=(const Model&) noexcept = default;
  Model(Model&&) noexcept = default;
  Model& operator=(Model&&) noexcept = default;

  [[nodiscard]] bool trained() const noexcept { return core_().trained(); }
  [[nodiscard]] bool has_model() const noexcept { return core_().has_model(); }
  [[nodiscard]] bool has_model_spec() const noexcept { return core_().has_model_spec(); }
  [[nodiscard]] bool has_opaque_transforms() const noexcept { return core_().has_opaque_transforms(); }
  [[nodiscard]] std::int64_t stage_count() const noexcept { return core_().stage_count(); }
  [[nodiscard]] TaskKind task_kind() const noexcept { return core_().task_kind(); }
  [[nodiscard]] const std::optional<ModelSpec>& model_spec() const noexcept { return core_().model_spec(); }
  [[nodiscard]] const std::vector<TransformSpec>& transform_specs() const noexcept { return core_().transform_specs(); }
  [[nodiscard]] FlowGraphInfo graph_info() const { return core_().graph_info(); }
  [[nodiscard]] std::vector<std::string> nested_param_keys() const { return core_().nested_param_keys(); }
  [[nodiscard]] double nested_param(std::string_view path) const { return core_().nested_param(path); }
  [[nodiscard]] std::string flow_summary() const { return core_().flow_summary(); }
  [[nodiscard]] std::string save_blueprint() const { return core_().save_blueprint(); }

  Model& set_nested_param_(std::string_view path, double value) {
    mut_core_().set_nested_param_(path, value);
    return *this;
  }

  Model& load_blueprint_(std::string_view text) {
    mut_core_().load_blueprint_(text);
    return *this;
  }

  Model& set_stage_key_(std::int64_t stage_index, std::string stage_key) {
    mut_core_().set_stage_key_(stage_index, std::move(stage_key));
    return *this;
  }

  Model& set_stage_route_(std::int64_t stage_index, pluss::msharp::RangeView::Int feature_route) {
    mut_core_().set_stage_route_(stage_index, feature_route);
    return *this;
  }

  Model& set_stage_remainder_(std::int64_t stage_index, RouteRemainderPolicy remainder) {
    mut_core_().set_stage_remainder_(stage_index, remainder);
    return *this;
  }

  Model& standard_scaler(prep::StandardScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().standard_scaler(options, feature_route);
    return *this;
  }

  Model& minmax_scaler(prep::MinMaxScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().minmax_scaler(options, feature_route);
    return *this;
  }

  Model& robust_scaler(prep::RobustScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().robust_scaler(options, feature_route);
    return *this;
  }

  Model& simple_imputer(prep::SimpleImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().simple_imputer(options, feature_route);
    return *this;
  }

  Model& neighbor_imputer(prep::NeighborImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().neighbor_imputer(options, feature_route);
    return *this;
  }

  Model& iterative_imputer(prep::IterativeImputerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().iterative_imputer(options, feature_route);
    return *this;
  }

  Model& one_hot_encoder(prep::OneHotEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().one_hot_encoder(options, feature_route);
    return *this;
  }

  Model& frequency_encoder(prep::FrequencyEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().frequency_encoder(options, feature_route);
    return *this;
  }

  Model& ordinal_encoder(prep::OrdinalEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().ordinal_encoder(options, feature_route);
    return *this;
  }

  Model& label_encoder(prep::LabelEncoderOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().label_encoder(options, feature_route);
    return *this;
  }

  Model& vector_normalizer(prep::VectorNormalizerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().vector_normalizer(options, feature_route);
    return *this;
  }

  Model& maxabs_scaler(prep::MaxAbsScalerOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().maxabs_scaler(options, feature_route);
    return *this;
  }

  Model& quantile_map(prep::QuantileMapOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().quantile_map(options, feature_route);
    return *this;
  }

  Model& power_map(prep::PowerMapOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().power_map(options, feature_route);
    return *this;
  }

  Model& threshold_binarizer(prep::ThresholdBinarizerOptions options = {},
                             pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().threshold_binarizer(options, feature_route);
    return *this;
  }

  Model& polynomial_lift(prep::PolynomialLiftOptions options = {}, pluss::msharp::RangeView::Int feature_route = {}) {
    mut_core_().polynomial_lift(options, feature_route);
    return *this;
  }

  Model& function_transformer(prep::FunctionTransformer transformer) {
    mut_core_().function_transformer(std::move(transformer));
    return *this;
  }

  Model& feature_transformer(prep::FeatureTransformer transformer) {
    mut_core_().feature_transformer(std::move(transformer));
    return *this;
  }

  Model& add(const CustomStage& stage) {
    if (!stage.map_fn) {
      ::pluss::throw_error("lymph::Model::add: custom stage \"" + stage.name + "\" has no callable.");
    }
    mut_core_().function_transformer(prep::FunctionTransformer(stage.map_fn, stage.options));
    return *this;
  }

  Model& append(Stage stage, ColumnRouter router) {
    mut_core_().append(std::move(stage), std::move(router));
    return *this;
  }

  [[nodiscard]] ModelPrep prepare() noexcept;

  Model& learn_(const LabeledBatch& batch, const TrainOptions& options = {}) {
    mut_core_().learn_(batch, options);
    return *this;
  }

  Model& learn_(const pluss::table::DataFrame& frame,
                 const DataFrameBatchOptions& data_options,
                 const TrainOptions& options = {}) {
    mut_core_().learn_(frame, data_options, options);
    return *this;
  }

  Model& learn_(const pluss::table::DataFrame& x,
                 const Series& y,
                 const TrainOptions& options = {}) {
    mut_core_().learn_(x, y, options);
    return *this;
  }

  Model& learn_(const pluss::table::DataFrame& x,
                 const pluss::table::Thread& y,
                 const TrainOptions& options = {}) {
    mut_core_().learn_(x, y, options);
    return *this;
  }

  [[nodiscard]] Model learn(const LabeledBatch& batch, const TrainOptions& options = {}) const {
    Model out(*this);
    out.learn_(batch, options);
    return out;
  }

  [[nodiscard]] Model learn(const pluss::table::DataFrame& frame,
                            const DataFrameBatchOptions& data_options,
                            const TrainOptions& options = {}) const {
    Model out(*this);
    out.learn_(frame, data_options, options);
    return out;
  }

  [[nodiscard]] Model learn(const pluss::table::DataFrame& x,
                            const Series& y,
                            const TrainOptions& options = {}) const {
    Model out(*this);
    out.learn_(x, y, options);
    return out;
  }

  [[nodiscard]] Model learn(const pluss::table::DataFrame& x,
                            const pluss::table::Thread& y,
                            const TrainOptions& options = {}) const {
    Model out(*this);
    out.learn_(x, y, options);
    return out;
  }

  Model& target_transform(TargetTransform transform) {
    mut_core_().target_transform(std::move(transform));
    return *this;
  }

  [[nodiscard]] bool has_target_transform() const noexcept { return core_().has_target_transform(); }
  [[nodiscard]] const TargetTransform& target_transform() const { return core_().target_transform(); }

  [[nodiscard]] Series map(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return core_().map(batch, options);
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& frame,
                           const DataFrameBatchOptions& data_options,
                           const InferOptions& options = {}) const {
    return core_().map(frame, data_options, options);
  }

  [[nodiscard]] Series map(const pluss::table::DataFrame& x, const InferOptions& options = {}) const {
    return core_().map(x, options);
  }

  [[nodiscard]] Series learn_map(const FeatureBatch& batch,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    return mut_core_().learn_map(batch, learn_options, infer_options);
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& frame,
                                 const DataFrameBatchOptions& data_options,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    return mut_core_().learn_map(frame, data_options, learn_options, infer_options);
  }

  [[nodiscard]] Series learn_map(const pluss::table::DataFrame& x,
                                 const TrainOptions& learn_options = {},
                                 const InferOptions& infer_options = {}) {
    return mut_core_().learn_map(x, learn_options, infer_options);
  }

  [[nodiscard]] Series infer(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return core_().infer(batch, options);
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& frame,
                             const DataFrameBatchOptions& data_options,
                             const InferOptions& options = {}) const {
    return core_().infer(frame, data_options, options);
  }

  [[nodiscard]] Series infer(const pluss::table::DataFrame& x, const InferOptions& options = {}) const {
    return core_().infer(x, options);
  }

  [[nodiscard]] Series infer_scores(const FeatureBatch& batch, const InferOptions& options = {}) const {
    return core_().infer_scores(batch, options);
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const InferOptions& options = {}) const {
    return core_().infer_scores(frame, data_options, options);
  }

  [[nodiscard]] Series infer_scores(const pluss::table::DataFrame& x, const InferOptions& options = {}) const {
    return core_().infer_scores(x, options);
  }

  [[nodiscard]] Series assess_series(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return core_().assess_series(batch, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& frame,
                                     const DataFrameBatchOptions& data_options,
                                     const AssessOptions& options = {}) const {
    return core_().assess_series(frame, data_options, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const Series& y,
                                     const AssessOptions& options = {}) const {
    return core_().assess_series(x, y, options);
  }

  [[nodiscard]] Series assess_series(const pluss::table::DataFrame& x,
                                     const pluss::table::Thread& y,
                                     const AssessOptions& options = {}) const {
    return core_().assess_series(x, y, options);
  }

  [[nodiscard]] MetricReport assess(const LabeledBatch& batch, const AssessOptions& options = {}) const {
    return core_().assess(batch, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& frame,
                                    const DataFrameBatchOptions& data_options,
                                    const AssessOptions& options = {}) const {
    return core_().assess(frame, data_options, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const Series& y,
                                    const AssessOptions& options = {}) const {
    return core_().assess(x, y, options);
  }

  [[nodiscard]] MetricReport assess(const pluss::table::DataFrame& x,
                                    const pluss::table::Thread& y,
                                    const AssessOptions& options = {}) const {
    return core_().assess(x, y, options);
  }

  [[nodiscard]] ::pluss::ai::lymph::reports::ModelReports reports() const;

  [[nodiscard]] PermuteImpactReport permutation_impact(const LabeledBatch& train,
                                                        const LabeledBatch& eval,
                                                        MetricKind metric = MetricKind::Accuracy,
                                                        std::int64_t repeats = 5,
                                                        RandomState random = {}) const {
    return core_().permutation_impact(train, eval, metric, repeats, random);
  }

  [[nodiscard]] DependenceCurve dependence_curve(const LabeledBatch& train,
                                                 const FeatureBatch& eval,
                                                 std::int64_t feature_index,
                                                 std::int64_t grid_points = 32) const {
    return core_().dependence_curve(train, eval, feature_index, grid_points);
  }

  [[nodiscard]] DependenceSurface dependence_surface(const LabeledBatch& train,
                                                     const FeatureBatch& eval,
                                                     std::int64_t feature_a,
                                                     std::int64_t feature_b,
                                                     std::int64_t grid_points_a = 16,
                                                     std::int64_t grid_points_b = 16) const {
    return core_().dependence_surface(train, eval, feature_a, feature_b, grid_points_a, grid_points_b);
  }

  [[nodiscard]] LinearParameters linear_parameters() const { return core_().linear_parameters(); }
  [[nodiscard]] std::vector<double> coefficients() const { return core_().coefficients(); }
  [[nodiscard]] double intercept_term() const { return core_().intercept_term(); }
  [[nodiscard]] const Series& coeff() const noexcept { return core_().coeff; }
  [[nodiscard]] const Series& intercept() const noexcept { return core_().intercept; }

  void save(const std::string& path,
            const LabeledBatch& train_batch,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    core_().save(path, train_batch, train_options, std::move(metadata));
  }

  void save(const std::string& path) const {
    core_().save(path);
  }

  void save(const std::string& path,
            io::FlowBundleMetadata metadata) const {
    core_().save(path, std::move(metadata));
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const Series& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    core_().save(path, x_train, y_train, train_options, std::move(metadata));
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& x_train,
            const pluss::table::Thread& y_train,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    core_().save(path, x_train, y_train, train_options, std::move(metadata));
  }

  void save(const std::string& path,
            const pluss::table::DataFrame& frame,
            const DataFrameBatchOptions& data_options,
            const TrainOptions& train_options = {},
            io::FlowBundleMetadata metadata = {}) const {
    core_().save(path, frame, data_options, train_options, std::move(metadata));
  }

  [[nodiscard]] static Model restore(const io::FlowBundle& bundle) {
    return Model(detail::ModelCore::restore(bundle));
  }

  [[nodiscard]] static Model load(const std::string& path) {
    return Model(detail::ModelCore::load(path));
  }

  [[nodiscard]] std::vector<std::vector<Series>> dump_transform_states() const {
    return core_().dump_transform_states();
  }

  [[nodiscard]] std::vector<Series> dump_model_state() const {
    return core_().dump_model_state();
  }

  Model& load_transform_states_(pluss::msharp::RangeView::Of<std::vector<Series>> states) {
    mut_core_().load_transform_states_(states);
    return *this;
  }

  Model& load_model_state_(pluss::msharp::RangeView::Series state) {
    mut_core_().load_model_state_(state);
    return *this;
  }

  Model& mark_trained_(bool trained = true) {
    mut_core_().mark_trained_(trained);
    return *this;
  }

  [[nodiscard]] const ::pluss::ai::nodej::Workflow& workflow() const noexcept { return core_().workflow(); }
  [[nodiscard]] ::pluss::ai::nodej::Workflow& workflow() noexcept { return mut_core_().workflow(); }

  [[nodiscard]] Model clone() const {
    return Model(core_().clone());
  }

 private:
  [[nodiscard]] const detail::ModelCore& core_() const noexcept { return impl_->core; }

  [[nodiscard]] detail::ModelCore& mut_core_() {
    detach_();
    return impl_->core;
  }

  void detach_() {
    if (!impl_) {
      ::pluss::throw_error("lymph::Model: invalid empty model handle.");
    }
    if (impl_.unique()) return;
    impl_ = pluss::msharp::make_intrusive<detail::ModelImpl>(impl_->core.clone());
  }

  pluss::msharp::intrusive_ptr<detail::ModelImpl> impl_{};
};

class ModelPrep final {
 public:
  explicit ModelPrep(Model& model) noexcept : model_(&model) {}

  [[nodiscard]] Model& model() const {
    if (!model_) {
      ::pluss::throw_error("ModelPrep: invalid empty preparation accessor.");
    }
    return *model_;
  }

  [[nodiscard]] ColumnRouter names(std::vector<std::string> values) const {
    return columnrouter::names(std::move(values));
  }

  [[nodiscard]] ColumnRouter names(std::initializer_list<std::string> values) const {
    return names(std::vector<std::string>(values.begin(), values.end()));
  }

  [[nodiscard]] ColumnRouter where(
      std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate) const {
    return columnrouter::where(std::move(predicate));
  }

  [[nodiscard]] ColumnRouter remaining(std::vector<std::string> values) const {
    return columnrouter::remaining_of(std::move(values));
  }

  [[nodiscard]] ColumnRouter remaining(std::initializer_list<std::string> values) const {
    return remaining(std::vector<std::string>(values.begin(), values.end()));
  }

  [[nodiscard]] ColumnRouter remaining(ColumnRouter router) const {
    return columnrouter::remaining_of(std::move(router));
  }

  ModelPrep& add(Stage stage, ColumnRouter router) {
    model().append(std::move(stage), std::move(router));
    return *this;
  }

  ModelPrep& standardize(ColumnRouter router, prep::StandardScalerOptions options = {}) {
    return add(stage::standard_scaler(options), std::move(router));
  }

  ModelPrep& standardize(std::initializer_list<std::string> names, prep::StandardScalerOptions options = {}) {
    return standardize(this->names(names), options);
  }

  ModelPrep& scale(ColumnRouter router, prep::StandardScalerOptions options = {}) {
    return standardize(std::move(router), options);
  }

  ModelPrep& scale(std::initializer_list<std::string> names, prep::StandardScalerOptions options = {}) {
    return scale(this->names(names), options);
  }

  ModelPrep& rescale(ColumnRouter router, prep::MinMaxScalerOptions options = {}) {
    return add(stage::minmax_scaler(options), std::move(router));
  }

  ModelPrep& rescale(std::initializer_list<std::string> names, prep::MinMaxScalerOptions options = {}) {
    return rescale(this->names(names), options);
  }

  ModelPrep& robust(ColumnRouter router, prep::RobustScalerOptions options = {}) {
    return add(stage::robust_scaler(options), std::move(router));
  }

  ModelPrep& fill(ColumnRouter router, prep::SimpleImputerOptions options = {}) {
    return add(stage::simple_imputer(options), std::move(router));
  }

  ModelPrep& fill(std::initializer_list<std::string> names, prep::SimpleImputerOptions options = {}) {
    return fill(this->names(names), options);
  }

  ModelPrep& neighbor_fill(ColumnRouter router, prep::NeighborImputerOptions options = {}) {
    return add(stage::neighbor_imputer(options), std::move(router));
  }

  ModelPrep& iterative_fill(ColumnRouter router, prep::IterativeImputerOptions options = {}) {
    return add(stage::iterative_imputer(options), std::move(router));
  }

  ModelPrep& encode(ColumnRouter router, prep::OneHotEncoderOptions options = {}) {
    return add(stage::one_hot_encoder(options), std::move(router));
  }

  ModelPrep& encode(std::initializer_list<std::string> names, prep::OneHotEncoderOptions options = {}) {
    return encode(this->names(names), options);
  }

  ModelPrep& count_encode(ColumnRouter router, prep::FrequencyEncoderOptions options = {}) {
    return add(stage::frequency_encoder(options), std::move(router));
  }

  ModelPrep& order_encode(ColumnRouter router, prep::OrdinalEncoderOptions options = {}) {
    return add(stage::ordinal_encoder(options), std::move(router));
  }

  ModelPrep& label_encode(ColumnRouter router, prep::LabelEncoderOptions options = {}) {
    return add(stage::label_encoder(options), std::move(router));
  }

  ModelPrep& normalize(ColumnRouter router, prep::VectorNormalizerOptions options = {}) {
    return add(stage::vector_normalizer(options), std::move(router));
  }

  ModelPrep& maxabs(ColumnRouter router, prep::MaxAbsScalerOptions options = {}) {
    return add(stage::maxabs_scaler(options), std::move(router));
  }

  ModelPrep& gaussianize(ColumnRouter router, prep::QuantileMapOptions options = {}) {
    options.output = prep::QuantileMapOutput::Gaussian;
    return add(stage::quantile_map(options), std::move(router));
  }

  ModelPrep& gaussianize(std::initializer_list<std::string> names, prep::QuantileMapOptions options = {}) {
    return gaussianize(this->names(names), options);
  }

  ModelPrep& quantiles(ColumnRouter router, prep::QuantileMapOptions options = {}) {
    return add(stage::quantile_map(options), std::move(router));
  }

  ModelPrep& power(ColumnRouter router, prep::PowerMapOptions options = {}) {
    return add(stage::power_map(options), std::move(router));
  }

  ModelPrep& binarize(ColumnRouter router, prep::ThresholdBinarizerOptions options = {}) {
    return add(stage::threshold_binarizer(options), std::move(router));
  }

  ModelPrep& polynomial(ColumnRouter router, prep::PolynomialLiftOptions options = {}) {
    return add(stage::polynomial_lift(options), std::move(router));
  }

 private:
  Model* model_ = nullptr;
};

[[nodiscard]] inline ModelPrep Model::prepare() noexcept {
  return ModelPrep(*this);
}

} // namespace model

namespace detail {

[[nodiscard]] inline const ModelCore& model_core(const Model& model) noexcept {
  return model.core_();
}

} // namespace detail

namespace model {

namespace linear {
[[nodiscard]] inline Model linear_regression(LinearFamilyOptions options = {}) { return detail::linear::linear_regression(options); }
[[nodiscard]] inline Model logistic_regression(LinearFamilyOptions options = {}) { return detail::linear::logistic_regression(options); }
[[nodiscard]] inline Model hinge_classifier(LinearFamilyOptions options = {}) { return detail::linear::hinge_classifier(options); }
} // namespace linear

namespace tree {
[[nodiscard]] inline Model decision_tree_regressor(TreeFamilyOptions options = {}) { return detail::tree::decision_tree_regressor(options); }
[[nodiscard]] inline Model decision_tree_classifier(TreeFamilyOptions options = {}) { return detail::tree::decision_tree_classifier(options); }
} // namespace tree

namespace ensemble {
[[nodiscard]] inline Model bagging_regressor(EnsembleFamilyOptions options = {}) { return detail::ensemble::bagging_regressor(options); }
[[nodiscard]] inline Model random_forest_regressor(EnsembleFamilyOptions options = {}) { return detail::ensemble::random_forest_regressor(options); }
[[nodiscard]] inline Model bagging_classifier(EnsembleFamilyOptions options = {}) { return detail::ensemble::bagging_classifier(options); }
[[nodiscard]] inline Model random_forest_classifier(EnsembleFamilyOptions options = {}) { return detail::ensemble::random_forest_classifier(options); }
[[nodiscard]] inline Model gradient_boost_regressor(EnsembleFamilyOptions options = {}) { return detail::ensemble::gradient_boost_regressor(options); }
[[nodiscard]] inline Model gradient_boost_classifier(EnsembleFamilyOptions options = {}) { return detail::ensemble::gradient_boost_classifier(options); }
[[nodiscard]] inline Model adaboost_classifier(EnsembleFamilyOptions options = {}) {
  options.method = EnsembleMethod::AdaBoost;
  return detail::ensemble::classifier(options);
}
[[nodiscard]] inline Model regressor(EnsembleFamilyOptions options = {}) { return detail::ensemble::regressor(options); }
[[nodiscard]] inline Model classifier(EnsembleFamilyOptions options = {}) { return detail::ensemble::classifier(options); }
} // namespace ensemble

namespace neighbor {
[[nodiscard]] inline Model knn_regressor(NeighborFamilyOptions options = {}) { return detail::neighbor::knn_regressor(options); }
[[nodiscard]] inline Model knn_classifier(NeighborFamilyOptions options = {}) { return detail::neighbor::knn_classifier(options); }
[[nodiscard]] inline Model radius_neighbors_regressor(NeighborFamilyOptions options = {}) { return detail::neighbor::radius_neighbors_regressor(options); }
[[nodiscard]] inline Model radius_neighbors_classifier(NeighborFamilyOptions options = {}) { return detail::neighbor::radius_neighbors_classifier(options); }
[[nodiscard]] inline Model regressor(NeighborFamilyOptions options = {}) { return detail::neighbor::regressor(options); }
[[nodiscard]] inline Model classifier(NeighborFamilyOptions options = {}) { return detail::neighbor::classifier(options); }
} // namespace neighbor

namespace kernel {
[[nodiscard]] inline Model svm_regressor(KernelFamilyOptions options = {}) { return detail::kernel::svm_regressor(options); }
[[nodiscard]] inline Model svm_classifier(KernelFamilyOptions options = {}) { return detail::kernel::svm_classifier(options); }
[[nodiscard]] inline Model nu_svm_regressor(KernelFamilyOptions options = {}) { return detail::kernel::nu_svm_regressor(options); }
[[nodiscard]] inline Model nu_svm_classifier(KernelFamilyOptions options = {}) { return detail::kernel::nu_svm_classifier(options); }
[[nodiscard]] inline Model one_class_svm(KernelFamilyOptions options = {}) { return detail::kernel::one_class_svm(options); }
[[nodiscard]] inline Model linear_kernel_regressor(KernelFamilyOptions options = {}) { return detail::kernel::linear_kernel_regressor(options); }
[[nodiscard]] inline Model rbf_kernel_regressor(KernelFamilyOptions options = {}) { return detail::kernel::rbf_kernel_regressor(options); }
[[nodiscard]] inline Model polynomial_kernel_regressor(KernelFamilyOptions options = {}) { return detail::kernel::polynomial_kernel_regressor(options); }
[[nodiscard]] inline Model sigmoid_kernel_regressor(KernelFamilyOptions options = {}) { return detail::kernel::sigmoid_kernel_regressor(options); }
[[nodiscard]] inline Model linear_kernel_classifier(KernelFamilyOptions options = {}) { return detail::kernel::linear_kernel_classifier(options); }
[[nodiscard]] inline Model rbf_kernel_classifier(KernelFamilyOptions options = {}) { return detail::kernel::rbf_kernel_classifier(options); }
[[nodiscard]] inline Model polynomial_kernel_classifier(KernelFamilyOptions options = {}) { return detail::kernel::polynomial_kernel_classifier(options); }
[[nodiscard]] inline Model sigmoid_kernel_classifier(KernelFamilyOptions options = {}) { return detail::kernel::sigmoid_kernel_classifier(options); }
[[nodiscard]] inline Model regressor(KernelFamilyOptions options = {}) { return detail::kernel::regressor(options); }
[[nodiscard]] inline Model classifier(KernelFamilyOptions options = {}) { return detail::kernel::classifier(options); }
[[nodiscard]] inline Model transformer(KernelFamilyOptions options = {}) { return detail::kernel::transformer(options); }
} // namespace kernel

namespace bayes {
[[nodiscard]] inline Model gaussian_nb_classifier(BayesFamilyOptions options = {}) { return detail::bayes::gaussian_nb_classifier(options); }
[[nodiscard]] inline Model gaussian_nb_regressor(BayesFamilyOptions options = {}) { return detail::bayes::gaussian_nb_regressor(options); }
[[nodiscard]] inline Model bernoulli_nb_classifier(BayesFamilyOptions options = {}) { return detail::bayes::bernoulli_nb_classifier(options); }
[[nodiscard]] inline Model bernoulli_nb_regressor(BayesFamilyOptions options = {}) { return detail::bayes::bernoulli_nb_regressor(options); }
[[nodiscard]] inline Model multinomial_nb_classifier(BayesFamilyOptions options = {}) { return detail::bayes::multinomial_nb_classifier(options); }
[[nodiscard]] inline Model complement_nb_classifier(BayesFamilyOptions options = {}) { return detail::bayes::complement_nb_classifier(options); }
[[nodiscard]] inline Model multinomial_nb_regressor(BayesFamilyOptions options = {}) { return detail::bayes::multinomial_nb_regressor(options); }
[[nodiscard]] inline Model regressor(BayesFamilyOptions options = {}) { return detail::bayes::regressor(options); }
[[nodiscard]] inline Model classifier(BayesFamilyOptions options = {}) { return detail::bayes::classifier(options); }
} // namespace bayes

namespace cluster {
[[nodiscard]] inline Model kmeans(ClusterFamilyOptions options = {}) { return detail::cluster::kmeans(options); }
[[nodiscard]] inline Model transformer(ClusterFamilyOptions options = {}) { return detail::cluster::transformer(options); }
} // namespace cluster

namespace decompose {
// PCA (and friends) as a standalone model in its own right -- distinct from
// the (not yet built) lymph::stage::pca(...) planned for attaching PCA as
// one step inside a larger pipeline (LYMPH_REDESIGN_PLAN.md 7.4).
[[nodiscard]] inline Model pca(DecomposeFamilyOptions options = {}) { return detail::decompose::pca(options); }
[[nodiscard]] inline Model transformer(DecomposeFamilyOptions options = {}) { return detail::decompose::transformer(options); }
} // namespace decompose

namespace detect {
[[nodiscard]] inline Model isolation_detector(DetectFamilyOptions options = {}) { return detail::detect::isolation_detector(options); }
[[nodiscard]] inline Model isolation_detector(prep::OutlierIsolatorOptions options) { return detail::detect::isolation_detector(options); }
[[nodiscard]] inline Model transformer(DetectFamilyOptions options = {}) { return detail::detect::transformer(options); }
} // namespace detect

[[nodiscard]] inline Model linear_regression(LinearFamilyOptions options = {}) { return linear::linear_regression(options); }
[[nodiscard]] inline Model logistic_regression(LinearFamilyOptions options = {}) { return linear::logistic_regression(options); }
[[nodiscard]] inline Model hinge_classifier(LinearFamilyOptions options = {}) { return linear::hinge_classifier(options); }

[[nodiscard]] inline Model decision_tree_regressor(TreeFamilyOptions options = {}) { return tree::decision_tree_regressor(options); }
[[nodiscard]] inline Model decision_tree_classifier(TreeFamilyOptions options = {}) { return tree::decision_tree_classifier(options); }

[[nodiscard]] inline Model bagging_regressor(EnsembleFamilyOptions options = {}) { return ensemble::bagging_regressor(options); }
[[nodiscard]] inline Model random_forest_regressor(EnsembleFamilyOptions options = {}) { return ensemble::random_forest_regressor(options); }
[[nodiscard]] inline Model bagging_classifier(EnsembleFamilyOptions options = {}) { return ensemble::bagging_classifier(options); }
[[nodiscard]] inline Model random_forest_classifier(EnsembleFamilyOptions options = {}) { return ensemble::random_forest_classifier(options); }
[[nodiscard]] inline Model gradient_boost_regressor(EnsembleFamilyOptions options = {}) { return ensemble::gradient_boost_regressor(options); }
[[nodiscard]] inline Model gradient_boost_classifier(EnsembleFamilyOptions options = {}) { return ensemble::gradient_boost_classifier(options); }
[[nodiscard]] inline Model adaboost_classifier(EnsembleFamilyOptions options = {}) { return ensemble::adaboost_classifier(options); }
[[nodiscard]] inline Model regressor(EnsembleFamilyOptions options = {}) { return ensemble::regressor(options); }
[[nodiscard]] inline Model classifier(EnsembleFamilyOptions options = {}) { return ensemble::classifier(options); }

[[nodiscard]] inline Model knn_regressor(NeighborFamilyOptions options = {}) { return neighbor::knn_regressor(options); }
[[nodiscard]] inline Model knn_classifier(NeighborFamilyOptions options = {}) { return neighbor::knn_classifier(options); }
[[nodiscard]] inline Model radius_neighbors_regressor(NeighborFamilyOptions options = {}) { return neighbor::radius_neighbors_regressor(options); }
[[nodiscard]] inline Model radius_neighbors_classifier(NeighborFamilyOptions options = {}) { return neighbor::radius_neighbors_classifier(options); }
[[nodiscard]] inline Model regressor(NeighborFamilyOptions options = {}) { return neighbor::regressor(options); }
[[nodiscard]] inline Model classifier(NeighborFamilyOptions options = {}) { return neighbor::classifier(options); }

[[nodiscard]] inline Model svm_regressor(KernelFamilyOptions options = {}) { return kernel::svm_regressor(options); }
[[nodiscard]] inline Model svm_classifier(KernelFamilyOptions options = {}) { return kernel::svm_classifier(options); }
[[nodiscard]] inline Model nu_svm_regressor(KernelFamilyOptions options = {}) { return kernel::nu_svm_regressor(options); }
[[nodiscard]] inline Model nu_svm_classifier(KernelFamilyOptions options = {}) { return kernel::nu_svm_classifier(options); }
[[nodiscard]] inline Model one_class_svm(KernelFamilyOptions options = {}) { return kernel::one_class_svm(options); }
[[nodiscard]] inline Model linear_kernel_regressor(KernelFamilyOptions options = {}) { return kernel::linear_kernel_regressor(options); }
[[nodiscard]] inline Model rbf_kernel_regressor(KernelFamilyOptions options = {}) { return kernel::rbf_kernel_regressor(options); }
[[nodiscard]] inline Model polynomial_kernel_regressor(KernelFamilyOptions options = {}) { return kernel::polynomial_kernel_regressor(options); }
[[nodiscard]] inline Model sigmoid_kernel_regressor(KernelFamilyOptions options = {}) { return kernel::sigmoid_kernel_regressor(options); }
[[nodiscard]] inline Model linear_kernel_classifier(KernelFamilyOptions options = {}) { return kernel::linear_kernel_classifier(options); }
[[nodiscard]] inline Model rbf_kernel_classifier(KernelFamilyOptions options = {}) { return kernel::rbf_kernel_classifier(options); }
[[nodiscard]] inline Model polynomial_kernel_classifier(KernelFamilyOptions options = {}) { return kernel::polynomial_kernel_classifier(options); }
[[nodiscard]] inline Model sigmoid_kernel_classifier(KernelFamilyOptions options = {}) { return kernel::sigmoid_kernel_classifier(options); }
[[nodiscard]] inline Model regressor(KernelFamilyOptions options = {}) { return kernel::regressor(options); }
[[nodiscard]] inline Model classifier(KernelFamilyOptions options = {}) { return kernel::classifier(options); }
[[nodiscard]] inline Model transformer(KernelFamilyOptions options = {}) { return kernel::transformer(options); }

[[nodiscard]] inline Model gaussian_nb_classifier(BayesFamilyOptions options = {}) { return bayes::gaussian_nb_classifier(options); }
[[nodiscard]] inline Model gaussian_nb_regressor(BayesFamilyOptions options = {}) { return bayes::gaussian_nb_regressor(options); }
[[nodiscard]] inline Model bernoulli_nb_classifier(BayesFamilyOptions options = {}) { return bayes::bernoulli_nb_classifier(options); }
[[nodiscard]] inline Model bernoulli_nb_regressor(BayesFamilyOptions options = {}) { return bayes::bernoulli_nb_regressor(options); }
[[nodiscard]] inline Model multinomial_nb_classifier(BayesFamilyOptions options = {}) { return bayes::multinomial_nb_classifier(options); }
[[nodiscard]] inline Model complement_nb_classifier(BayesFamilyOptions options = {}) { return bayes::complement_nb_classifier(options); }
[[nodiscard]] inline Model multinomial_nb_regressor(BayesFamilyOptions options = {}) { return bayes::multinomial_nb_regressor(options); }
[[nodiscard]] inline Model regressor(BayesFamilyOptions options = {}) { return bayes::regressor(options); }
[[nodiscard]] inline Model classifier(BayesFamilyOptions options = {}) { return bayes::classifier(options); }

[[nodiscard]] inline Model kmeans(ClusterFamilyOptions options = {}) { return cluster::kmeans(options); }
[[nodiscard]] inline Model transformer(ClusterFamilyOptions options = {}) { return cluster::transformer(options); }

[[nodiscard]] inline Model pca(DecomposeFamilyOptions options = {}) { return decompose::pca(options); }
[[nodiscard]] inline Model transformer(DecomposeFamilyOptions options = {}) { return decompose::transformer(options); }

[[nodiscard]] inline Model isolation_detector(DetectFamilyOptions options = {}) { return detect::isolation_detector(options); }
[[nodiscard]] inline Model isolation_detector(prep::OutlierIsolatorOptions options) { return detect::isolation_detector(options); }
[[nodiscard]] inline Model transformer(DetectFamilyOptions options = {}) { return detect::transformer(options); }

} // namespace model

} // namespace pluss::ai::lymph
