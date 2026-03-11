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
using pluss::ai::nodej::LinearFamilyOptions;
using pluss::ai::nodej::MetricKind;
using pluss::ai::nodej::ModelFamily;
using pluss::ai::nodej::ModelSpec;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::StatusCode;
using pluss::ai::nodej::TaskKind;
using pluss::ai::nodej::TreeCriterion;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-6) {
  return std::abs(a - b) <= tol;
}

bool run_nodej_bayes_kernel_checks(std::ostream* err = nullptr) {
  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.5, -1.0,
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
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .use_gaussian = true,
            .smoothing = 1e-9,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(spec);
    if (!cls_or.ok()) {
      if (err) *err << "make_classifier should succeed for bayes classifier\n";
      return false;
    }
    auto cls = cls_or.move_value();
    if (!cls->learn(batch).ok()) {
      if (err) *err << "bayes classifier learn should succeed\n";
      return false;
    }

    auto score_or = cls->infer_scores(features);
    if (!score_or.ok()) {
      if (err) *err << "bayes classifier infer_scores should succeed\n";
      return false;
    }
    const auto score_vec = score_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (!(score_vec.front() < 0.0 && score_vec.back() > 0.0)) {
      if (err) *err << "bayes classifier scores should separate classes by sign\n";
      return false;
    }

    auto pred_or = cls->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "bayes classifier infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i])) {
        if (err) *err << "bayes classifier should fit this separable training set\n";
        return false;
      }
    }

    pluss::ai::nodej::AssessOptions assess{};
    assess.metrics = {MetricKind::Accuracy, MetricKind::LogLoss};
    auto metrics_or = cls->assess(batch, assess);
    if (!metrics_or.ok()) {
      if (err) *err << "bayes classifier assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.size() != 2 || metrics[0] < 0.99 || metrics[1] > 0.1) {
      if (err) *err << "bayes classifier metrics should show strong fit and low log-loss\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -1.0, -1.0,
                                            -1.0,  1.0,
                                             1.0, -1.0,
                                             1.0,  1.0,
                                            -0.8, -0.8,
                                            -0.8,  0.8,
                                             0.8, -0.8,
                                             0.8,  0.8},
                                        {8, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0}, {8}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Rbf,
            .c = 50.0,
            .gamma = 2.0,
            .epsilon = 0.0,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(spec);
    if (!cls_or.ok()) {
      if (err) *err << "make_classifier should succeed for kernel classifier\n";
      return false;
    }
    auto cls = cls_or.move_value();
    if (!cls->learn(batch).ok()) {
      if (err) *err << "kernel classifier learn should succeed\n";
      return false;
    }

    auto pred_or = cls->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "kernel classifier infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i])) {
        if (err) *err << "kernel classifier should model this XOR pattern\n";
        return false;
      }
    }

    auto metrics_or = cls->assess(batch);
    if (!metrics_or.ok()) {
      if (err) *err << "kernel classifier assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.empty() || metrics[0] < 0.99) {
      if (err) *err << "kernel classifier default metric should indicate high accuracy\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -1.0,
                                            -1.0, -0.5,
                                             0.0,  0.0,
                                             1.0,  0.5,
                                             2.0,  1.0,
                                             3.0,  1.5},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{-1.0, -0.5, 0.0, 0.5, 1.0, 1.5}, {6}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Rbf,
            .c = 100.0,
            .gamma = 0.7,
            .epsilon = 0.0,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for kernel regressor\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "kernel regressor learn should succeed\n";
      return false;
    }

    auto pred_or = reg->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "kernel regressor infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i], 2e-2)) {
        if (err) *err << "kernel regressor should approximate training signal closely\n";
        return false;
      }
    }

    auto metrics_or = reg->assess(batch);
    if (!metrics_or.ok()) {
      if (err) *err << "kernel regressor assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.size() < 3 || metrics[0] > 1e-2 || metrics[2] < 0.98) {
      if (err) *err << "kernel regressor metrics should show low error and high R2\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {4, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec bad_bayes{
        .task = TaskKind::Classification,
        .family = ModelFamily::Bayes,
        .options = BayesFamilyOptions{
            .use_gaussian = false,
            .smoothing = 1e-9,
        },
    };
    auto bayes_or = pluss::ai::nodej::make_classifier(bad_bayes);
    if (!bayes_or.ok()) {
      if (err) *err << "builder should still construct bayes classifier\n";
      return false;
    }
    auto bayes = bayes_or.move_value();
    const auto bayes_status = bayes->learn(batch);
    if (bayes_status.ok() || bayes_status.code != StatusCode::NotImplemented) {
      if (err) *err << "bayes classifier should reject unsupported likelihood mode\n";
      return false;
    }

    const ModelSpec bad_kernel{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Rbf,
            .c = -1.0,
            .gamma = 0.5,
            .epsilon = 0.0,
        },
    };
    auto kernel_or = pluss::ai::nodej::make_regressor(bad_kernel);
    if (!kernel_or.ok()) {
      if (err) *err << "builder should still construct kernel regressor\n";
      return false;
    }
    auto kernel = kernel_or.move_value();
    const auto kernel_status = kernel->learn(batch);
    if (kernel_status.ok() || kernel_status.code != StatusCode::InvalidArgument) {
      if (err) *err << "kernel regressor should reject invalid c option\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej bayes and kernel families") {
  REQUIRE(run_nodej_bayes_kernel_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_bayes_kernel_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_bayes_kernel_test passed\n";
  return 0;
}
#endif
