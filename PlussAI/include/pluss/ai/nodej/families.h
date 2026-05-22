#pragma once

#include "pluss/ai/nodej/options.h"

#include <cstdint>
#include <variant>

namespace pluss::ai::nodej {

// Phase-0 focus: families that unlock broad coverage quickly.
enum class ModelFamily : std::uint8_t {
  Linear = 0,
  Tree = 1,
  Ensemble = 2,
  Neighbor = 3,
  Kernel = 4,
  Bayes = 5,
  Cluster = 6,
  Decompose = 7,
};

enum class RegularizationKind : std::uint8_t {
  None = 0,
  L1 = 1,
  L2 = 2,
  Elastic = 3,
};

enum class LinearObjective : std::uint8_t {
  LeastSquares = 0,
  Logistic = 1,
  Hinge = 2,
  RobustLeast = 3,
};

enum class TreeCriterion : std::uint8_t {
  Gini = 0,
  Entropy = 1,
  Mse = 2,
  Mae = 3,
};

enum class EnsembleMethod : std::uint8_t {
  Bagging = 0,
  RandomForest = 1,
  GradientBoost = 2,
  AdaBoost = 3,
};

enum class NeighborQueryMode : std::uint8_t {
  KNearest = 0,
  Radius = 1,
};

enum class KernelKind : std::uint8_t {
  Linear = 0,
  Rbf = 1,
  Polynomial = 2,
  Sigmoid = 3,
};

enum class KernelForm : std::uint8_t {
  C = 0,
  Nu = 1,
  OneClass = 2,
};

enum class BayesLikelihood : std::uint8_t {
  Gaussian = 0,
  Bernoulli = 1,
  Multinomial = 2,
};

enum class ClusterMethod : std::uint8_t {
  KMeans = 0,
};

enum class DecomposeMethod : std::uint8_t {
  Pca = 0,
};

struct LinearFamilyOptions {
  LinearObjective objective = LinearObjective::LeastSquares;
  RegularizationKind regularization = RegularizationKind::L2;
  double regularization_strength = 1e-6;
  // Blend factor used when regularization == RegularizationKind::Elastic.
  // 1.0 -> pure L1, 0.0 -> pure L2.
  double elastic_l1_ratio = 0.5;
  // Transition point for LinearObjective::RobustLeast (Huber-style):
  // residuals with |r| <= robust_transition use quadratic loss, else linear.
  double robust_transition = 1.35;
  bool fit_intercept = true;
};

struct TreeFamilyOptions {
  TreeCriterion criterion = TreeCriterion::Gini;
  std::int64_t max_depth = -1;
  std::int64_t min_samples_leaf = 1;
  std::int64_t min_samples_split = 2;
};

struct EnsembleFamilyOptions {
  EnsembleMethod method = EnsembleMethod::RandomForest;
  std::int64_t learners = 100;
  double subsample = 1.0;
  double learning_rate = 0.1;
};

struct NeighborFamilyOptions {
  std::int64_t k = 5;
  NeighborQueryMode query_mode = NeighborQueryMode::KNearest;
  double radius = 1.0;
  // For radius mode only: when no neighbor is found, fallback to nearest k.
  std::int64_t radius_fallback_k = 1;
  bool distance_weighted = false;
  bool standardize = true;
};

struct KernelFamilyOptions {
  KernelKind kernel = KernelKind::Rbf;
  KernelForm form = KernelForm::C;
  double c = 1.0;
  double gamma = 0.0;
  double epsilon = 0.1;
  double nu = 0.5;
};

struct BayesFamilyOptions {
  // Preferred selector for Bayes family likelihood model.
  BayesLikelihood likelihood = BayesLikelihood::Gaussian;
  // Backward-compatibility shim: false maps to a discrete Bayes mode when
  // likelihood is left at Gaussian.
  bool use_gaussian = true;
  double smoothing = 1e-9;
};

struct ClusterFamilyOptions {
  ClusterMethod method = ClusterMethod::KMeans;
  std::int64_t clusters = 8;
  std::int64_t max_iterations = 300;
  std::int64_t restarts = 4;
  double tolerance = 1e-4;
};

struct DecomposeFamilyOptions {
  DecomposeMethod method = DecomposeMethod::Pca;
  std::int64_t components = -1;
  bool whiten = false;
  std::int64_t max_iterations = 500;
  double tolerance = 1e-6;
};

using FamilyOptions = std::variant<LinearFamilyOptions,
                                   TreeFamilyOptions,
                                   EnsembleFamilyOptions,
                                   NeighborFamilyOptions,
                                   KernelFamilyOptions,
                                   BayesFamilyOptions,
                                   ClusterFamilyOptions,
                                   DecomposeFamilyOptions>;

struct ModelSpec {
  TaskKind task = TaskKind::Regression;
  ModelFamily family = ModelFamily::Linear;
  FamilyOptions options = LinearFamilyOptions{};
};

} // namespace pluss::ai::nodej
