#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/detail/model_core.h"

namespace pluss::ai::lymph {

namespace detail {

struct linear_factory {
  [[nodiscard]] static detail::ModelCore linear_regression(LinearFamilyOptions options = {}) {
    options.objective = LinearObjective::LeastSquares;
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = options,
    });
  }

  [[nodiscard]] static detail::ModelCore logistic_regression(LinearFamilyOptions options = {}) {
    if (options.objective == LinearObjective::LeastSquares) options.objective = LinearObjective::Logistic;
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = options,
    });
  }

  [[nodiscard]] static detail::ModelCore hinge_classifier(LinearFamilyOptions options = {}) {
    options.objective = LinearObjective::Hinge;
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = options,
    });
  }
};

struct tree_factory {
  [[nodiscard]] static detail::ModelCore decision_tree_regressor(TreeFamilyOptions options = {}) {
    if (options.criterion == TreeCriterion::Gini || options.criterion == TreeCriterion::Entropy) {
      options.criterion = TreeCriterion::Mse;
    }
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Tree,
        .options = options,
    });
  }

  [[nodiscard]] static detail::ModelCore decision_tree_classifier(TreeFamilyOptions options = {}) {
    if (options.criterion == TreeCriterion::Mse || options.criterion == TreeCriterion::Mae) {
      options.criterion = TreeCriterion::Gini;
    }
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Tree,
        .options = options,
    });
  }
};

struct ensemble_factory {
  [[nodiscard]] static detail::ModelCore bagging_regressor(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::Bagging;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore random_forest_regressor(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::RandomForest;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore bagging_classifier(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::Bagging;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore random_forest_classifier(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::RandomForest;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore gradient_boost_regressor(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::GradientBoost;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore gradient_boost_classifier(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::GradientBoost;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore regressor(EnsembleFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = options,
    });
  }

  [[nodiscard]] static detail::ModelCore classifier(EnsembleFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Ensemble,
        .options = options,
    });
  }
};

struct neighbor_factory {
  [[nodiscard]] static detail::ModelCore knn_regressor(NeighborFamilyOptions options = {}) {
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore knn_classifier(NeighborFamilyOptions options = {}) {
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore radius_neighbors_regressor(NeighborFamilyOptions options = {}) {
    options.query_mode = NeighborQueryMode::Radius;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore radius_neighbors_classifier(NeighborFamilyOptions options = {}) {
    options.query_mode = NeighborQueryMode::Radius;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore regressor(NeighborFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Neighbor,
        .options = options,
    });
  }

  [[nodiscard]] static detail::ModelCore classifier(NeighborFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Neighbor,
        .options = options,
    });
  }
};

struct kernel_factory {
  [[nodiscard]] static detail::ModelCore svm_regressor(KernelFamilyOptions options = {}) {
    options.form = KernelForm::C;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore svm_classifier(KernelFamilyOptions options = {}) {
    options.form = KernelForm::C;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore nu_svm_regressor(KernelFamilyOptions options = {}) {
    options.form = KernelForm::Nu;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore nu_svm_classifier(KernelFamilyOptions options = {}) {
    options.form = KernelForm::Nu;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore one_class_svm(KernelFamilyOptions options = {}) {
    options.form = KernelForm::OneClass;
    return transformer(options);
  }

  [[nodiscard]] static detail::ModelCore linear_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Linear;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore rbf_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Rbf;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore polynomial_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Polynomial;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore sigmoid_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Sigmoid;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore linear_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Linear;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore rbf_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Rbf;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore polynomial_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Polynomial;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore sigmoid_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Sigmoid;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore regressor(KernelFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = options,
    });
  }

  [[nodiscard]] static detail::ModelCore classifier(KernelFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Kernel,
        .options = options,
    });
  }

  [[nodiscard]] static detail::ModelCore transformer(KernelFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Kernel,
        .options = options,
    });
  }
};

struct bayes_factory {
  [[nodiscard]] static detail::ModelCore gaussian_nb_classifier(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Gaussian;
    options.use_gaussian = true;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore gaussian_nb_regressor(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Gaussian;
    options.use_gaussian = true;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore bernoulli_nb_classifier(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Bernoulli;
    options.use_gaussian = false;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore bernoulli_nb_regressor(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Bernoulli;
    options.use_gaussian = false;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore multinomial_nb_classifier(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Multinomial;
    options.use_gaussian = false;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore complement_nb_classifier(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Complement;
    options.use_gaussian = false;
    return classifier(options);
  }

  [[nodiscard]] static detail::ModelCore multinomial_nb_regressor(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Multinomial;
    options.use_gaussian = false;
    return regressor(options);
  }

  [[nodiscard]] static detail::ModelCore regressor(BayesFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Bayes,
        .options = options,
    });
  }

  [[nodiscard]] static detail::ModelCore classifier(BayesFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Bayes,
        .options = options,
    });
  }
};

struct cluster_factory {
  [[nodiscard]] static detail::ModelCore kmeans(ClusterFamilyOptions options = {}) {
    options.method = ClusterMethod::KMeans;
    return transformer(options);
  }

  [[nodiscard]] static detail::ModelCore transformer(ClusterFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Cluster,
        .options = options,
    });
  }
};

struct decompose_factory {
  [[nodiscard]] static detail::ModelCore pca(DecomposeFamilyOptions options = {}) {
    options.method = DecomposeMethod::Pca;
    return transformer(options);
  }

  [[nodiscard]] static detail::ModelCore transformer(DecomposeFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Decompose,
        .options = options,
    });
  }
};

struct detect_factory {
  [[nodiscard]] static detail::ModelCore isolation_detector(DetectFamilyOptions options = {}) {
    options.method = DetectMethod::Isolation;
    return transformer(options);
  }

  [[nodiscard]] static detail::ModelCore isolation_detector(prep::OutlierIsolatorOptions options) {
    DetectFamilyOptions detect_options{};
    detect_options.method = DetectMethod::Isolation;
    detect_options.isolation = options;
    return transformer(detect_options);
  }

  [[nodiscard]] static detail::ModelCore transformer(DetectFamilyOptions options = {}) {
    return detail::ModelCore(ModelSpec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Detect,
        .options = options,
    });
  }
};

} // namespace detail

using OnlineLinearOptions = ::pluss::ai::nodej::online::OnlineLinearOptions;

} // namespace pluss::ai::lymph
