#pragma once

#include "pluss/ai/lymph/common.h"
#include "pluss/ai/lymph/workflow_model.h"

namespace pluss::ai::lymph {

struct linear_factory {
  [[nodiscard]] static WorkflowModel linear_regression(LinearFamilyOptions options = {}) {
    options.objective = LinearObjective::LeastSquares;
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel logistic_regression(LinearFamilyOptions options = {}) {
    if (options.objective == LinearObjective::LeastSquares) options.objective = LinearObjective::Logistic;
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel hinge_classifier(LinearFamilyOptions options = {}) {
    options.objective = LinearObjective::Hinge;
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = options,
    });
  }
};

struct tree_factory {
  [[nodiscard]] static WorkflowModel decision_tree_regressor(TreeFamilyOptions options = {}) {
    if (options.criterion == TreeCriterion::Gini || options.criterion == TreeCriterion::Entropy) {
      options.criterion = TreeCriterion::Mse;
    }
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Tree,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel decision_tree_classifier(TreeFamilyOptions options = {}) {
    if (options.criterion == TreeCriterion::Mse || options.criterion == TreeCriterion::Mae) {
      options.criterion = TreeCriterion::Gini;
    }
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Tree,
        .options = options,
    });
  }
};

struct ensemble_factory {
  [[nodiscard]] static WorkflowModel bagging_regressor(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::Bagging;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel random_forest_regressor(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::RandomForest;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel bagging_classifier(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::Bagging;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel random_forest_classifier(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::RandomForest;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel gradient_boost_regressor(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::GradientBoost;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel gradient_boost_classifier(EnsembleFamilyOptions options = {}) {
    options.method = EnsembleMethod::GradientBoost;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel regressor(EnsembleFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel classifier(EnsembleFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Ensemble,
        .options = options,
    });
  }
};

struct neighbor_factory {
  [[nodiscard]] static WorkflowModel knn_regressor(NeighborFamilyOptions options = {}) {
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel knn_classifier(NeighborFamilyOptions options = {}) {
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel radius_neighbors_regressor(NeighborFamilyOptions options = {}) {
    options.query_mode = NeighborQueryMode::Radius;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel radius_neighbors_classifier(NeighborFamilyOptions options = {}) {
    options.query_mode = NeighborQueryMode::Radius;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel regressor(NeighborFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Neighbor,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel classifier(NeighborFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Neighbor,
        .options = options,
    });
  }
};

struct kernel_factory {
  [[nodiscard]] static WorkflowModel svm_regressor(KernelFamilyOptions options = {}) {
    options.form = KernelForm::C;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel svm_classifier(KernelFamilyOptions options = {}) {
    options.form = KernelForm::C;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel nu_svm_regressor(KernelFamilyOptions options = {}) {
    options.form = KernelForm::Nu;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel nu_svm_classifier(KernelFamilyOptions options = {}) {
    options.form = KernelForm::Nu;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel one_class_svm(KernelFamilyOptions options = {}) {
    options.form = KernelForm::OneClass;
    return transformer(options);
  }

  [[nodiscard]] static WorkflowModel linear_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Linear;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel rbf_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Rbf;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel polynomial_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Polynomial;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel sigmoid_kernel_regressor(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Sigmoid;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel linear_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Linear;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel rbf_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Rbf;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel polynomial_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Polynomial;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel sigmoid_kernel_classifier(KernelFamilyOptions options = {}) {
    options.kernel = KernelKind::Sigmoid;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel regressor(KernelFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel classifier(KernelFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Kernel,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel transformer(KernelFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Kernel,
        .options = options,
    });
  }
};

struct bayes_factory {
  [[nodiscard]] static WorkflowModel gaussian_nb_classifier(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Gaussian;
    options.use_gaussian = true;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel gaussian_nb_regressor(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Gaussian;
    options.use_gaussian = true;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel bernoulli_nb_classifier(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Bernoulli;
    options.use_gaussian = false;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel bernoulli_nb_regressor(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Bernoulli;
    options.use_gaussian = false;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel multinomial_nb_classifier(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Multinomial;
    options.use_gaussian = false;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel complement_nb_classifier(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Complement;
    options.use_gaussian = false;
    return classifier(options);
  }

  [[nodiscard]] static WorkflowModel multinomial_nb_regressor(BayesFamilyOptions options = {}) {
    options.likelihood = BayesLikelihood::Multinomial;
    options.use_gaussian = false;
    return regressor(options);
  }

  [[nodiscard]] static WorkflowModel regressor(BayesFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Bayes,
        .options = options,
    });
  }

  [[nodiscard]] static WorkflowModel classifier(BayesFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Bayes,
        .options = options,
    });
  }
};

struct cluster_factory {
  [[nodiscard]] static WorkflowModel kmeans(ClusterFamilyOptions options = {}) {
    options.method = ClusterMethod::KMeans;
    return transformer(options);
  }

  [[nodiscard]] static WorkflowModel transformer(ClusterFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Cluster,
        .options = options,
    });
  }
};

struct decompose_factory {
  [[nodiscard]] static WorkflowModel pca(DecomposeFamilyOptions options = {}) {
    options.method = DecomposeMethod::Pca;
    return transformer(options);
  }

  [[nodiscard]] static WorkflowModel transformer(DecomposeFamilyOptions options = {}) {
    return WorkflowModel(ModelSpec{
        .task = TaskKind::Transform,
        .family = ModelFamily::Decompose,
        .options = options,
    });
  }
};

using OnlineLinearOptions = ::pluss::ai::nodej::online::OnlineLinearOptions;

} // namespace pluss::ai::lymph
