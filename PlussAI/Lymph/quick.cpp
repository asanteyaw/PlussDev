#include "pluss/ai/lymph/quick.h"

namespace pluss::ai::lymph {
namespace {

template <class FamilyOptionsT>
StatusOr<std::unique_ptr<nodej::ClassifierBase>> learn_classifier_family(nodej::ModelFamily family,
                                                                         const FamilyOptionsT& family_options,
                                                                         const nodej::LabeledBatch& train,
                                                                         const nodej::TrainOptions& options) {
  const nodej::ModelSpec spec{
      .task = nodej::TaskKind::Classification,
      .family = family,
      .options = family_options,
  };
  return learn_classifier(spec, train, options);
}

template <class FamilyOptionsT>
StatusOr<std::unique_ptr<nodej::RegressorBase>> learn_regressor_family(nodej::ModelFamily family,
                                                                       const FamilyOptionsT& family_options,
                                                                       const nodej::LabeledBatch& train,
                                                                       const nodej::TrainOptions& options) {
  const nodej::ModelSpec spec{
      .task = nodej::TaskKind::Regression,
      .family = family,
      .options = family_options,
  };
  return learn_regressor(spec, train, options);
}

} // namespace

StatusOr<std::unique_ptr<nodej::ClassifierBase>> learn_classifier(const nodej::ModelSpec& spec,
                                                                  const nodej::LabeledBatch& train,
                                                                  const nodej::TrainOptions& options) {
  StatusOr<std::unique_ptr<nodej::ClassifierBase>> model_or = nodej::make_classifier(spec);
  if (!model_or.ok()) return StatusOr<std::unique_ptr<nodej::ClassifierBase>>(model_or.status());
  std::unique_ptr<nodej::ClassifierBase> model = model_or.move_value();
  nodej::Status status = model->learn(train, options);
  if (!status.ok()) return StatusOr<std::unique_ptr<nodej::ClassifierBase>>(std::move(status));
  return StatusOr<std::unique_ptr<nodej::ClassifierBase>>(std::move(model));
}

StatusOr<std::unique_ptr<nodej::RegressorBase>> learn_regressor(const nodej::ModelSpec& spec,
                                                                 const nodej::LabeledBatch& train,
                                                                 const nodej::TrainOptions& options) {
  StatusOr<std::unique_ptr<nodej::RegressorBase>> model_or = nodej::make_regressor(spec);
  if (!model_or.ok()) return StatusOr<std::unique_ptr<nodej::RegressorBase>>(model_or.status());
  std::unique_ptr<nodej::RegressorBase> model = model_or.move_value();
  nodej::Status status = model->learn(train, options);
  if (!status.ok()) return StatusOr<std::unique_ptr<nodej::RegressorBase>>(std::move(status));
  return StatusOr<std::unique_ptr<nodej::RegressorBase>>(std::move(model));
}

StatusOr<std::unique_ptr<nodej::ClassifierBase>> linear_classifier(const nodej::LabeledBatch& train,
                                                                   const nodej::LinearFamilyOptions& family_options,
                                                                   const nodej::TrainOptions& options) {
  nodej::LinearFamilyOptions resolved = family_options;
  if (resolved.objective == nodej::LinearObjective::LeastSquares) {
    resolved.objective = nodej::LinearObjective::Logistic;
  }
  return learn_classifier_family(nodej::ModelFamily::Linear, resolved, train, options);
}

StatusOr<std::unique_ptr<nodej::ClassifierBase>> linear_logistic_classifier(const nodej::LabeledBatch& train,
                                                                             double regularization_strength,
                                                                             const nodej::TrainOptions& options) {
  return linear_classifier(
      train,
      nodej::LinearFamilyOptions{
          .objective = nodej::LinearObjective::Logistic,
          .regularization = nodej::RegularizationKind::L2,
          .regularization_strength = regularization_strength,
          .fit_intercept = true,
      },
      options);
}

StatusOr<std::unique_ptr<nodej::ClassifierBase>> linear_hinge_classifier(const nodej::LabeledBatch& train,
                                                                          double regularization_strength,
                                                                          const nodej::TrainOptions& options) {
  return linear_classifier(
      train,
      nodej::LinearFamilyOptions{
          .objective = nodej::LinearObjective::Hinge,
          .regularization = nodej::RegularizationKind::L2,
          .regularization_strength = regularization_strength,
          .fit_intercept = true,
      },
      options);
}

StatusOr<std::unique_ptr<nodej::ClassifierBase>> tree_classifier(const nodej::LabeledBatch& train,
                                                                 const nodej::TreeFamilyOptions& family_options,
                                                                 const nodej::TrainOptions& options) {
  return learn_classifier_family(nodej::ModelFamily::Tree, family_options, train, options);
}

StatusOr<std::unique_ptr<nodej::ClassifierBase>> forest_classifier(const nodej::LabeledBatch& train,
                                                                   std::int64_t learners,
                                                                   double subsample,
                                                                   const nodej::TrainOptions& options) {
  return ensemble_classifier(
      train,
      nodej::EnsembleFamilyOptions{
          .method = nodej::EnsembleMethod::RandomForest,
          .learners = learners,
          .subsample = subsample,
      },
      options);
}

StatusOr<std::unique_ptr<nodej::ClassifierBase>> ensemble_classifier(const nodej::LabeledBatch& train,
                                                                     const nodej::EnsembleFamilyOptions& family_options,
                                                                     const nodej::TrainOptions& options) {
  return learn_classifier_family(nodej::ModelFamily::Ensemble, family_options, train, options);
}

StatusOr<std::unique_ptr<nodej::ClassifierBase>> neighbor_classifier(
    const nodej::LabeledBatch& train,
    const nodej::NeighborFamilyOptions& family_options,
    const nodej::TrainOptions& options) {
  return learn_classifier_family(nodej::ModelFamily::Neighbor, family_options, train, options);
}

StatusOr<std::unique_ptr<nodej::ClassifierBase>> bayes_classifier(const nodej::LabeledBatch& train,
                                                                  const nodej::BayesFamilyOptions& family_options,
                                                                  const nodej::TrainOptions& options) {
  return learn_classifier_family(nodej::ModelFamily::Bayes, family_options, train, options);
}

StatusOr<std::unique_ptr<nodej::ClassifierBase>> gaussian_bayes_classifier(const nodej::LabeledBatch& train,
                                                                            double smoothing,
                                                                            const nodej::TrainOptions& options) {
  return bayes_classifier(
      train,
      nodej::BayesFamilyOptions{
          .use_gaussian = true,
          .smoothing = smoothing,
      },
      options);
}

StatusOr<std::unique_ptr<nodej::ClassifierBase>> kernel_classifier(const nodej::LabeledBatch& train,
                                                                   const nodej::KernelFamilyOptions& family_options,
                                                                   const nodej::TrainOptions& options) {
  return learn_classifier_family(nodej::ModelFamily::Kernel, family_options, train, options);
}

StatusOr<std::unique_ptr<nodej::ClassifierBase>> rbf_kernel_classifier(const nodej::LabeledBatch& train,
                                                                        double c,
                                                                        double gamma,
                                                                        double epsilon,
                                                                        const nodej::TrainOptions& options) {
  return kernel_classifier(
      train,
      nodej::KernelFamilyOptions{
          .kernel = nodej::KernelKind::Rbf,
          .c = c,
          .gamma = gamma,
          .epsilon = epsilon,
      },
      options);
}

StatusOr<std::unique_ptr<nodej::RegressorBase>> linear_regressor(const nodej::LabeledBatch& train,
                                                                 const nodej::LinearFamilyOptions& family_options,
                                                                 const nodej::TrainOptions& options) {
  return learn_regressor_family(nodej::ModelFamily::Linear, family_options, train, options);
}

StatusOr<std::unique_ptr<nodej::RegressorBase>> ridge_regressor(const nodej::LabeledBatch& train,
                                                                 double regularization_strength,
                                                                 const nodej::TrainOptions& options) {
  return linear_regressor(
      train,
      nodej::LinearFamilyOptions{
          .objective = nodej::LinearObjective::LeastSquares,
          .regularization = nodej::RegularizationKind::L2,
          .regularization_strength = regularization_strength,
          .fit_intercept = true,
      },
      options);
}

StatusOr<std::unique_ptr<nodej::RegressorBase>> tree_regressor(const nodej::LabeledBatch& train,
                                                               const nodej::TreeFamilyOptions& family_options,
                                                               const nodej::TrainOptions& options) {
  return learn_regressor_family(nodej::ModelFamily::Tree, family_options, train, options);
}

StatusOr<std::unique_ptr<nodej::RegressorBase>> forest_regressor(const nodej::LabeledBatch& train,
                                                                  std::int64_t learners,
                                                                  double subsample,
                                                                  const nodej::TrainOptions& options) {
  return ensemble_regressor(
      train,
      nodej::EnsembleFamilyOptions{
          .method = nodej::EnsembleMethod::RandomForest,
          .learners = learners,
          .subsample = subsample,
      },
      options);
}

StatusOr<std::unique_ptr<nodej::RegressorBase>> ensemble_regressor(const nodej::LabeledBatch& train,
                                                                   const nodej::EnsembleFamilyOptions& family_options,
                                                                   const nodej::TrainOptions& options) {
  return learn_regressor_family(nodej::ModelFamily::Ensemble, family_options, train, options);
}

StatusOr<std::unique_ptr<nodej::RegressorBase>> neighbor_regressor(const nodej::LabeledBatch& train,
                                                                   const nodej::NeighborFamilyOptions& family_options,
                                                                   const nodej::TrainOptions& options) {
  return learn_regressor_family(nodej::ModelFamily::Neighbor, family_options, train, options);
}

StatusOr<std::unique_ptr<nodej::RegressorBase>> bayes_regressor(const nodej::LabeledBatch& train,
                                                                const nodej::BayesFamilyOptions& family_options,
                                                                const nodej::TrainOptions& options) {
  return learn_regressor_family(nodej::ModelFamily::Bayes, family_options, train, options);
}

StatusOr<std::unique_ptr<nodej::RegressorBase>> gaussian_bayes_regressor(const nodej::LabeledBatch& train,
                                                                          double smoothing,
                                                                          const nodej::TrainOptions& options) {
  return bayes_regressor(
      train,
      nodej::BayesFamilyOptions{
          .use_gaussian = true,
          .smoothing = smoothing,
      },
      options);
}

StatusOr<std::unique_ptr<nodej::RegressorBase>> kernel_regressor(const nodej::LabeledBatch& train,
                                                                 const nodej::KernelFamilyOptions& family_options,
                                                                 const nodej::TrainOptions& options) {
  return learn_regressor_family(nodej::ModelFamily::Kernel, family_options, train, options);
}

StatusOr<std::unique_ptr<nodej::RegressorBase>> rbf_kernel_regressor(const nodej::LabeledBatch& train,
                                                                      double c,
                                                                      double gamma,
                                                                      double epsilon,
                                                                      const nodej::TrainOptions& options) {
  return kernel_regressor(
      train,
      nodej::KernelFamilyOptions{
          .kernel = nodej::KernelKind::Rbf,
          .c = c,
          .gamma = gamma,
          .epsilon = epsilon,
      },
      options);
}

} // namespace pluss::ai::lymph
