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
using pluss::ai::nodej::EnsembleFamilyOptions;
using pluss::ai::nodej::EnsembleMethod;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::MetricKind;
using pluss::ai::nodej::ModelFamily;
using pluss::ai::nodej::ModelSpec;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::StatusCode;
using pluss::ai::nodej::TaskKind;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

bool run_nodej_ensemble_bayes_regression_checks(std::ostream* err = nullptr) {
  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.5, -1.0,
                                            -2.0, -2.0,
                                            -1.0, -1.5,
                                             1.0,  1.0,
                                             2.0,  1.0,
                                             1.0,  2.0,
                                             1.5,  1.0},
                                        {8, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64());
    const LabeledBatch batch{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    const ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::RandomForest,
            .learners = 21,
            .subsample = 1.0,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(spec);
    if (!cls_or.ok()) {
      if (err) *err << "make_classifier should succeed for ensemble classifier\n";
      return false;
    }
    auto cls = cls_or.move_value();
    if (!cls->learn(batch).ok()) {
      if (err) *err << "ensemble classifier learn should succeed\n";
      return false;
    }

    auto pred_or = cls->infer(batch.features);
    if (!pred_or.ok()) {
      if (err) *err << "ensemble classifier infer should succeed\n";
      return false;
    }

    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::size_t correct = 0;
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (approx(pred[i], truth[i])) ++correct;
    }
    const double accuracy = static_cast<double>(correct) / static_cast<double>(pred.size());
    if (accuracy < 0.95) {
      if (err) *err << "ensemble classifier should fit separable train set with high accuracy\n";
      return false;
    }

    auto metric_or = cls->assess(batch, pluss::ai::nodej::AssessOptions{.metrics = {MetricKind::Accuracy}});
    if (!metric_or.ok()) {
      if (err) *err << "ensemble classifier assess should succeed\n";
      return false;
    }
    const auto metric = metric_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metric.empty() || metric[0] < 0.95) {
      if (err) *err << "ensemble classifier assess should report high accuracy\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{-2.0, -1.5, -1.0, -0.5, 0.0, 0.5, 1.0, 1.5, 2.0}, {9, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{4.0, 2.25, 1.0, 0.25, 0.0, 0.25, 1.0, 2.25, 4.0}, {9}, f64());
    const LabeledBatch batch{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::RandomForest,
            .learners = 31,
            .subsample = 0.9,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for ensemble regressor\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "ensemble regressor learn should succeed\n";
      return false;
    }

    auto metric_or = reg->assess(batch, pluss::ai::nodej::AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
    if (!metric_or.ok()) {
      if (err) *err << "ensemble regressor assess should succeed\n";
      return false;
    }
    const auto metric = metric_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metric.size() != 2 || metric[0] > 0.5 || metric[1] < 0.7) {
      if (err) *err << "ensemble regressor should model smooth nonlinear train signal\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, 0.0,
                                            -1.0, 1.0,
                                             0.0, 0.0,
                                             1.0, 1.0,
                                             2.0, 0.0,
                                             3.0, 1.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{-3.0, -1.5, 1.0, 2.5, 5.0, 6.5}, {6}, f64());
    const LabeledBatch batch{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .use_gaussian = true,
            .smoothing = 1e-6,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for bayes regressor\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "bayes regressor learn should succeed\n";
      return false;
    }

    auto pred_or = reg->infer(batch.features);
    if (!pred_or.ok()) {
      if (err) *err << "bayes regressor infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i], 1e-3)) {
        if (err) *err << "bayes regressor should recover near-linear relationship on train set\n";
        return false;
      }
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {4, 1}, f64());
    const Series y_cls = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
    const Series y_reg = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {4}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};

    auto bad_cls = pluss::ai::nodej::make_classifier(ModelSpec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::GradientBoost,
            .learners = 5,
            .subsample = 1.0,
        },
    });
    if (!bad_cls.ok()) {
      if (err) *err << "builder should still construct ensemble classifier\n";
      return false;
    }
    const auto bad_cls_status = bad_cls.value()->learn(LabeledBatch{.features = features, .y = y_cls});
    if (bad_cls_status.ok() || bad_cls_status.code != StatusCode::NotImplemented) {
      if (err) *err << "ensemble classifier should reject unsupported gradient-boost mode for now\n";
      return false;
    }

    auto bad_reg = pluss::ai::nodej::make_regressor(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Ensemble,
        .options = EnsembleFamilyOptions{
            .method = EnsembleMethod::GradientBoost,
            .learners = 5,
            .subsample = 1.0,
        },
    });
    if (!bad_reg.ok()) {
      if (err) *err << "builder should still construct ensemble regressor\n";
      return false;
    }
    const auto bad_reg_status = bad_reg.value()->learn(LabeledBatch{.features = features, .y = y_reg});
    if (bad_reg_status.ok() || bad_reg_status.code != StatusCode::NotImplemented) {
      if (err) *err << "ensemble regressor should reject unsupported gradient-boost mode for now\n";
      return false;
    }

    auto bad_bayes = pluss::ai::nodej::make_regressor(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .use_gaussian = false,
            .smoothing = 1e-6,
        },
    });
    if (!bad_bayes.ok()) {
      if (err) *err << "builder should still construct bayes regressor\n";
      return false;
    }
    const auto bad_bayes_status = bad_bayes.value()->learn(LabeledBatch{.features = features, .y = y_reg});
    if (bad_bayes_status.ok() || bad_bayes_status.code != StatusCode::NotImplemented) {
      if (err) *err << "bayes regressor should reject unsupported non-gaussian mode\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej ensemble and bayes regression families") {
  REQUIRE(run_nodej_ensemble_bayes_regression_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_ensemble_bayes_regression_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_ensemble_bayes_regression_test passed\n";
  return 0;
}
#endif
