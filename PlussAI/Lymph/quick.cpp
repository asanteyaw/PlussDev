#include "pluss/ai/lymph/quick.h"

namespace pluss::ai::lymph::quick {
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

StatusOr<std::unique_ptr<nodej::TransformerBase>> learn_transformer(const nodej::ModelSpec& spec,
                                                                     const nodej::FeatureBatch& train,
                                                                     const nodej::TrainOptions& options) {
  StatusOr<std::unique_ptr<nodej::TransformerBase>> model_or = nodej::make_transformer(spec);
  if (!model_or.ok()) return StatusOr<std::unique_ptr<nodej::TransformerBase>>(model_or.status());
  std::unique_ptr<nodej::TransformerBase> model = model_or.move_value();
  nodej::Status status = model->learn(train, options);
  if (!status.ok()) return StatusOr<std::unique_ptr<nodej::TransformerBase>>(std::move(status));
  return StatusOr<std::unique_ptr<nodej::TransformerBase>>(std::move(model));
}

StatusOr<nodej::Flow> learn_flow(nodej::Flow flow,
                                  const nodej::LabeledBatch& train,
                                  const nodej::TrainOptions& options) {
  nodej::Status status = flow.learn(train, options);
  if (!status.ok()) return StatusOr<nodej::Flow>(std::move(status));
  return StatusOr<nodej::Flow>(std::move(flow));
}

StatusOr<nodej::Flow> learn_flow(nodej::Flow flow,
                                  const pluss::table::DataFrame& frame,
                                  const nodej::DataFrameBatchOptions& data_options,
                                  const nodej::TrainOptions& options) {
  nodej::Status status = flow.learn(frame, data_options, options);
  if (!status.ok()) return StatusOr<nodej::Flow>(std::move(status));
  return StatusOr<nodej::Flow>(std::move(flow));
}

StatusOr<FrameFlowArtifact> fit_evaluate_save(const nodej::ModelSpec& spec,
                                              const pluss::table::DataFrame& frame,
                                              const nodej::DataFrameBatchOptions& data_options,
                                              const nodej::TrainOptions& train_options,
                                              const nodej::AssessOptions& assess_options,
                                              const std::string& save_path,
                                              nodej::io::FlowBundleMetadata metadata) {
  auto flow_or = nodej::make_flow(spec);
  if (!flow_or.ok()) return StatusOr<FrameFlowArtifact>(flow_or.status());
  nodej::Flow flow = flow_or.move_value();

  auto batch_or = nodej::labeled_batch_from_dataframe(frame, data_options);
  if (!batch_or.ok()) return StatusOr<FrameFlowArtifact>(batch_or.status());
  nodej::LabeledBatch batch = batch_or.move_value();

  nodej::Status learn_status = flow.learn(batch, train_options);
  if (!learn_status.ok()) return StatusOr<FrameFlowArtifact>(std::move(learn_status));

  auto metrics_or = flow.assess(batch, assess_options);
  if (!metrics_or.ok()) return StatusOr<FrameFlowArtifact>(metrics_or.status());

  if (!save_path.empty()) {
    nodej::Status save_status = nodej::io::save(save_path, flow, batch, train_options, std::move(metadata));
    if (!save_status.ok()) return StatusOr<FrameFlowArtifact>(std::move(save_status));
  }

  FrameFlowArtifact artifact{};
  artifact.flow = std::move(flow);
  artifact.metrics = metrics_or.move_value();
  artifact.train_batch = std::move(batch);
  return StatusOr<FrameFlowArtifact>(std::move(artifact));
}

StatusOr<nodej::Flow> linear_classifier_flow(const nodej::LabeledBatch& train,
                                             double regularization_strength,
                                             bool with_standard_scaler,
                                             bool with_mean_imputer,
                                             const nodej::TrainOptions& options) {
  nodej::ModelSpec spec{
      .task = nodej::TaskKind::Classification,
      .family = nodej::ModelFamily::Linear,
      .options = nodej::LinearFamilyOptions{
          .objective = nodej::LinearObjective::Logistic,
          .regularization = nodej::RegularizationKind::L2,
          .regularization_strength = regularization_strength,
          .fit_intercept = true,
      },
  };
  StatusOr<nodej::Flow> flow_or = nodej::make_flow(spec);
  if (!flow_or.ok()) return StatusOr<nodej::Flow>(flow_or.status());
  nodej::Flow flow = flow_or.move_value();

  if (with_mean_imputer) {
    nodej::Status s = flow.add_simple_imputer(nodej::prep::SimpleImputerOptions{
        .strategy = nodej::prep::SimpleImputerStrategy::Mean,
    });
    if (!s.ok()) return StatusOr<nodej::Flow>(std::move(s));
  }
  if (with_standard_scaler) {
    nodej::Status s = flow.add_standard_scaler();
    if (!s.ok()) return StatusOr<nodej::Flow>(std::move(s));
  }
  return learn_flow(std::move(flow), train, options);
}

StatusOr<nodej::Flow> ridge_regressor_flow(const nodej::LabeledBatch& train,
                                           double regularization_strength,
                                           bool with_standard_scaler,
                                           bool with_mean_imputer,
                                           const nodej::TrainOptions& options) {
  nodej::ModelSpec spec{
      .task = nodej::TaskKind::Regression,
      .family = nodej::ModelFamily::Linear,
      .options = nodej::LinearFamilyOptions{
          .objective = nodej::LinearObjective::LeastSquares,
          .regularization = nodej::RegularizationKind::L2,
          .regularization_strength = regularization_strength,
          .fit_intercept = true,
      },
  };
  StatusOr<nodej::Flow> flow_or = nodej::make_flow(spec);
  if (!flow_or.ok()) return StatusOr<nodej::Flow>(flow_or.status());
  nodej::Flow flow = flow_or.move_value();

  if (with_mean_imputer) {
    nodej::Status s = flow.add_simple_imputer(nodej::prep::SimpleImputerOptions{
        .strategy = nodej::prep::SimpleImputerStrategy::Mean,
    });
    if (!s.ok()) return StatusOr<nodej::Flow>(std::move(s));
  }
  if (with_standard_scaler) {
    nodej::Status s = flow.add_standard_scaler();
    if (!s.ok()) return StatusOr<nodej::Flow>(std::move(s));
  }
  return learn_flow(std::move(flow), train, options);
}

StatusOr<nodej::Flow> linear_regressor_flow(const nodej::LabeledBatch& train,
                                            const nodej::LinearFamilyOptions& family_options,
                                            bool with_standard_scaler,
                                            bool with_mean_imputer,
                                            const nodej::TrainOptions& options) {
  nodej::LinearFamilyOptions resolved = family_options;
  resolved.objective = nodej::LinearObjective::LeastSquares;
  nodej::ModelSpec spec{
      .task = nodej::TaskKind::Regression,
      .family = nodej::ModelFamily::Linear,
      .options = resolved,
  };
  StatusOr<nodej::Flow> flow_or = nodej::make_flow(spec);
  if (!flow_or.ok()) return StatusOr<nodej::Flow>(flow_or.status());
  nodej::Flow flow = flow_or.move_value();

  if (with_mean_imputer) {
    nodej::Status s = flow.add_simple_imputer(nodej::prep::SimpleImputerOptions{
        .strategy = nodej::prep::SimpleImputerStrategy::Mean,
    });
    if (!s.ok()) return StatusOr<nodej::Flow>(std::move(s));
  }
  if (with_standard_scaler) {
    nodej::Status s = flow.add_standard_scaler();
    if (!s.ok()) return StatusOr<nodej::Flow>(std::move(s));
  }
  return learn_flow(std::move(flow), train, options);
}

StatusOr<nodej::Flow> tree_regressor_flow(const nodej::LabeledBatch& train,
                                          const nodej::TreeFamilyOptions& family_options,
                                          bool with_mean_imputer,
                                          const nodej::TrainOptions& options) {
  nodej::TreeFamilyOptions resolved = family_options;
  if (resolved.criterion == nodej::TreeCriterion::Gini || resolved.criterion == nodej::TreeCriterion::Entropy) {
    resolved.criterion = nodej::TreeCriterion::Mse;
  }
  nodej::ModelSpec spec{
      .task = nodej::TaskKind::Regression,
      .family = nodej::ModelFamily::Tree,
      .options = resolved,
  };
  StatusOr<nodej::Flow> flow_or = nodej::make_flow(spec);
  if (!flow_or.ok()) return StatusOr<nodej::Flow>(flow_or.status());
  nodej::Flow flow = flow_or.move_value();

  if (with_mean_imputer) {
    nodej::Status s = flow.add_simple_imputer(nodej::prep::SimpleImputerOptions{
        .strategy = nodej::prep::SimpleImputerStrategy::Mean,
    });
    if (!s.ok()) return StatusOr<nodej::Flow>(std::move(s));
  }
  return learn_flow(std::move(flow), train, options);
}

StatusOr<nodej::online::OnlineLinearClassifier> online_linear_classifier(
    const nodej::LabeledBatch& warm_start,
    const nodej::online::OnlineLinearOptions& online_options,
    const nodej::TrainOptions& train_options) {
  auto model_or = nodej::online::make_online_linear_classifier(online_options);
  if (!model_or.ok()) return StatusOr<nodej::online::OnlineLinearClassifier>(model_or.status());
  nodej::online::OnlineLinearClassifier model = model_or.move_value();
  nodej::Status status = model.learn(warm_start, train_options);
  if (!status.ok()) return StatusOr<nodej::online::OnlineLinearClassifier>(std::move(status));
  return StatusOr<nodej::online::OnlineLinearClassifier>(std::move(model));
}

StatusOr<nodej::online::OnlineLinearRegressor> online_linear_regressor(
    const nodej::LabeledBatch& warm_start,
    const nodej::online::OnlineLinearOptions& online_options,
    const nodej::TrainOptions& train_options) {
  auto model_or = nodej::online::make_online_linear_regressor(online_options);
  if (!model_or.ok()) return StatusOr<nodej::online::OnlineLinearRegressor>(model_or.status());
  nodej::online::OnlineLinearRegressor model = model_or.move_value();
  nodej::Status status = model.learn(warm_start, train_options);
  if (!status.ok()) return StatusOr<nodej::online::OnlineLinearRegressor>(std::move(status));
  return StatusOr<nodej::online::OnlineLinearRegressor>(std::move(model));
}

StatusOr<nodej::CrossAssessReport> assess_cv(const nodej::ModelSpec& spec,
                                             const nodej::LabeledBatch& batch,
                                             const nodej::SplitOptions& split,
                                             const nodej::TrainOptions& train_options,
                                             const nodej::AssessOptions& assess_options) {
  return nodej::cross_assess(spec, batch, split, train_options, assess_options);
}

StatusOr<nodej::TuneReport> tune_sweep(const std::vector<nodej::ModelSpec>& candidates,
                                       const nodej::LabeledBatch& batch,
                                       const nodej::SplitOptions& split,
                                       const nodej::TrainOptions& train_options,
                                       const nodej::AssessOptions& assess_options,
                                       nodej::MetricKind objective_metric,
                                       nodej::TuneObjectivePolicy objective_policy) {
  return nodej::sweep(candidates,
                      batch,
                      split,
                      train_options,
                      assess_options,
                      objective_metric,
                      objective_policy);
}

StatusOr<nodej::TuneReport> tune_probe(const nodej::ProbeSpecSampler& sampler,
                                       std::int64_t trials,
                                       const nodej::LabeledBatch& batch,
                                       const nodej::SplitOptions& split,
                                       const nodej::TrainOptions& train_options,
                                       const nodej::AssessOptions& assess_options,
                                       nodej::MetricKind objective_metric,
                                       nodej::TuneObjectivePolicy objective_policy,
                                       nodej::RandomState random) {
  return nodej::probe(sampler,
                      trials,
                      batch,
                      split,
                      train_options,
                      assess_options,
                      objective_metric,
                      objective_policy,
                      random);
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

StatusOr<std::unique_ptr<nodej::TransformerBase>> kmeans_clusterer(const nodej::FeatureBatch& train,
                                                                    std::int64_t clusters,
                                                                    std::int64_t restarts,
                                                                    std::int64_t max_iterations,
                                                                    double tolerance,
                                                                    const nodej::TrainOptions& options) {
  const nodej::ModelSpec spec{
      .task = nodej::TaskKind::Transform,
      .family = nodej::ModelFamily::Cluster,
      .options = nodej::ClusterFamilyOptions{
          .method = nodej::ClusterMethod::KMeans,
          .clusters = clusters,
          .max_iterations = max_iterations,
          .restarts = restarts,
          .tolerance = tolerance,
      },
  };
  return learn_transformer(spec, train, options);
}

StatusOr<std::unique_ptr<nodej::TransformerBase>> pca_mapper(const nodej::FeatureBatch& train,
                                                              std::int64_t components,
                                                              bool whiten,
                                                              std::int64_t max_iterations,
                                                              double tolerance,
                                                              const nodej::TrainOptions& options) {
  const nodej::ModelSpec spec{
      .task = nodej::TaskKind::Transform,
      .family = nodej::ModelFamily::Decompose,
      .options = nodej::DecomposeFamilyOptions{
          .method = nodej::DecomposeMethod::Pca,
          .components = components,
          .whiten = whiten,
          .max_iterations = max_iterations,
          .tolerance = tolerance,
      },
  };
  return learn_transformer(spec, train, options);
}

} // namespace pluss::ai::lymph::quick
