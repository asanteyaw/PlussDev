#pragma once

#include "pluss/ai/nodej/builders.h"
#include "pluss/ai/nodej/compose.h"
#include "pluss/ai/nodej/io.h"
#include "pluss/ai/nodej/online.h"
#include "pluss/ai/nodej/tune.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace pluss::table {
class DataFrame;
}

namespace pluss::ai::lymph::quick {

template <class T>
using StatusOr = nodej::StatusOr<T>;

template <class T>
T must(StatusOr<T> value_or, const char* where = nullptr) {
  if (!value_or.ok()) {
    const std::string prefix = (where != nullptr && where[0] != '\0')
                                   ? (std::string(where) + ": ")
                                   : std::string{};
    throw std::runtime_error(prefix + value_or.status().message);
  }
  return value_or.move_value();
}

inline void must(nodej::Status status, const char* where = nullptr) {
  if (status.ok()) return;
  const std::string prefix = (where != nullptr && where[0] != '\0')
                                 ? (std::string(where) + ": ")
                                 : std::string{};
  throw std::runtime_error(prefix + status.message);
}

struct FrameFlowArtifact {
  nodej::Flow flow{};
  nodej::Series metrics = nodej::Series::undefined();
  nodej::LabeledBatch train_batch{};
};

StatusOr<std::unique_ptr<nodej::ClassifierBase>> learn_classifier(
    const nodej::ModelSpec& spec,
    const nodej::LabeledBatch& train,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> learn_regressor(
    const nodej::ModelSpec& spec,
    const nodej::LabeledBatch& train,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::TransformerBase>> learn_transformer(
    const nodej::ModelSpec& spec,
    const nodej::FeatureBatch& train,
    const nodej::TrainOptions& options = {});

StatusOr<nodej::Flow> learn_flow(
    nodej::Flow flow,
    const nodej::LabeledBatch& train,
    const nodej::TrainOptions& options = {});

StatusOr<nodej::Flow> learn_flow(
    nodej::Flow flow,
    const pluss::table::DataFrame& frame,
    const nodej::DataFrameBatchOptions& data_options,
    const nodej::TrainOptions& options = {});

StatusOr<FrameFlowArtifact> fit_evaluate_save(
    const nodej::ModelSpec& spec,
    const pluss::table::DataFrame& frame,
    const nodej::DataFrameBatchOptions& data_options,
    const nodej::TrainOptions& train_options = {},
    const nodej::AssessOptions& assess_options = {},
    const std::string& save_path = "",
    nodej::io::FlowBundleMetadata metadata = {});

StatusOr<nodej::Flow> linear_classifier_flow(
    const nodej::LabeledBatch& train,
    double regularization_strength = 1e-6,
    bool with_standard_scaler = true,
    bool with_mean_imputer = false,
    const nodej::TrainOptions& options = {});

StatusOr<nodej::Flow> ridge_regressor_flow(
    const nodej::LabeledBatch& train,
    double regularization_strength = 1e-6,
    bool with_standard_scaler = true,
    bool with_mean_imputer = false,
    const nodej::TrainOptions& options = {});

StatusOr<nodej::Flow> linear_regressor_flow(
    const nodej::LabeledBatch& train,
    const nodej::LinearFamilyOptions& family_options = {},
    bool with_standard_scaler = true,
    bool with_mean_imputer = false,
    const nodej::TrainOptions& options = {});

StatusOr<nodej::Flow> tree_regressor_flow(
    const nodej::LabeledBatch& train,
    const nodej::TreeFamilyOptions& family_options = {},
    bool with_mean_imputer = false,
    const nodej::TrainOptions& options = {});

StatusOr<nodej::online::OnlineLinearClassifier> online_linear_classifier(
    const nodej::LabeledBatch& warm_start,
    const nodej::online::OnlineLinearOptions& online_options = {},
    const nodej::TrainOptions& train_options = {});

StatusOr<nodej::online::OnlineLinearRegressor> online_linear_regressor(
    const nodej::LabeledBatch& warm_start,
    const nodej::online::OnlineLinearOptions& online_options = {},
    const nodej::TrainOptions& train_options = {});

StatusOr<nodej::CrossAssessReport> assess_cv(
    const nodej::ModelSpec& spec,
    const nodej::LabeledBatch& batch,
    const nodej::SplitOptions& split = {},
    const nodej::TrainOptions& train_options = {},
    const nodej::AssessOptions& assess_options = {});

StatusOr<nodej::TuneReport> tune_sweep(
    const std::vector<nodej::ModelSpec>& candidates,
    const nodej::LabeledBatch& batch,
    const nodej::SplitOptions& split = {},
    const nodej::TrainOptions& train_options = {},
    const nodej::AssessOptions& assess_options = {},
    nodej::MetricKind objective_metric = nodej::MetricKind::Accuracy,
    nodej::TuneObjectivePolicy objective_policy = nodej::TuneObjectivePolicy::Auto);

StatusOr<nodej::TuneReport> tune_probe(
    const nodej::ProbeSpecSampler& sampler,
    std::int64_t trials,
    const nodej::LabeledBatch& batch,
    const nodej::SplitOptions& split = {},
    const nodej::TrainOptions& train_options = {},
    const nodej::AssessOptions& assess_options = {},
    nodej::MetricKind objective_metric = nodej::MetricKind::Accuracy,
    nodej::TuneObjectivePolicy objective_policy = nodej::TuneObjectivePolicy::Auto,
    nodej::RandomState random = {});

StatusOr<std::unique_ptr<nodej::ClassifierBase>> linear_classifier(
    const nodej::LabeledBatch& train,
    const nodej::LinearFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::ClassifierBase>> linear_logistic_classifier(
    const nodej::LabeledBatch& train,
    double regularization_strength = 1e-6,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::ClassifierBase>> linear_hinge_classifier(
    const nodej::LabeledBatch& train,
    double regularization_strength = 1e-6,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::ClassifierBase>> tree_classifier(
    const nodej::LabeledBatch& train,
    const nodej::TreeFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::ClassifierBase>> forest_classifier(
    const nodej::LabeledBatch& train,
    std::int64_t learners = 100,
    double subsample = 1.0,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::ClassifierBase>> ensemble_classifier(
    const nodej::LabeledBatch& train,
    const nodej::EnsembleFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::ClassifierBase>> neighbor_classifier(
    const nodej::LabeledBatch& train,
    const nodej::NeighborFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::ClassifierBase>> bayes_classifier(
    const nodej::LabeledBatch& train,
    const nodej::BayesFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::ClassifierBase>> gaussian_bayes_classifier(
    const nodej::LabeledBatch& train,
    double smoothing = 1e-9,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::ClassifierBase>> kernel_classifier(
    const nodej::LabeledBatch& train,
    const nodej::KernelFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::ClassifierBase>> rbf_kernel_classifier(
    const nodej::LabeledBatch& train,
    double c = 1.0,
    double gamma = 0.0,
    double epsilon = 0.0,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> linear_regressor(
    const nodej::LabeledBatch& train,
    const nodej::LinearFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> ridge_regressor(
    const nodej::LabeledBatch& train,
    double regularization_strength = 1e-6,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> tree_regressor(
    const nodej::LabeledBatch& train,
    const nodej::TreeFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> forest_regressor(
    const nodej::LabeledBatch& train,
    std::int64_t learners = 100,
    double subsample = 1.0,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> ensemble_regressor(
    const nodej::LabeledBatch& train,
    const nodej::EnsembleFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> neighbor_regressor(
    const nodej::LabeledBatch& train,
    const nodej::NeighborFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> bayes_regressor(
    const nodej::LabeledBatch& train,
    const nodej::BayesFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> gaussian_bayes_regressor(
    const nodej::LabeledBatch& train,
    double smoothing = 1e-6,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> kernel_regressor(
    const nodej::LabeledBatch& train,
    const nodej::KernelFamilyOptions& family_options = {},
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> rbf_kernel_regressor(
    const nodej::LabeledBatch& train,
    double c = 1.0,
    double gamma = 0.0,
    double epsilon = 0.0,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::TransformerBase>> kmeans_clusterer(
    const nodej::FeatureBatch& train,
    std::int64_t clusters = 8,
    std::int64_t restarts = 4,
    std::int64_t max_iterations = 300,
    double tolerance = 1e-4,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::TransformerBase>> pca_mapper(
    const nodej::FeatureBatch& train,
    std::int64_t components = -1,
    bool whiten = false,
    std::int64_t max_iterations = 500,
    double tolerance = 1e-6,
    const nodej::TrainOptions& options = {});

} // namespace pluss::ai::lymph::quick
