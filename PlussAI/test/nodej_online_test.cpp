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
using pluss::ai::nodej::LinearFamilyOptions;
using pluss::ai::nodej::LinearObjective;
using pluss::ai::nodej::MetricKind;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::RegularizationKind;
using pluss::ai::nodej::TrainOptions;
using pluss::ai::nodej::online::OnlineLinearOptions;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

Config i64() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

LabeledBatch slice_rows(const LabeledBatch& batch, std::int64_t start, std::int64_t count) {
  const Series idx = pluss::arange(start, start + count, i64());
  const Series x = batch.features.x.index_select(0, idx).reshape({count, batch.features.feature_count()});
  const Series y = batch.y.index_select(0, idx).reshape({count});
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

LabeledBatch make_linear_classification_batch() {
  std::vector<double> xv{};
  std::vector<double> yv{};
  xv.reserve(2 * 120);
  yv.reserve(120);

  for (int i = 0; i < 60; ++i) {
    const double x0 = -2.5 + 0.05 * static_cast<double>(i);
    const double x1 = -1.8 + 0.04 * static_cast<double>(i);
    xv.push_back(x0);
    xv.push_back(x1);
    yv.push_back(0.0);
  }
  for (int i = 0; i < 60; ++i) {
    const double x0 = 1.0 + 0.05 * static_cast<double>(i);
    const double x1 = 1.4 + 0.04 * static_cast<double>(i);
    xv.push_back(x0);
    xv.push_back(x1);
    yv.push_back(1.0);
  }

  const Series x = pluss::make_series(std::move(xv), {120, 2}, f64());
  const Series y = pluss::make_series(std::move(yv), {120}, f64());
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

LabeledBatch make_linear_regression_batch() {
  std::vector<double> xv{};
  std::vector<double> yv{};
  xv.reserve(2 * 150);
  yv.reserve(150);

  for (int i = 0; i < 150; ++i) {
    const double x0 = -3.0 + 0.04 * static_cast<double>(i);
    const double x1 = 2.0 - 0.03 * static_cast<double>(i);
    const double y = 1.2 + 2.8 * x0 - 1.6 * x1;
    xv.push_back(x0);
    xv.push_back(x1);
    yv.push_back(y);
  }

  const Series x = pluss::make_series(std::move(xv), {150, 2}, f64());
  const Series y = pluss::make_series(std::move(yv), {150}, f64());
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

bool run_nodej_online_checks(std::ostream* err = nullptr) {
  {
    const LabeledBatch full = make_linear_classification_batch();
    const LabeledBatch warm = slice_rows(full, 48, 24);
    const LabeledBatch stream_a = slice_rows(full, 0, 48);
    const LabeledBatch stream_b = slice_rows(full, 72, 48);

    OnlineLinearOptions options{};
    options.family = LinearFamilyOptions{
        .objective = LinearObjective::Logistic,
        .regularization = RegularizationKind::L2,
        .regularization_strength = 1e-6,
        .fit_intercept = true,
    };
    options.learning_rate = 0.08;
    options.decaying_rate = true;

    auto model_or = pluss::ai::nodej::online::make_online_linear_classifier(options);
    if (!model_or.ok()) {
      if (err) *err << "make_online_linear_classifier should succeed\n";
      return false;
    }
    auto model = model_or.move_value();

    TrainOptions warm_train{};
    warm_train.max_iterations = 100;
    warm_train.tolerance = 1e-6;
    if (!model.learn(warm, warm_train).ok()) {
      if (err) *err << "online classifier warm learn should succeed\n";
      return false;
    }
    if (!model.advance(stream_a).ok() || !model.advance(stream_b).ok()) {
      if (err) *err << "online classifier advance should succeed\n";
      return false;
    }
    const std::int64_t expected_seen =
        warm_train.max_iterations * warm.features.sample_count() + stream_a.features.sample_count() + stream_b.features.sample_count();
    if (model.seen_samples() != expected_seen) {
      if (err) *err << "online classifier should track seen sample count across warm+stream updates\n";
      return false;
    }

    pluss::ai::nodej::AssessOptions assess{};
    assess.metrics = {MetricKind::Accuracy, MetricKind::LogLoss};
    auto metrics_or = model.assess(full, assess);
    if (!metrics_or.ok()) {
      if (err) *err << "online classifier assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.size() != 2 || metrics[0] < 0.93 || metrics[1] > 0.35) {
      if (err) *err << "online classifier should reach high accuracy with low logloss; got accuracy="
                    << (metrics.empty() ? -1.0 : metrics[0]) << ", logloss="
                    << (metrics.size() < 2 ? -1.0 : metrics[1]) << '\n';
      return false;
    }

    auto lymph_or = pluss::ai::lymph::online_linear_classifier(warm, options, warm_train);
    if (!lymph_or.ok()) {
      if (err) *err << "lymph online_linear_classifier should succeed\n";
      return false;
    }
    auto lymph_model = lymph_or.move_value();
    if (!lymph_model.advance(stream_a).ok()) {
      if (err) *err << "lymph online classifier should support advance after warm start\n";
      return false;
    }
  }

  {
    const LabeledBatch full = make_linear_regression_batch();
    const LabeledBatch warm = slice_rows(full, 0, 120);
    const LabeledBatch stream_a = slice_rows(full, 120, 15);
    const LabeledBatch stream_b = slice_rows(full, 135, 15);

    OnlineLinearOptions options{};
    options.family = LinearFamilyOptions{
        .objective = LinearObjective::LeastSquares,
        .regularization = RegularizationKind::L2,
        .regularization_strength = 1e-8,
        .fit_intercept = true,
    };
    options.learning_rate = 0.01;
    options.decaying_rate = false;

    auto model_or = pluss::ai::nodej::online::make_online_linear_regressor(options);
    if (!model_or.ok()) {
      if (err) *err << "make_online_linear_regressor should succeed\n";
      return false;
    }
    auto model = model_or.move_value();

    TrainOptions warm_train{};
    warm_train.max_iterations = 500;
    warm_train.tolerance = 1e-6;
    if (!model.learn(warm, warm_train).ok()) {
      if (err) *err << "online regressor warm learn should succeed\n";
      return false;
    }
    if (!model.advance(stream_a).ok() || !model.advance(stream_b).ok()) {
      if (err) *err << "online regressor advance should succeed\n";
      return false;
    }
    const std::int64_t expected_seen =
        warm_train.max_iterations * warm.features.sample_count() + stream_a.features.sample_count() + stream_b.features.sample_count();
    if (model.seen_samples() != expected_seen) {
      if (err) *err << "online regressor should track seen sample count across warm+stream updates\n";
      return false;
    }

    auto metrics_or = model.assess(full);
    if (!metrics_or.ok()) {
      if (err) *err << "online regressor assess should succeed\n";
      return false;
    }
    const auto metrics = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    if (metrics.size() < 3 || metrics[0] > 0.5 || metrics[2] < 0.98) {
      if (err) *err << "online regressor should learn low-MSE high-R2 fit; got mse="
                    << (metrics.empty() ? -1.0 : metrics[0]) << ", r2="
                    << (metrics.size() < 3 ? -1.0 : metrics[2]) << '\n';
      return false;
    }

    auto lymph_or = pluss::ai::lymph::online_linear_regressor(warm, options, warm_train);
    if (!lymph_or.ok()) {
      if (err) *err << "lymph online_linear_regressor should succeed\n";
      return false;
    }
    auto lymph_model = lymph_or.move_value();
    if (!lymph_model.advance(stream_a).ok()) {
      if (err) *err << "lymph online regressor should support advance after warm start\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej online linear learners") {
  const bool ok = run_nodej_online_checks(&std::cerr);
  REQUIRE(ok);
}
#else
int main() {
  if (!run_nodej_online_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_online_test: OK\n";
  return 0;
}
#endif
