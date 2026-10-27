#include <pluss/ai/ml.h>

#include <cmath>
#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::ai::nodej::BayesFamilyOptions;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::KernelFamilyOptions;
using pluss::ai::nodej::KernelKind;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::ModelFamily;
using pluss::ai::nodej::ModelSpec;
using pluss::ai::nodej::NeighborFamilyOptions;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::TaskKind;
using pluss::ai::nodej::TreeCriterion;
using pluss::ai::nodej::TreeFamilyOptions;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool weighted_neighbor_votes_flip(std::ostream* err) {
  const Series x = pluss::make_series(std::vector<double>{-1.0, 0.0, 1.0}, {3, 1}, f64());
  const Series y = pluss::make_series(std::vector<double>{0.0, 1.0, 1.0}, {3}, f64());
  const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
  const LabeledBatch batch{
      .features = features,
      .y = y,
      .sample_weights = pluss::make_series(std::vector<double>{10.0, 1.0, 1.0}, {3}, f64()),
  };
  const ModelSpec spec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Neighbor,
      .options = NeighborFamilyOptions{.k = 3, .distance_weighted = false, .standardize = false},
  };
  auto cls_or = pluss::ai::nodej::make_classifier(spec);
  if (!cls_or.ok()) return false;
  auto cls = cls_or.move_value();
  if (!cls->learn(batch).ok()) return false;
  auto scores_or = cls->infer_scores(features);
  if (!scores_or.ok()) return false;
  const auto scores = scores_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
  if (!(scores.size() == 3 && scores[1] < 0.25)) {
    if (err) *err << "weighted neighbor vote should downweight the positive majority near the center\n";
    return false;
  }
  return true;
}

bool weighted_tree_leaf_flips(std::ostream* err) {
  const Series x = pluss::series::ones({3, 1}, f64());
  const Series y = pluss::make_series(std::vector<double>{0.0, 1.0, 1.0}, {3}, f64());
  const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
  const LabeledBatch batch{
      .features = features,
      .y = y,
      .sample_weights = pluss::make_series(std::vector<double>{10.0, 1.0, 1.0}, {3}, f64()),
  };
  const ModelSpec spec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Tree,
      .options = TreeFamilyOptions{.criterion = TreeCriterion::Gini, .max_depth = 2},
  };
  auto cls_or = pluss::ai::nodej::make_classifier(spec);
  if (!cls_or.ok()) return false;
  auto cls = cls_or.move_value();
  if (!cls->learn(batch).ok()) return false;
  auto scores_or = cls->infer_scores(features);
  if (!scores_or.ok()) return false;
  const auto scores = scores_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
  if (!(scores.size() == 3 && scores[0] < 0.25)) {
    if (err) *err << "weighted tree leaf probability should follow weighted class mass\n";
    return false;
  }
  return true;
}

bool weighted_bayes_prior_flips(std::ostream* err) {
  const Series x = pluss::series::zeros({2, 1}, f64());
  const Series y = pluss::make_series(std::vector<double>{0.0, 1.0}, {2}, f64());
  const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
  const LabeledBatch batch{
      .features = features,
      .y = y,
      .sample_weights = pluss::make_series(std::vector<double>{1.0, 9.0}, {2}, f64()),
  };
  const ModelSpec spec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Bayes,
      .options = BayesFamilyOptions{.smoothing = 1e-6},
  };
  auto cls_or = pluss::ai::nodej::make_classifier(spec);
  if (!cls_or.ok()) return false;
  auto cls = cls_or.move_value();
  if (!cls->learn(batch).ok()) return false;
  auto scores_or = cls->infer_scores(features);
  if (!scores_or.ok()) return false;
  const auto scores = scores_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
  if (!(scores.size() == 2 && scores[0] > 1.0)) {
    if (err) *err << "weighted Bayes prior should favor the higher-weight class\n";
    return false;
  }
  return true;
}

bool weighted_kernel_solution_flips(std::ostream* err) {
  const Series x = pluss::series::zeros({2, 1}, f64());
  const Series y = pluss::make_series(std::vector<double>{0.0, 1.0}, {2}, f64());
  const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
  const LabeledBatch batch{
      .features = features,
      .y = y,
      .sample_weights = pluss::make_series(std::vector<double>{1.0, 9.0}, {2}, f64()),
  };
  const ModelSpec spec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Kernel,
      .options = KernelFamilyOptions{.kernel = KernelKind::Rbf, .c = 10.0},
  };
  auto cls_or = pluss::ai::nodej::make_classifier(spec);
  if (!cls_or.ok()) return false;
  auto cls = cls_or.move_value();
  pluss::ai::nodej::TrainOptions train{};
  train.max_iterations = 200;
  train.tolerance = 1e-8;
  if (!cls->learn(batch, train).ok()) return false;
  auto scores_or = cls->infer_scores(features);
  if (!scores_or.ok()) return false;
  const auto scores = scores_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
  if (!(scores.size() == 2 && scores[0] > 0.0)) {
    if (err) *err << "weighted kernel solution should favor the higher-weight class\n";
    return false;
  }
  return true;
}

bool run_nodej_weighted_family_checks(std::ostream* err = nullptr) {
  return weighted_neighbor_votes_flip(err)
      && weighted_tree_leaf_flips(err)
      && weighted_bayes_prior_flips(err)
      && weighted_kernel_solution_flips(err);
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej classifier families consume centralized training weights", "[nodej][weights]") {
  REQUIRE(run_nodej_weighted_family_checks());
}
#else
int main() {
  if (!run_nodej_weighted_family_checks(&std::cerr)) return 1;
  return 0;
}
#endif
