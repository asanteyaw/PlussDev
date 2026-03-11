#pragma once

#include "pluss/ai/nodej/builders.h"

#include <cstdint>
#include <memory>

namespace pluss::ai::lymph {

template <class T>
using StatusOr = nodej::StatusOr<T>;

StatusOr<std::unique_ptr<nodej::ClassifierBase>> learn_classifier(
    const nodej::ModelSpec& spec,
    const nodej::LabeledBatch& train,
    const nodej::TrainOptions& options = {});

StatusOr<std::unique_ptr<nodej::RegressorBase>> learn_regressor(
    const nodej::ModelSpec& spec,
    const nodej::LabeledBatch& train,
    const nodej::TrainOptions& options = {});

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

} // namespace pluss::ai::lymph
