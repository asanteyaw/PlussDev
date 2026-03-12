#include <pluss/lymph.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

template <class T>
bool require_ok(const pluss::ai::lymph::StatusOr<T>& out, const std::string& label) {
  if (out.ok()) return true;
  std::cerr << label << " failed: " << out.status().message << "\n";
  return false;
}

LabeledBatch make_batch() {
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

ModelSpec linear_spec() {
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

ModelSpec kernel_spec() {
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

ModelSpec linear_reg_spec(double lambda) {
  return ModelSpec{
      .task = TaskKind::Regression,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::LeastSquares,
          .regularization = RegularizationKind::L2,
          .regularization_strength = lambda,
          .fit_intercept = true,
      },
  };
}

LabeledBatch make_reg_batch() {
  constexpr std::int64_t n = 80;
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
  return LabeledBatch{
      .features = FeatureBatch{
          .x = pluss::make_series(std::move(xv), {n, 1}, f64()),
          .layout = ObservationLayout::Rows,
      },
      .y = pluss::make_series(std::move(yv), {n}, f64()),
  };
}

} // namespace

int main() {
  std::cout << "lymph CV + tune demo\n";

  const LabeledBatch batch = make_batch();

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

  auto cv_or = pluss::ai::lymph::assess_cv(kernel_spec(), batch, split, train, assess);
  if (!require_ok(cv_or, "lymph::assess_cv(kernel)")) return 1;
  const auto mean = cv_or.value().mean_metrics.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  std::cout << "kernel cv mean [acc, bal_acc, precision, recall, f1, roc_auc, logloss]:";
  for (double v : mean) std::cout << " " << v;
  std::cout << "\n";

  AssessOptions tune_assess{};
  tune_assess.metrics = {MetricKind::Accuracy};
  const std::vector<ModelSpec> candidates{linear_spec(), kernel_spec()};
  auto sweep_or = pluss::ai::lymph::tune_sweep(candidates,
                                                batch,
                                                split,
                                                train,
                                                tune_assess,
                                                MetricKind::Accuracy,
                                                TuneObjectivePolicy::Auto);
  if (!require_ok(sweep_or, "lymph::tune_sweep")) return 1;

  std::cout << "lymph sweep best index: " << sweep_or.value().best_index << "\n";

  auto sampler = [](std::int64_t trial_index, std::uint64_t seed) -> StatusOr<ModelSpec> {
    (void)seed;
    return StatusOr<ModelSpec>((trial_index % 2) == 0 ? linear_spec() : kernel_spec());
  };

  auto probe_or = pluss::ai::lymph::tune_probe(sampler,
                                                4,
                                                batch,
                                                split,
                                                train,
                                                tune_assess,
                                                MetricKind::Accuracy,
                                                TuneObjectivePolicy::Auto,
                                                RandomState{.seed = 99, .deterministic = true});
  if (!require_ok(probe_or, "lymph::tune_probe")) return 1;

  std::cout << "lymph probe best index: " << probe_or.value().best_index << "\n";

  const LabeledBatch reg_batch = make_reg_batch();
  AssessOptions reg_assess{};
  reg_assess.metrics = {MetricKind::QuantileLoss, MetricKind::Mse};
  reg_assess.regression_quantile = 0.8;
  const std::vector<ModelSpec> reg_candidates{linear_reg_spec(1e-9), linear_reg_spec(200.0)};
  auto reg_sweep_or = pluss::ai::lymph::tune_sweep(reg_candidates,
                                                    reg_batch,
                                                    split,
                                                    TrainOptions{},
                                                    reg_assess,
                                                    MetricKind::QuantileLoss,
                                                    TuneObjectivePolicy::Auto);
  if (!require_ok(reg_sweep_or, "lymph::tune_sweep(regression quantile)")) return 1;
  std::cout << "lymph regression quantile sweep best index: " << reg_sweep_or.value().best_index << "\n";

  std::cout << "lymph CV + tune demo completed.\n";
  return 0;
}
