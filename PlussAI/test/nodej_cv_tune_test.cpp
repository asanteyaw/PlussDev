#include <pluss/ai/ml.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
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

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

LabeledBatch make_xor_batch() {
  const Series x = pluss::make_series(std::vector<double>{
                                          -1.0, -1.0,
                                          -1.0,  1.0,
                                           1.0, -1.0,
                                           1.0,  1.0,
                                          -0.9, -0.9,
                                          -0.9,  0.9,
                                           0.9, -0.9,
                                           0.9,  0.9,
                                          -1.2, -1.1,
                                          -1.1,  1.2,
                                           1.1, -1.2,
                                           1.2,  1.1,
                                          -0.4, -0.7,
                                          -0.5,  0.8,
                                           0.6, -0.8,
                                           0.5,  0.7},
                                      {16, 2},
                                      f64());
  const Series y = pluss::make_series(std::vector<double>{
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0,
                                          0.0, 1.0, 1.0, 0.0},
                                      {16},
                                      f64());
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

ModelSpec linear_logistic_spec() {
  return ModelSpec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::Logistic,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      },
  };
}

ModelSpec rbf_kernel_spec() {
  return ModelSpec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Kernel,
      .options = KernelFamilyOptions{
          .kernel = KernelKind::Rbf,
          .c = 40.0,
          .gamma = 2.0,
          .epsilon = 0.0,
      },
  };
}

ModelSpec linear_reg_spec(double lambda, bool fit_intercept = true) {
  return ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = lambda,
          .fit_intercept = fit_intercept,
      },
  };
}

bool run_nodej_cv_tune_checks(std::ostream* err = nullptr) {
  const LabeledBatch batch = make_xor_batch();

  SplitOptions split{};
  split.plan = CVPlanKind::KFold;
  split.folds = 4;
  split.shuffle = true;
  split.random.seed = 73;
  split.random.deterministic = true;

  TrainOptions train{};
  train.max_iterations = 2500;
  train.tolerance = 1e-8;

  AssessOptions assess{};
  assess.metrics = {
      MetricKind::Accuracy,
      MetricKind::BalancedAccuracy,
      MetricKind::Precision,
      MetricKind::Recall,
      MetricKind::F1,
      MetricKind::RocAuc,
      MetricKind::LogLoss};

  {
    auto cv_or = cross_assess(linear_logistic_spec(), batch, split, train, assess);
    if (!cv_or.ok()) {
      if (err) *err << "cross_assess should succeed for linear logistic\n";
      return false;
    }
    const CrossAssessReport& cv = cv_or.value();
    if (cv.folds.size() != 4 || cv.metrics.size() != 7 || cv.mean_metrics.numel() != 7 || cv.std_metrics.numel() != 7) {
      if (err) *err << "cross_assess should return fold reports plus mean/std for requested metrics\n";
      return false;
    }
    if (!cv.mean_metrics.isfinite().all().item<bool>() || !cv.std_metrics.isfinite().all().item<bool>()) {
      if (err) *err << "cross_assess aggregate metrics should be finite\n";
      return false;
    }
    const auto mean = as_f64(cv.mean_metrics);
    if (mean.size() != 7 || mean[5] < 0.0 || mean[5] > 1.0) {
      if (err) *err << "ROC AUC should be present and bounded in [0,1]\n";
      return false;
    }
  }

  {
    const std::vector<ModelSpec> candidates{linear_logistic_spec(), rbf_kernel_spec()};
    AssessOptions tune_assess{};
    tune_assess.metrics = {MetricKind::Accuracy};
    auto sweep_or = sweep(candidates,
                          batch,
                          split,
                          train,
                          tune_assess,
                          MetricKind::Accuracy,
                          TuneObjectivePolicy::Auto);
    if (!sweep_or.ok()) {
      if (err) *err << "sweep should succeed for valid candidate list\n";
      return false;
    }

    const TuneReport& report = sweep_or.value();
    if (report.trials.size() != 2 || report.best_index < 0) {
      if (err) *err << "sweep should evaluate all candidates and choose a best trial\n";
      return false;
    }
    if (!(report.best_index == 1)) {
      if (err) *err << "kernel candidate should win on XOR with accuracy objective\n";
      return false;
    }
  }

  {
    auto sampler = [](std::int64_t trial_index, std::uint64_t seed) -> StatusOr<ModelSpec> {
      (void)seed;
      if (trial_index == 1) {
        return StatusOr<ModelSpec>(Status::invalid_argument("synthetic sampler failure"));
      }
      if ((trial_index % 2) == 0) {
        return StatusOr<ModelSpec>(linear_logistic_spec());
      }
      return StatusOr<ModelSpec>(rbf_kernel_spec());
    };

    AssessOptions tune_assess{};
    tune_assess.metrics = {MetricKind::Accuracy};
    auto probe_or = probe(sampler,
                          3,
                          batch,
                          split,
                          train,
                          tune_assess,
                          MetricKind::Accuracy,
                          TuneObjectivePolicy::Auto,
                          RandomState{.seed = 5, .deterministic = true});
    if (!probe_or.ok()) {
      if (err) *err << "probe should return report even if one sampled trial fails\n";
      return false;
    }

    const TuneReport& report = probe_or.value();
    if (report.trials.size() != 3) {
      if (err) *err << "probe should emit one trial per requested sample\n";
      return false;
    }
    if (report.trials[1].status.code != StatusCode::InvalidArgument) {
      if (err) *err << "probe should preserve sampler failure status in the corresponding trial\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -3.0,
                                            -2.0,
                                            -1.0,
                                             1.0,
                                             2.0,
                                             3.0},
                                        {6, 1},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
    const LabeledBatch easy{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto free_or = make_classifier(linear_logistic_spec());
    auto bounded_or = make_classifier(linear_logistic_spec());
    if (!free_or.ok() || !bounded_or.ok()) {
      if (err) *err << "make_classifier should succeed for linear logistic\n";
      return false;
    }

    TrainOptions free_train{};
    free_train.max_iterations = 1800;
    free_train.tolerance = 1e-8;

    TrainOptions bounded = free_train;
    bounded.parameter_lower_bounds = pluss::make_series(std::vector<double>{0.0}, {1}, f64());
    bounded.parameter_upper_bounds = pluss::make_series(std::vector<double>{0.0}, {1}, f64());

    auto free = free_or.move_value();
    auto bounded_model = bounded_or.move_value();
    if (!free->learn(easy, free_train).ok() || !bounded_model->learn(easy, bounded).ok()) {
      if (err) *err << "linear learn should succeed with and without parameter bounds\n";
      return false;
    }

    auto free_acc_or = free->assess(easy, AssessOptions{.metrics = {MetricKind::Accuracy}});
    auto bounded_acc_or = bounded_model->assess(easy, AssessOptions{.metrics = {MetricKind::Accuracy}});
    auto bounded_scores_or = bounded_model->infer_scores(easy.features);
    if (!free_acc_or.ok() || !bounded_acc_or.ok() || !bounded_scores_or.ok()) {
      if (err) *err << "assess/infer_scores should succeed after learn\n";
      return false;
    }

    const double free_acc = as_f64(free_acc_or.value())[0];
    const double bounded_acc = as_f64(bounded_acc_or.value())[0];
    const double bounded_max_abs_score = bounded_scores_or.value().abs().max().item<double>();

    if (free_acc < 0.99) {
      if (err) *err << "unconstrained logistic should fit separable data\n";
      return false;
    }
    if (!(bounded_acc >= 0.45 && bounded_acc <= 0.55)) {
      if (err) *err << "zero-bounded logistic should collapse near chance-level on balanced data\n";
      return false;
    }
    if (!std::isfinite(bounded_max_abs_score) || bounded_max_abs_score > 1e-8) {
      if (err) *err << "zero-bounded logistic should produce near-zero scores\n";
      return false;
    }
  }

  {
    const std::int64_t n = 80;
    std::vector<double> xv{};
    std::vector<double> yv{};
    xv.reserve(static_cast<std::size_t>(n));
    yv.reserve(static_cast<std::size_t>(n));
    for (std::int64_t i = 0; i < n; ++i) {
      const double x = -2.0 + 4.0 * static_cast<double>(i) / static_cast<double>(n - 1);
      const double y = 2.0 * x + 1.0 + ((i % 9) == 0 ? 0.35 : 0.0);
      xv.push_back(x);
      yv.push_back(y);
    }

    const LabeledBatch reg_batch{
        .features = FeatureBatch{.x = pluss::make_series(std::move(xv), {n, 1}, f64()), .layout = ObservationLayout::Rows},
        .y = pluss::make_series(std::move(yv), {n}, f64()),
    };

    AssessOptions reg_assess{};
    reg_assess.metrics = {MetricKind::QuantileLoss, MetricKind::Mse};
    reg_assess.regression_quantile = 0.8;

    const std::vector<ModelSpec> candidates{
        linear_reg_spec(1e-9, true),
        linear_reg_spec(200.0, true),
    };

    auto sweep_or = sweep(candidates,
                          reg_batch,
                          split,
                          TrainOptions{},
                          reg_assess,
                          MetricKind::QuantileLoss,
                          TuneObjectivePolicy::Auto);
    if (!sweep_or.ok()) {
      if (err) *err << "sweep should support QuantileLoss objective\n";
      return false;
    }

    const TuneReport& report = sweep_or.value();
    if (report.trials.size() != 2 || report.best_index < 0) {
      if (err) *err << "QuantileLoss sweep should evaluate and pick a best trial\n";
      return false;
    }
    double min_objective = std::numeric_limits<double>::infinity();
    std::int64_t min_index = -1;
    for (std::size_t i = 0; i < report.trials.size(); ++i) {
      if (!report.trials[i].status.ok()) continue;
      const double objective = report.trials[i].objective;
      if (!std::isfinite(objective)) continue;
      if (objective < min_objective) {
        min_objective = objective;
        min_index = static_cast<std::int64_t>(i);
      }
    }
    if (min_index < 0 || report.best_index != min_index) {
      if (err) *err << "Auto objective policy should minimize QuantileLoss\n";
      return false;
    }
  }

  {
    const Series x = pluss::make_series(std::vector<double>{-2.0, -1.0, 0.0, 1.0, 2.0}, {5, 1}, f64());
    const Series y = pluss::make_series(std::vector<double>{-3.0, -1.0, 1.0, 3.0, 5.0}, {5}, f64());
    const LabeledBatch batch_reg{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto reg_or = make_regressor(linear_reg_spec(0.0, false));
    if (!reg_or.ok()) {
      if (err) *err << "make_regressor should succeed for linear regression\n";
      return false;
    }
    auto reg = reg_or.move_value();
    if (!reg->learn(batch_reg).ok()) {
      if (err) *err << "linear regressor should learn on simple data\n";
      return false;
    }

    constexpr double q = 0.8;
    auto pred_or = reg->infer(batch_reg.features);
    auto qloss_or = reg->assess(batch_reg, AssessOptions{.metrics = {MetricKind::QuantileLoss}, .regression_quantile = q});
    if (!pred_or.ok() || !qloss_or.ok()) {
      if (err) *err << "infer and quantile assess should succeed\n";
      return false;
    }

    const auto y_true = as_f64(y);
    const auto y_pred = as_f64(pred_or.value());
    double manual = 0.0;
    for (std::size_t i = 0; i < y_true.size(); ++i) {
      const double r = y_true[i] - y_pred[i];
      manual += r * (q - (r < 0.0 ? 1.0 : 0.0));
    }
    manual /= static_cast<double>(y_true.size());

    const double got = as_f64(qloss_or.value())[0];
    if (!std::isfinite(got) || std::abs(got - manual) > 1e-10) {
      if (err) *err << "QuantileLoss should match pinball-loss definition\n";
      return false;
    }

    auto bad_q_or = reg->assess(batch_reg, AssessOptions{.metrics = {MetricKind::QuantileLoss}, .regression_quantile = 1.0});
    if (bad_q_or.ok() || bad_q_or.status().code != StatusCode::InvalidArgument) {
      if (err) *err << "invalid quantile should return InvalidArgument\n";
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej cross-assess, tuning, and constrained linear optimization") {
  REQUIRE(run_nodej_cv_tune_checks(&std::cerr));
}
#else
int main() {
  if (!run_nodej_cv_tune_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej_cv_tune_test passed\n";
  return 0;
}
#endif
