#include <pluss/lymph.h>

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
using pluss::ai::nodej::LinearFamilyOptions;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::TrainOptions;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

bool run_lymph_quick_checks(std::ostream* err = nullptr) {
  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.0, -0.5,
                                             1.0,  0.5,
                                             2.0,  1.0},
                                        {4, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    TrainOptions options{};
    options.max_iterations = 4000;
    options.tolerance = 1e-8;
    auto model_or = pluss::ai::lymph::linear_classifier(train, LinearFamilyOptions{}, options);
    if (!model_or.ok()) {
      if (err) *err << "lymph::linear_classifier should return learned model\n";
      return false;
    }

    auto pred_or = model_or.value()->infer(train.features);
    if (!pred_or.ok()) {
      if (err) *err << "lymph linear classifier model should infer\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i])) {
        if (err) *err << "lymph linear classifier should fit separable data\n";
        return false;
      }
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.0, -0.5,
                                             1.0,  0.5,
                                             2.0,  1.0},
                                        {4, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto logistic_or = pluss::ai::lymph::linear_logistic_classifier(train, 1e-6);
    if (!logistic_or.ok()) {
      if (err) *err << "lymph::linear_logistic_classifier should return learned model\n";
      return false;
    }
    auto hinge_or = pluss::ai::lymph::linear_hinge_classifier(train, 1e-6);
    if (!hinge_or.ok()) {
      if (err) *err << "lymph::linear_hinge_classifier should return learned model\n";
      return false;
    }

    auto logistic_pred_or = logistic_or.value()->infer(train.features);
    auto hinge_pred_or = hinge_or.value()->infer(train.features);
    if (!logistic_pred_or.ok() || !hinge_pred_or.ok()) {
      if (err) *err << "lymph linear preset models should infer\n";
      return false;
    }

    const auto logistic_pred = logistic_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto hinge_pred = hinge_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < truth.size(); ++i) {
      if (!approx(logistic_pred[i], truth[i]) || !approx(hinge_pred[i], truth[i])) {
        if (err) *err << "lymph linear preset classifiers should fit this separable dataset\n";
        return false;
      }
    }
  }

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
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto cls_or = pluss::ai::lymph::forest_classifier(train, 21, 1.0);
    if (!cls_or.ok()) {
      if (err) *err << "lymph::forest_classifier should return learned model\n";
      return false;
    }

    auto metrics_or = cls_or.value()->assess(train);
    if (!metrics_or.ok()) {
      if (err) *err << "lymph forest classifier assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.empty() || metrics[0] < 0.95) {
      if (err) *err << "lymph forest classifier should produce high train accuracy\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0, 3.0}, {6, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0, 3.0}, {6}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto reg_or = pluss::ai::lymph::rbf_kernel_regressor(train, 50.0, 0.7, 0.0);
    if (!reg_or.ok()) {
      if (err) *err << "lymph::rbf_kernel_regressor should return learned model\n";
      return false;
    }

    auto pred_or = reg_or.value()->infer(train.features);
    if (!pred_or.ok()) {
      if (err) *err << "lymph kernel regressor model should infer\n";
      return false;
    }

    auto metrics_or = reg_or.value()->assess(train);
    if (!metrics_or.ok()) {
      if (err) *err << "lymph kernel regressor assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.size() < 3 || metrics[2] < 0.9) {
      if (err) *err << "lymph kernel regressor should produce strong fit quality\n";
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
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto ridge_or = pluss::ai::lymph::ridge_regressor(train, 1e-8);
    auto bayes_or = pluss::ai::lymph::gaussian_bayes_regressor(train, 1e-6);
    if (!ridge_or.ok() || !bayes_or.ok()) {
      if (err) *err << "lymph regression presets should return learned models\n";
      return false;
    }

    auto ridge_metrics_or = ridge_or.value()->assess(train);
    auto bayes_pred_or = bayes_or.value()->infer(train.features);
    if (!ridge_metrics_or.ok() || !bayes_pred_or.ok()) {
      if (err) *err << "lymph regression presets should assess/infer\n";
      return false;
    }

    const auto ridge_metrics = ridge_metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (ridge_metrics.size() < 3 || ridge_metrics[2] < 0.95) {
      if (err) *err << "lymph ridge preset should produce high training R2\n";
      return false;
    }

    const auto pred = bayes_pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i], 1e-3)) {
        if (err) *err << "lymph gaussian bayes regressor should recover near-linear training relation\n";
        return false;
      }
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph quick wrappers") {
  REQUIRE(run_lymph_quick_checks(&std::cerr));
}
#else
int main() {
  if (!run_lymph_quick_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph_quick_test passed\n";
  return 0;
}
#endif
