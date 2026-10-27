#include <pluss/ml.h>

#include <iostream>
#include <limits>
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

void print_metric_values(const std::string& label, const Series& x) {
  const auto v = x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
  std::cout << label << ":";
  for (double value : v) std::cout << " " << value;
  std::cout << "\n";
}

} // namespace

int main() {
  std::cout << "NodeJ Flow demo\n";

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -3.0, -1.0,
                                            -2.0, -1.0,
                                            std::numeric_limits<double>::quiet_NaN(), -0.8,
                                            -1.0, -0.6,
                                            1.0, 0.6,
                                            2.0, 0.8,
                                            3.0, 1.0,
                                            std::numeric_limits<double>::quiet_NaN(), 0.7},
                                        {8, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    ModelSpec cls_spec{
        .task = TaskKind::Classification,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::Logistic,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-6,
            .fit_intercept = true,
        },
    };

    auto flow_or = make_flow(cls_spec);
    if (!require_ok(flow_or, "make_flow(classification)")) return 1;
    Flow cls = flow_or.move_value();
    if (!cls.add_simple_imputer(prep::SimpleImputerOptions{.strategy = prep::SimpleImputerStrategy::Mean}).ok()) return 1;
    if (!cls.add_standard_scaler().ok()) return 1;

    TrainOptions opts{};
    opts.max_iterations = 3000;
    opts.tolerance = 1e-8;
    if (!cls.learn(train, opts).ok()) {
      std::cerr << "classification flow learn failed\n";
      return 1;
    }

    auto metric_or = cls.assess(train, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::LogLoss}});
    if (!require_ok(metric_or, "classification flow assess")) return 1;
    print_metric_values("classification flow metrics [accuracy, logloss]", metric_or.value());
  }

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -2.0, 0.0,
                                            -1.0, std::numeric_limits<double>::quiet_NaN(),
                                            0.0, 0.0,
                                            1.0, 1.0,
                                            2.0, 0.0,
                                            3.0, 1.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{-3.0, -1.0, 1.0, 3.0, 5.0, 7.0}, {6}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    ModelSpec reg_spec{
        .task = TaskKind::Regression,
        .family = ModelFamily::Linear,
        .options = LinearFamilyOptions{
            .objective = LinearObjective::LeastSquares,
            .regularization = RegularizationKind::L2,
            .regularization_strength = 1e-8,
            .fit_intercept = true,
        },
    };

    auto flow_or = make_flow(reg_spec);
    if (!require_ok(flow_or, "make_flow(regression)")) return 1;
    Flow reg = flow_or.move_value();
    if (!reg.add_simple_imputer(prep::SimpleImputerOptions{.strategy = prep::SimpleImputerStrategy::Mean}).ok()) return 1;
    if (!reg.add_standard_scaler().ok()) return 1;

    if (!reg.learn(train).ok()) {
      std::cerr << "regression flow learn failed\n";
      return 1;
    }

    auto metric_or = reg.assess(train, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
    if (!require_ok(metric_or, "regression flow assess")) return 1;
    print_metric_values("regression flow metrics [mse, r2]", metric_or.value());
  }

  std::cout << "NodeJ Flow demo completed.\n";
  return 0;
}
