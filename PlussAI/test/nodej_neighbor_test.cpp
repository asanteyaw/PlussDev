#include <pluss/ai/ml.h>
#include <pluss/ai/lymph.h>

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
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::ModelFamily;
using pluss::ai::nodej::ModelSpec;
using pluss::ai::nodej::NeighborFamilyOptions;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::StatusCode;
using pluss::ai::nodej::TaskKind;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

bool run_nodej_neighbor_checks(std::ostream* err = nullptr) {
  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.0, -1.0,
                                            -2.0, -2.0,
                                             1.0,  1.0,
                                             2.0,  1.0,
                                             1.0,  2.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());

    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Neighbor,
        .options = NeighborFamilyOptions{
            .k = 3,
            .distance_weighted = true,
            .standardize = true,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(spec);
    if (!cls_or.ok()) {
      if (err) *err << "make_classifier should succeed for classification/neighbor\n";
      return false;
    }
    auto cls = cls_or.move_value();
    if (!cls->learn(batch).ok()) {
      if (err) *err << "neighbor classifier learn should succeed\n";
      return false;
    }

    auto score_or = cls->infer_scores(features);
    if (!score_or.ok()) {
      if (err) *err << "neighbor classifier infer_scores should succeed\n";
      return false;
    }
    const auto scores = score_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (!(scores.front() < 0.2 && scores.back() > 0.8)) {
      if (err) *err << "neighbor classifier scores should reflect neighborhood class density\n";
      return false;
    }

    auto pred_or = cls->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "neighbor classifier infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (pred.size() != truth.size()) {
      if (err) *err << "neighbor classifier predictions should match label length\n";
      return false;
    }
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i])) {
        if (err) *err << "neighbor classifier should fit this separable training set\n";
        return false;
      }
    }

    auto metrics_or = cls->assess(batch);
    if (!metrics_or.ok()) {
      if (err) *err << "neighbor classifier assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (metrics.empty() || metrics[0] < 0.99) {
      if (err) *err << "neighbor classifier default assess metric should be high accuracy\n";
      return false;
    }

    const ModelSpec radius_spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Neighbor,
        .options = NeighborFamilyOptions{
            .k = 3,
            .query_mode = pluss::ai::nodej::NeighborQueryMode::Radius,
            .radius = 0.05,
            .radius_fallback_k = 1,
            .distance_weighted = true,
            .standardize = true,
        },
    };
    auto rcls_or = pluss::ai::nodej::make_classifier(radius_spec);
    if (!rcls_or.ok()) {
      if (err) *err << "make_classifier should succeed for radius neighbor classifier\n";
      return false;
    }
    auto rcls = rcls_or.move_value();
    if (!rcls->learn(batch).ok()) {
      if (err) *err << "radius neighbor classifier learn should succeed\n";
      return false;
    }
    auto rpred_or = rcls->infer(features);
    if (!rpred_or.ok()) {
      if (err) *err << "radius neighbor classifier infer should succeed\n";
      return false;
    }
    const auto rpred = rpred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (std::size_t i = 0; i < rpred.size(); ++i) {
      if (!approx(rpred[i], truth[i])) {
        if (err) *err << "radius neighbor classifier should fit training labels with fallback\n";
        return false;
      }
    }

    auto lymph_radius_cls = pluss::ai::lymph::models::neighbor::radius_neighbors_classifier(NeighborFamilyOptions{
        .k = 3,
        .radius = 0.05,
        .radius_fallback_k = 1,
        .distance_weighted = true,
        .standardize = true,
    }).learn(batch);
    const auto lymph_radius_pred = lymph_radius_cls.infer(features).to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (std::size_t i = 0; i < lymph_radius_pred.size(); ++i) {
      if (!approx(lymph_radius_pred[i], truth[i])) {
        if (err) *err << "lymph radius_neighbors_classifier should train and infer through NodeJ radius mode\n";
        return false;
      }
    }

    const Series x_bad = pluss::make_series(std::vector<double>{
                                                1.0, 2.0, 3.0,
                                                4.0, 5.0, 6.0},
                                            {2, 3},
                                            f64());
    const FeatureBatch bad_features{.x = x_bad, .layout = ObservationLayout::Rows};
    auto bad_or = cls->infer(bad_features);
    if (bad_or.ok() || bad_or.status().code != StatusCode::ShapeMismatch) {
      if (err) *err << "neighbor classifier should enforce learn-time feature count\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0}, {5, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 1.0, 4.0, 9.0, 16.0}, {5}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec reg_spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Neighbor,
        .options = NeighborFamilyOptions{
            .k = 1,
            .distance_weighted = false,
            .standardize = true,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(reg_spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for regression/neighbor\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "neighbor regressor learn should succeed\n";
      return false;
    }

    auto pred_or = reg->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "neighbor regressor infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i], 1e-8)) {
        if (err) *err << "neighbor regressor with k=1 should reproduce training targets\n";
        return false;
      }
    }

    auto metrics_or = reg->assess(batch);
    if (!metrics_or.ok()) {
      if (err) *err << "neighbor regressor assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (metrics.size() < 3 || metrics[0] > 1e-12 || metrics[2] < 0.999999) {
      if (err) *err << "neighbor regressor default metrics should indicate perfect fit for k=1\n";
      return false;
    }

    const ModelSpec radius_reg_spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Neighbor,
        .options = NeighborFamilyOptions{
            .k = 1,
            .query_mode = pluss::ai::nodej::NeighborQueryMode::Radius,
            .radius = 0.01,
            .radius_fallback_k = 1,
            .distance_weighted = false,
            .standardize = true,
        },
    };
    auto rreg_or = pluss::ai::nodej::make_regressor(radius_reg_spec);
    if (!rreg_or.ok()) {
      if (err) *err << "make_regressor should succeed for radius neighbor regressor\n";
      return false;
    }
    auto rreg = rreg_or.move_value();
    if (!rreg->learn(batch).ok()) {
      if (err) *err << "radius neighbor regressor learn should succeed\n";
      return false;
    }
    auto rreg_pred_or = rreg->infer(features);
    if (!rreg_pred_or.ok()) {
      if (err) *err << "radius neighbor regressor infer should succeed\n";
      return false;
    }
    const auto rreg_pred = rreg_pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (std::size_t i = 0; i < rreg_pred.size(); ++i) {
      if (!approx(rreg_pred[i], truth[i], 1e-8)) {
        if (err) *err << "radius neighbor regressor should fit training targets with fallback\n";
        return false;
      }
    }

    auto lymph_radius_reg = pluss::ai::lymph::models::neighbor::radius_neighbors_regressor(NeighborFamilyOptions{
        .k = 1,
        .radius = 0.01,
        .radius_fallback_k = 1,
        .distance_weighted = false,
        .standardize = true,
    }).learn(batch);
    const auto lymph_radius_reg_pred = lymph_radius_reg.infer(features).to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (std::size_t i = 0; i < lymph_radius_reg_pred.size(); ++i) {
      if (!approx(lymph_radius_reg_pred[i], truth[i], 1e-8)) {
        if (err) *err << "lymph radius_neighbors_regressor should train and infer through NodeJ radius mode\n";
        return false;
      }
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej neighbor family estimators") {
  REQUIRE(run_nodej_neighbor_checks());
}
#else
int main() {
  if (!run_nodej_neighbor_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_neighbor_test passed\n";
  return 0;
}
#endif
