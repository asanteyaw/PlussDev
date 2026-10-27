#include <pluss/ai/ml.h>

#include <cmath>
#include <cstdint>
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
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::StatusCode;
using pluss::ai::nodej::TaskKind;
using pluss::ai::nodej::TreeCriterion;
using pluss::ai::nodej::TreeFamilyOptions;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool approx(double a, double b, double tol = 1e-7) {
  return std::abs(a - b) <= tol;
}

bool run_nodej_tree_checks(std::ostream* err = nullptr) {
  {
    // Axis-aligned L-shape pattern: requires multi-level splits and should be exactly learnable.
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, -2.0,
                                            -2.0,  2.0,
                                            2.0, -2.0,
                                            2.0,  2.0,
                                             3.0, -1.0,
                                             3.0,  1.0,
                                             1.0, -3.0,
                                             1.0,  3.0},
                                        {8, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0}, {8}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Tree,
        .options = TreeFamilyOptions{
            .criterion = TreeCriterion::Gini,
            .max_depth = 3,
            .min_samples_leaf = 1,
            .min_samples_split = 2,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(spec);
    if (!cls_or.ok()) {
      if (err) *err << "make_classifier should succeed for classification/tree\n";
      return false;
    }
    auto cls = cls_or.move_value();
    if (!cls->learn(batch).ok()) {
      if (err) *err << "tree classifier learn should succeed\n";
      return false;
    }

    auto score_or = cls->infer_scores(features);
    if (!score_or.ok()) {
      if (err) *err << "tree classifier infer_scores should succeed\n";
      return false;
    }
    const auto scores = score_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (scores.size() != 8) {
      if (err) *err << "tree classifier score length mismatch\n";
      return false;
    }
    for (double s : scores) {
      if (!std::isfinite(s) || s < 0.0 || s > 1.0) {
        if (err) *err << "tree classifier scores should be finite probabilities\n";
        return false;
      }
    }

    auto pred_or = cls->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "tree classifier infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i], 1e-8)) {
        if (err) *err << "tree classifier should fit this multi-level separable training set\n";
        return false;
      }
    }

    auto metrics_or = cls->assess(batch);
    if (!metrics_or.ok()) {
      if (err) *err << "tree classifier assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (metrics.empty() || metrics[0] < 0.99) {
      if (err) *err << "tree classifier default assess metric should be high accuracy\n";
      return false;
    }
  }

  {
    // Constant feature edge case: tree should stay at a single majority-class leaf.
    const Series x = pluss::series::ones({5, 2}, f64());
    const Series y = pluss::make_series(std::vector<double>{1.0, 1.0, 1.0, 1.0, 0.0}, {5}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Tree,
        .options = TreeFamilyOptions{
            .criterion = TreeCriterion::Entropy,
            .max_depth = -1,
            .min_samples_leaf = 1,
            .min_samples_split = 2,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(spec);
    if (!cls_or.ok()) {
      if (err) *err << "make_classifier should succeed for entropy tree classifier\n";
      return false;
    }
    auto cls = cls_or.move_value();
    if (!cls->learn(batch).ok()) {
      if (err) *err << "entropy tree classifier learn should succeed\n";
      return false;
    }
    auto pred_or = cls->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "entropy tree classifier infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (double p : pred) {
      if (!approx(p, 1.0, 1e-8)) {
        if (err) *err << "constant-feature tree should predict majority class\n";
        return false;
      }
    }

    const Series x_bad = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {2, 3}, f64());
    const FeatureBatch bad_features{.x = x_bad, .layout = ObservationLayout::Rows};
    auto bad_or = cls->infer(bad_features);
    if (bad_or.ok() || bad_or.status().code != StatusCode::ShapeMismatch) {
      if (err) *err << "tree classifier should enforce learn-time feature count\n";
      return false;
    }
  }

  {
    // Unsupported classifier split criterion should fail fast.
    const Series x = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0}, {4, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 1.0, 1.0}, {4}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec bad_spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Tree,
        .options = TreeFamilyOptions{
            .criterion = TreeCriterion::Mse,
            .max_depth = -1,
            .min_samples_leaf = 1,
            .min_samples_split = 2,
        },
    };
    auto cls_or = pluss::ai::nodej::make_classifier(bad_spec);
    if (!cls_or.ok()) {
      if (err) *err << "builder should still construct tree classifier with options\n";
      return false;
    }
    auto cls = cls_or.move_value();
    const auto status = cls->learn(batch);
    if (status.ok() || status.code != StatusCode::NotImplemented) {
      if (err) *err << "tree classifier should reject unsupported criterion with NotImplemented\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0}, {8, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0, 6.0, 6.0, 6.0, 6.0}, {8}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Tree,
        .options = TreeFamilyOptions{
            .criterion = TreeCriterion::Mse,
            .max_depth = 2,
            .min_samples_leaf = 1,
            .min_samples_split = 2,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for regression/tree\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "tree regressor learn should succeed\n";
      return false;
    }

    auto pred_or = reg->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "tree regressor infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    const auto truth = y.to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (std::size_t i = 0; i < pred.size(); ++i) {
      if (!approx(pred[i], truth[i], 1e-8)) {
        if (err) *err << "tree regressor should fit this step signal\n";
        return false;
      }
    }

    auto metrics_or = reg->assess(batch);
    if (!metrics_or.ok()) {
      if (err) *err << "tree regressor assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (metrics.size() < 3 || metrics[0] > 1e-12 || metrics[2] < 0.999999) {
      if (err) *err << "tree regressor metrics should indicate near-perfect fit\n";
      return false;
    }
  }

  {
    // depth=0 should force a stump-like constant predictor at the global mean.
    const Series x = pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{1.0, 3.0, 5.0, 7.0}, {4}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Tree,
        .options = TreeFamilyOptions{
            .criterion = TreeCriterion::Mse,
            .max_depth = 0,
            .min_samples_leaf = 1,
            .min_samples_split = 2,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for depth-limited tree\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "depth-limited tree regressor learn should succeed\n";
      return false;
    }
    auto pred_or = reg->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "depth-limited tree regressor infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (double v : pred) {
      if (!approx(v, 4.0, 1e-8)) {
        if (err) *err << "depth-0 regressor should output global mean response\n";
        return false;
      }
    }
  }

  {
    // With MAE criterion, a depth-0 leaf should predict the median (robust to outliers).
    const Series x = pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}, {4, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{1.0, 3.0, 5.0, 100.0}, {4}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Tree,
        .options = TreeFamilyOptions{
            .criterion = TreeCriterion::Mae,
            .max_depth = 0,
            .min_samples_leaf = 1,
            .min_samples_split = 2,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for MAE tree\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "MAE tree regressor learn should succeed\n";
      return false;
    }
    auto pred_or = reg->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "MAE tree regressor infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    for (double v : pred) {
      if (!approx(v, 4.0, 1e-8)) {
        if (err) *err << "depth-0 MAE regressor should output global median response\n";
        return false;
      }
    }
  }

  {
    // Stress case: larger synthetic regression batch should not crash or emit NaN/Inf.
    constexpr std::int64_t n = 1024;
    constexpr std::int64_t p = 6;
    std::vector<double> x_vals{};
    std::vector<double> y_vals{};
    x_vals.reserve(static_cast<std::size_t>(n * p));
    y_vals.reserve(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i) {
      const double t = static_cast<double>(i) / static_cast<double>(n);
      const double x0 = std::sin(20.0 * t);
      const double x1 = std::cos(11.0 * t);
      const double x2 = (static_cast<double>(i % 17) - 8.0) * 1.0e6;
      const double x3 = (i % 3 == 0) ? -1.0 : 1.0;
      const double x4 = static_cast<double>((i * 31) % 19) - 9.0;
      const double x5 = static_cast<double>((i * 7) % 5);
      x_vals.insert(x_vals.end(), {x0, x1, x2, x3, x4, x5});
      y_vals.push_back((x0 > 0.0 ? 2.0 : -2.0) + 0.3 * x1 + 0.1 * x3);
    }
    const Series x = pluss::make_series(x_vals, {n, p}, f64());
    const Series y = pluss::make_series(y_vals, {n}, f64());
    const FeatureBatch features{.x = x, .layout = ObservationLayout::Rows};
    const LabeledBatch batch{.features = features, .y = y};

    const ModelSpec spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Tree,
        .options = TreeFamilyOptions{
            .criterion = TreeCriterion::Mse,
            .max_depth = 8,
            .min_samples_leaf = 4,
            .min_samples_split = 8,
        },
    };
    auto reg_or = pluss::ai::nodej::make_regressor(spec);
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for stress tree regression\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch).ok()) {
      if (err) *err << "stress tree regressor learn should succeed\n";
      return false;
    }
    auto pred_or = reg->infer(features);
    if (!pred_or.ok()) {
      if (err) *err << "stress tree regressor infer should succeed\n";
      return false;
    }
    const auto pred = pred_or.value().to(pluss::pCPU, ScalarType::Float64).host<double>();
    if (pred.size() != static_cast<std::size_t>(n)) {
      if (err) *err << "stress tree regressor output length mismatch\n";
      return false;
    }
    for (double v : pred) {
      if (!std::isfinite(v)) {
        if (err) *err << "stress tree regressor predictions should stay finite\n";
        return false;
      }
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej tree family estimators") {
  REQUIRE(run_nodej_tree_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_tree_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_tree_test passed\n";
  return 0;
}
#endif
