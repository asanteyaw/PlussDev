#pragma once

#include "pluss/ai/lymph/workflow_model.h"
#include "pluss/ai/lymph/factories.h"

// model.h -- Model (LYMPH_REDESIGN_PLAN.md phase 3 + 6).
//
// Phase 3 (rename): `Model` is a type alias for `WorkflowModel`, not a
// physical rename yet -- every existing WorkflowModel call site (this
// session's whole downstream history) keeps compiling unchanged while new
// code can start writing `Model`. Retiring the `Workflow` lazy facade
// (phase 4) is select.h's concern, not this file's, since that's about
// what `Comparison`'s result holds, not `Model` itself.
//
// Phase 6 (consolidate factories): `lymph::model::` re-exports every
// existing linear_factory::/tree_factory::/ensemble_factory::/
// neighbor_factory::/kernel_factory::/bayes_factory::/cluster_factory::/
// decompose_factory:: static method into one namespace. Attempted this as
// bare `using linear_factory::linear_regression;`-style using-declarations
// first -- C++ does not allow a using-declaration at namespace scope to
// name a class's static member ("using declaration cannot refer to class
// member"; that form is only valid inside another class body, for
// inheritance). One-line forwarding wrapper functions below instead: each
// just calls straight through to the existing *_factory:: static method,
// so there is still nothing to keep in sync by hand beyond the function
// name itself. Where several families already share a name
// (`regressor`/`classifier`/`transformer`, each taking a different
// *FamilyOptions type), the wrappers become ordinary overloads resolved by
// argument type -- exactly the behavior a caller already gets today spread
// across seven different struct names, just reachable from one namespace
// instead of seven.

namespace pluss::ai::lymph {

using Model = WorkflowModel;

namespace model {

[[nodiscard]] inline Model linear_regression(LinearFamilyOptions options = {}) { return linear_factory::linear_regression(options); }
[[nodiscard]] inline Model logistic_regression(LinearFamilyOptions options = {}) { return linear_factory::logistic_regression(options); }
[[nodiscard]] inline Model hinge_classifier(LinearFamilyOptions options = {}) { return linear_factory::hinge_classifier(options); }

[[nodiscard]] inline Model decision_tree_regressor(TreeFamilyOptions options = {}) { return tree_factory::decision_tree_regressor(options); }
[[nodiscard]] inline Model decision_tree_classifier(TreeFamilyOptions options = {}) { return tree_factory::decision_tree_classifier(options); }

[[nodiscard]] inline Model bagging_regressor(EnsembleFamilyOptions options = {}) { return ensemble_factory::bagging_regressor(options); }
[[nodiscard]] inline Model random_forest_regressor(EnsembleFamilyOptions options = {}) { return ensemble_factory::random_forest_regressor(options); }
[[nodiscard]] inline Model bagging_classifier(EnsembleFamilyOptions options = {}) { return ensemble_factory::bagging_classifier(options); }
[[nodiscard]] inline Model random_forest_classifier(EnsembleFamilyOptions options = {}) { return ensemble_factory::random_forest_classifier(options); }
[[nodiscard]] inline Model gradient_boost_regressor(EnsembleFamilyOptions options = {}) { return ensemble_factory::gradient_boost_regressor(options); }
[[nodiscard]] inline Model gradient_boost_classifier(EnsembleFamilyOptions options = {}) { return ensemble_factory::gradient_boost_classifier(options); }
[[nodiscard]] inline Model regressor(EnsembleFamilyOptions options = {}) { return ensemble_factory::regressor(options); }
[[nodiscard]] inline Model classifier(EnsembleFamilyOptions options = {}) { return ensemble_factory::classifier(options); }

[[nodiscard]] inline Model knn_regressor(NeighborFamilyOptions options = {}) { return neighbor_factory::knn_regressor(options); }
[[nodiscard]] inline Model knn_classifier(NeighborFamilyOptions options = {}) { return neighbor_factory::knn_classifier(options); }
[[nodiscard]] inline Model radius_neighbors_regressor(NeighborFamilyOptions options = {}) { return neighbor_factory::radius_neighbors_regressor(options); }
[[nodiscard]] inline Model radius_neighbors_classifier(NeighborFamilyOptions options = {}) { return neighbor_factory::radius_neighbors_classifier(options); }
[[nodiscard]] inline Model regressor(NeighborFamilyOptions options = {}) { return neighbor_factory::regressor(options); }
[[nodiscard]] inline Model classifier(NeighborFamilyOptions options = {}) { return neighbor_factory::classifier(options); }

[[nodiscard]] inline Model svm_regressor(KernelFamilyOptions options = {}) { return kernel_factory::svm_regressor(options); }
[[nodiscard]] inline Model svm_classifier(KernelFamilyOptions options = {}) { return kernel_factory::svm_classifier(options); }
[[nodiscard]] inline Model nu_svm_regressor(KernelFamilyOptions options = {}) { return kernel_factory::nu_svm_regressor(options); }
[[nodiscard]] inline Model nu_svm_classifier(KernelFamilyOptions options = {}) { return kernel_factory::nu_svm_classifier(options); }
[[nodiscard]] inline Model one_class_svm(KernelFamilyOptions options = {}) { return kernel_factory::one_class_svm(options); }
[[nodiscard]] inline Model linear_kernel_regressor(KernelFamilyOptions options = {}) { return kernel_factory::linear_kernel_regressor(options); }
[[nodiscard]] inline Model rbf_kernel_regressor(KernelFamilyOptions options = {}) { return kernel_factory::rbf_kernel_regressor(options); }
[[nodiscard]] inline Model polynomial_kernel_regressor(KernelFamilyOptions options = {}) { return kernel_factory::polynomial_kernel_regressor(options); }
[[nodiscard]] inline Model sigmoid_kernel_regressor(KernelFamilyOptions options = {}) { return kernel_factory::sigmoid_kernel_regressor(options); }
[[nodiscard]] inline Model linear_kernel_classifier(KernelFamilyOptions options = {}) { return kernel_factory::linear_kernel_classifier(options); }
[[nodiscard]] inline Model rbf_kernel_classifier(KernelFamilyOptions options = {}) { return kernel_factory::rbf_kernel_classifier(options); }
[[nodiscard]] inline Model polynomial_kernel_classifier(KernelFamilyOptions options = {}) { return kernel_factory::polynomial_kernel_classifier(options); }
[[nodiscard]] inline Model sigmoid_kernel_classifier(KernelFamilyOptions options = {}) { return kernel_factory::sigmoid_kernel_classifier(options); }
[[nodiscard]] inline Model regressor(KernelFamilyOptions options = {}) { return kernel_factory::regressor(options); }
[[nodiscard]] inline Model classifier(KernelFamilyOptions options = {}) { return kernel_factory::classifier(options); }
[[nodiscard]] inline Model transformer(KernelFamilyOptions options = {}) { return kernel_factory::transformer(options); }

[[nodiscard]] inline Model gaussian_nb_classifier(BayesFamilyOptions options = {}) { return bayes_factory::gaussian_nb_classifier(options); }
[[nodiscard]] inline Model gaussian_nb_regressor(BayesFamilyOptions options = {}) { return bayes_factory::gaussian_nb_regressor(options); }
[[nodiscard]] inline Model bernoulli_nb_classifier(BayesFamilyOptions options = {}) { return bayes_factory::bernoulli_nb_classifier(options); }
[[nodiscard]] inline Model bernoulli_nb_regressor(BayesFamilyOptions options = {}) { return bayes_factory::bernoulli_nb_regressor(options); }
[[nodiscard]] inline Model multinomial_nb_classifier(BayesFamilyOptions options = {}) { return bayes_factory::multinomial_nb_classifier(options); }
[[nodiscard]] inline Model complement_nb_classifier(BayesFamilyOptions options = {}) { return bayes_factory::complement_nb_classifier(options); }
[[nodiscard]] inline Model multinomial_nb_regressor(BayesFamilyOptions options = {}) { return bayes_factory::multinomial_nb_regressor(options); }
[[nodiscard]] inline Model regressor(BayesFamilyOptions options = {}) { return bayes_factory::regressor(options); }
[[nodiscard]] inline Model classifier(BayesFamilyOptions options = {}) { return bayes_factory::classifier(options); }

[[nodiscard]] inline Model kmeans(ClusterFamilyOptions options = {}) { return cluster_factory::kmeans(options); }
[[nodiscard]] inline Model transformer(ClusterFamilyOptions options = {}) { return cluster_factory::transformer(options); }

// PCA (and friends) as a standalone model in its own right -- distinct from
// the (not yet built) lymph::stage::pca(...) planned for attaching PCA as
// one step inside a larger pipeline (LYMPH_REDESIGN_PLAN.md 7.4).
[[nodiscard]] inline Model pca(DecomposeFamilyOptions options = {}) { return decompose_factory::pca(options); }
[[nodiscard]] inline Model transformer(DecomposeFamilyOptions options = {}) { return decompose_factory::transformer(options); }

} // namespace model

} // namespace pluss::ai::lymph
