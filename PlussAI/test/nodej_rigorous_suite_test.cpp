#include <pluss/ai/ml.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <unordered_set>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

std::vector<double> as_f64_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
}

std::vector<std::int64_t> as_i64_vec(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
}

bool has_duplicates(const std::vector<std::int64_t>& indices) {
  std::unordered_set<std::int64_t> seen;
  seen.reserve(indices.size());
  for (std::int64_t idx : indices) {
    if (!seen.insert(idx).second) return true;
  }
  return false;
}

bool run_nodej_rigorous_checks(std::ostream* err = nullptr) {
  {
    const std::int64_t n = 512;
    std::vector<double> xv{};
    std::vector<double> yv{};
    xv.reserve(static_cast<std::size_t>(n * 3));
    yv.reserve(static_cast<std::size_t>(n));

    for (std::int64_t i = 0; i < n; ++i) {
      const double x0 = -4.0 + 8.0 * static_cast<double>(i) / static_cast<double>(n - 1);
      const double x1 = 0.8 * std::sin(1.7 * x0);
      const double x2 = 0.6 * x0 + 0.2 * x1;
      const double margin = x0 + 0.3 * x1 - 0.1 * x2;
      const double y = margin >= 0.0 ? 1.0 : 0.0;
      xv.push_back(x0);
      xv.push_back(x1);
      xv.push_back(x2);
      yv.push_back(y);
    }

    const Series x = pluss::make_series(std::move(xv), {n, 3}, f64());
    const Series y = pluss::make_series(std::move(yv), {n}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    TrainOptions train_options{};
    train_options.max_iterations = 6000;
    train_options.tolerance = 1e-8;

    const ModelSpec logistic_spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::Logistic,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-6,
            .fit_intercept = true,
        },
    };
    auto logistic_or = make_classifier(logistic_spec);
    if (!logistic_or.ok()) {
      if (err) *err << "rigorous: make_classifier logistic should succeed\n";
      return false;
    }
    auto logistic = logistic_or.move_value();
    if (!logistic->learn(train, train_options).ok()) {
      if (err) *err << "rigorous: logistic learn should succeed\n";
      return false;
    }
    auto score_or = logistic->infer_scores(train.features);
    auto metric_or = logistic->assess(train);
    if (!score_or.ok() || !metric_or.ok()) {
      if (err) *err << "rigorous: logistic infer_scores/assess should succeed\n";
      return false;
    }
    if (!score_or.value().isfinite().all().item<bool>()) {
      if (err) *err << "rigorous: logistic scores should remain finite\n";
      return false;
    }
    const auto logistic_metrics = as_f64_vec(metric_or.value());
    if (logistic_metrics.empty() || logistic_metrics[0] < 0.97) {
      if (err) *err << "rigorous: logistic should achieve high accuracy on separable data\n";
      return false;
    }

    const ModelSpec hinge_spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::Hinge,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-6,
            .fit_intercept = true,
        },
    };
    auto hinge_or = make_classifier(hinge_spec);
    if (!hinge_or.ok()) {
      if (err) *err << "rigorous: make_classifier hinge should succeed\n";
      return false;
    }
    auto hinge = hinge_or.move_value();
    if (!hinge->learn(train, train_options).ok()) {
      if (err) *err << "rigorous: hinge learn should succeed\n";
      return false;
    }
    auto hinge_score_or = hinge->infer_scores(train.features);
    auto hinge_metric_or = hinge->assess(train);
    if (!hinge_score_or.ok() || !hinge_metric_or.ok()) {
      if (err) *err << "rigorous: hinge infer_scores/assess should succeed\n";
      return false;
    }
    if (!hinge_score_or.value().isfinite().all().item<bool>()) {
      if (err) *err << "rigorous: hinge scores should remain finite\n";
      return false;
    }
    const auto hinge_metrics = as_f64_vec(hinge_metric_or.value());
    if (hinge_metrics.empty() || hinge_metrics[0] < 0.97) {
      if (err) *err << "rigorous: hinge should achieve high accuracy on separable data\n";
      return false;
    }
  }

  {
    std::mt19937_64 rng(17);
    std::uniform_real_distribution<double> dist(-2.0, 2.0);
    for (int trial = 0; trial < 8; ++trial) {
      std::vector<double> xv{};
      std::vector<double> yv{};
      xv.reserve(256 * 2);
      yv.reserve(256);
      for (int i = 0; i < 256; ++i) {
        const double x0 = dist(rng);
        const double x1 = dist(rng);
        const double y = (x0 - 0.7 * x1 >= 0.0) ? 1.0 : 0.0;
        xv.push_back(x0);
        xv.push_back(x1);
        yv.push_back(y);
      }

      const Series x = pluss::make_series(std::move(xv), {256, 2}, f64());
      const Series y = pluss::make_series(std::move(yv), {256}, f64());
      const LabeledBatch train{
          .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
          .y = y,
      };

      TrainOptions options{};
      options.max_iterations = 1500;
      options.tolerance = 1e-7;

      auto cls_or = make_classifier(ModelSpec{
          .task = TaskKind::Classification,
          .family = ModelFamily::Linear,
          .options = LinearFamilyOptions{
              .objective = LinearObjective::Logistic,
              .regularization = RegularizationKind::L2,
              .regularization_strength = 1e-5,
              .fit_intercept = true,
          },
      });
      if (!cls_or.ok()) {
        if (err) *err << "rigorous: randomized logistic make_classifier should succeed\n";
        return false;
      }
      auto cls = cls_or.move_value();
      if (!cls->learn(train, options).ok()) {
        if (err) *err << "rigorous: randomized logistic learn should succeed\n";
        return false;
      }
      auto score_or = cls->infer_scores(train.features);
      if (!score_or.ok() || !score_or.value().isfinite().all().item<bool>()) {
        if (err) *err << "rigorous: randomized logistic scores should be finite\n";
        return false;
      }
    }
  }

  {
    const std::int64_t n = 320;
    std::vector<double> xv{};
    std::vector<double> yv{};
    xv.reserve(static_cast<std::size_t>(n * 3));
    yv.reserve(static_cast<std::size_t>(n));

    for (std::int64_t i = 0; i < n; ++i) {
      const double x0 = -3.0 + 6.0 * static_cast<double>(i) / static_cast<double>(n - 1);
      const double x1 = std::sin(0.7 * x0);
      const double x2 = std::cos(1.1 * x0);
      const double y = 1.5 * x0 - 0.7 * x1 + 0.2 * x2 + 0.5;
      xv.push_back(x0);
      xv.push_back(x1);
      xv.push_back(x2);
      yv.push_back(y);
    }

    const Series x = pluss::make_series(std::move(xv), {n, 3}, f64());
    const Series y = pluss::make_series(std::move(yv), {n}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto ridge_or = make_regressor(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-9,
            .fit_intercept = true,
        },
    });
    if (!ridge_or.ok()) {
      if (err) *err << "rigorous: make_regressor ridge should succeed\n";
      return false;
    }
    auto ridge = ridge_or.move_value();
    if (!ridge->learn(train).ok()) {
      if (err) *err << "rigorous: ridge learn should succeed\n";
      return false;
    }
    auto ridge_pred_or = ridge->infer(train.features);
    auto ridge_metric_or = ridge->assess(train);
    if (!ridge_pred_or.ok() || !ridge_metric_or.ok()) {
      if (err) *err << "rigorous: ridge infer/assess should succeed\n";
      return false;
    }
    if (!ridge_pred_or.value().isfinite().all().item<bool>()) {
      if (err) *err << "rigorous: ridge predictions should be finite\n";
      return false;
    }
    const auto ridge_metrics = as_f64_vec(ridge_metric_or.value());
    if (ridge_metrics.size() < 3 || ridge_metrics[2] < 0.995) {
      if (err) *err << "rigorous: ridge should produce very high R2 on clean linear signal\n";
      return false;
    }

    auto kernel_or = make_regressor(ModelSpec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Kernel,
        .options = KernelFamilyOptions{
            .kernel = KernelKind::Rbf,
            .c = 100.0,
            .gamma = 0.4,
            .epsilon = 0.0,
        },
    });
    if (!kernel_or.ok()) {
      if (err) *err << "rigorous: make_regressor kernel should succeed\n";
      return false;
    }
    auto kernel = kernel_or.move_value();
    if (!kernel->learn(train).ok()) {
      if (err) *err << "rigorous: kernel learn should succeed\n";
      return false;
    }
    auto kernel_pred_or = kernel->infer(train.features);
    auto kernel_metric_or = kernel->assess(train);
    if (!kernel_pred_or.ok() || !kernel_metric_or.ok()) {
      if (err) *err << "rigorous: kernel infer/assess should succeed\n";
      return false;
    }
    if (!kernel_pred_or.value().isfinite().all().item<bool>()) {
      if (err) *err << "rigorous: kernel predictions should be finite\n";
      return false;
    }
    const auto kernel_metrics = as_f64_vec(kernel_metric_or.value());
    if (kernel_metrics.size() < 3 || kernel_metrics[2] < 0.98) {
      if (err) *err << "rigorous: kernel regressor should produce strong R2 on smooth signal\n";
      return false;
    }
  }

  {
    SplitOptions options{};
    options.holdout_fraction = 0.2;
    options.shuffle = true;
    options.random.seed = 404;
    options.random.deterministic = true;

    auto split_a = holdout_split(10000, options);
    auto split_b = holdout_split(10000, options);
    if (!split_a.ok() || !split_b.ok()) {
      if (err) *err << "rigorous: holdout_split should succeed on large sample count\n";
      return false;
    }
    const auto train_a = as_i64_vec(split_a.value().train_indices);
    const auto test_a = as_i64_vec(split_a.value().test_indices);
    const auto train_b = as_i64_vec(split_b.value().train_indices);
    const auto test_b = as_i64_vec(split_b.value().test_indices);
    if (train_a != train_b || test_a != test_b) {
      if (err) *err << "rigorous: holdout_split should remain deterministic\n";
      return false;
    }
    if (train_a.size() != 8000 || test_a.size() != 2000) {
      if (err) *err << "rigorous: holdout_split sizes should match requested fraction\n";
      return false;
    }
    if (has_duplicates(train_a) || has_duplicates(test_a)) {
      if (err) *err << "rigorous: holdout indices should remain duplicate-free\n";
      return false;
    }

    std::vector<double> labels_vec(10000, 0.0);
    std::fill(labels_vec.begin() + 7000, labels_vec.begin() + 9000, 1.0);
    std::fill(labels_vec.begin() + 9000, labels_vec.end(), 2.0);
    const Series labels = pluss::make_series(std::move(labels_vec), {10000}, f64());

    auto strat_a = strat_holdout_split(labels, options);
    auto strat_b = strat_holdout_split(labels, options);
    if (!strat_a.ok() || !strat_b.ok()) {
      if (err) *err << "rigorous: strat_holdout_split should succeed on large class-imbalanced labels\n";
      return false;
    }
    const auto strat_test_a = as_i64_vec(strat_a.value().test_indices);
    const auto strat_test_b = as_i64_vec(strat_b.value().test_indices);
    if (strat_test_a != strat_test_b) {
      if (err) *err << "rigorous: strat_holdout_split should remain deterministic\n";
      return false;
    }

    std::array<std::int64_t, 3> counts{0, 0, 0};
    for (std::int64_t idx : strat_test_a) {
      if (idx < 7000) {
        ++counts[0];
      } else if (idx < 9000) {
        ++counts[1];
      } else {
        ++counts[2];
      }
    }
    if (!(counts[0] >= 1399 && counts[0] <= 1401 && counts[1] >= 399 && counts[1] <= 401 && counts[2] >= 199
          && counts[2] <= 201)) {
      if (err) *err << "rigorous: strat_holdout_split should preserve class proportions closely\n";
      return false;
    }
  }

  {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const Series x = pluss::make_series(std::vector<double>{
                                            5.0, 1.0, nan,
                                            5.0, 2.0, nan,
                                            5.0, nan, nan,
                                            5.0, 4.0, nan,
                                            5.0, 5.0, nan},
                                        {5, 3},
                                        f64());
    const FeatureBatch batch{.x = x, .layout = ObservationLayout::Rows};

    prep::StandardScaler scaler;
    auto scaled_or = scaler.learn_map(batch);
    if (!scaled_or.ok()) {
      if (err) *err << "rigorous: StandardScaler learn_map should succeed on constant/all-NaN features\n";
      return false;
    }
    const Series scaled = scaled_or.value();
    if (!scaled.isnan().eq(x.isnan()).all().item<bool>()) {
      if (err) *err << "rigorous: StandardScaler should preserve NaN mask locations\n";
      return false;
    }
    if (!scaled.isnan().logical_not().where(scaled, pluss::zeros_like(scaled, ScalarType::Float64)).isfinite().all().item<bool>()) {
      if (err) *err << "rigorous: StandardScaler non-NaN outputs should be finite\n";
      return false;
    }

    prep::SimpleImputer imputer(prep::SimpleImputerOptions{
        .strategy = prep::SimpleImputerStrategy::Mean,
        .empty_feature_fallback = 0.0,
    });
    auto imputed_or = imputer.learn_map(batch);
    if (!imputed_or.ok()) {
      if (err) *err << "rigorous: SimpleImputer(mean) learn_map should succeed\n";
      return false;
    }
    if (imputed_or.value().isnan().any().item<bool>()) {
      if (err) *err << "rigorous: SimpleImputer(mean) should remove NaNs\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej rigorous suite") {
  REQUIRE(run_nodej_rigorous_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_rigorous_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_rigorous_suite_test passed\n";
  return 0;
}
#endif
