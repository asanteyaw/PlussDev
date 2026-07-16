#include <pluss/ml.h>

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
bool require_ok(const StatusOr<T>& out, const std::string& label) {
  if (out.ok()) return true;
  std::cerr << label << " failed: " << out.status().message << "\n";
  return false;
}

void print_series(const std::string& label, const Series& x) {
  const auto v = x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
  std::cout << label << ":";
  for (double value : v) std::cout << " " << value;
  std::cout << "\n";
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
  std::cout << "NodeJ CV + tune demo\n";

  const LabeledBatch batch = make_xor_batch();

  SplitOptions split{};
  split.plan = CVPlanKind::CrossVal;
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

  auto linear_cv_or = cross_assess(linear_logistic_spec(), batch, split, train, assess);
  if (!require_ok(linear_cv_or, "cross_assess(linear)")) return 1;
  print_series("linear mean metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]", linear_cv_or.value().mean_metrics);
  print_series("linear std metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]", linear_cv_or.value().std_metrics);

  auto kernel_cv_or = cross_assess(rbf_kernel_spec(), batch, split, train, assess);
  if (!require_ok(kernel_cv_or, "cross_assess(kernel)")) return 1;
  print_series("kernel mean metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]", kernel_cv_or.value().mean_metrics);
  print_series("kernel std metrics [acc, bal_acc, precision, recall, f1, roc_auc, logloss]", kernel_cv_or.value().std_metrics);

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
  if (!require_ok(sweep_or, "sweep")) return 1;

  std::cout << "sweep trial objectives:";
  for (const auto& trial : sweep_or.value().trials) {
    std::cout << " " << trial.objective;
  }
  std::cout << "\n";
  std::cout << "sweep best index: " << sweep_or.value().best_index << "\n";

  auto sampler = [](std::int64_t trial_index, std::uint64_t seed) -> StatusOr<ModelSpec> {
    (void)seed;
    return StatusOr<ModelSpec>((trial_index % 2) == 0 ? linear_logistic_spec() : rbf_kernel_spec());
  };

  auto probe_or = probe(sampler,
                        4,
                        batch,
                        split,
                        train,
                        tune_assess,
                        MetricKind::Accuracy,
                        TuneObjectivePolicy::Auto,
                        RandomState{.seed = 11, .deterministic = true});
  if (!require_ok(probe_or, "probe")) return 1;
  std::cout << "probe best index: " << probe_or.value().best_index << "\n";

  const LabeledBatch reg_batch = make_reg_batch();
  AssessOptions reg_assess{};
  reg_assess.metrics = {MetricKind::QuantileLoss, MetricKind::Mse};
  reg_assess.regression_quantile = 0.8;
  const std::vector<ModelSpec> reg_candidates{linear_reg_spec(1e-9), linear_reg_spec(200.0)};
  auto reg_sweep_or = sweep(reg_candidates,
                            reg_batch,
                            split,
                            TrainOptions{},
                            reg_assess,
                            MetricKind::QuantileLoss,
                            TuneObjectivePolicy::Auto);
  if (!require_ok(reg_sweep_or, "sweep(regression quantile)")) return 1;
  std::cout << "regression quantile sweep objectives:";
  for (const auto& trial : reg_sweep_or.value().trials) std::cout << " " << trial.objective;
  std::cout << "\n";
  std::cout << "regression quantile sweep best index: " << reg_sweep_or.value().best_index << "\n";

  auto bounded_or = make_classifier(linear_logistic_spec());
  if (!require_ok(bounded_or, "make_classifier(linear bounded)")) return 1;
  auto bounded = bounded_or.move_value();

  TrainOptions bounded_train = train;
  bounded_train.parameter_lower_bounds = pluss::make_series(std::vector<double>{0.0}, {1}, f64());
  bounded_train.parameter_upper_bounds = pluss::make_series(std::vector<double>{0.0}, {1}, f64());
  if (!bounded->learn(batch, bounded_train).ok()) {
    std::cerr << "bounded linear learn failed\n";
    return 1;
  }

  auto bounded_scores_or = bounded->infer_scores(batch.features);
  if (!require_ok(bounded_scores_or, "infer_scores(bounded)")) return 1;
  std::cout << "max |bounded score|: " << bounded_scores_or.value().abs().max().item<double>() << "\n";

  std::cout << "NodeJ CV + tune demo completed.\n";
  return 0;
}
