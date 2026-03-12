#include <pluss/lymph.h>

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
bool require_ok(const pluss::ai::lymph::quick::StatusOr<T>& out, const std::string& label) {
  if (out.ok()) return true;
  std::cerr << label << " failed: " << out.status().message << "\n";
  return false;
}

} // namespace

int main() {
  std::cout << "lymph Flow demo\n";

  {
    const Series x = pluss::make_series(std::vector<double>{
                                            -3.0, -1.0,
                                            -2.0, -1.0,
                                            -1.0, -0.5,
                                             1.0,  0.5,
                                             2.0,  1.0,
                                             3.0,  1.0},
                                        {6, 2},
                                        f64());
    const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
    const LabeledBatch train{
        .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
        .y = y,
    };

    auto flow_or = pluss::ai::lymph::quick::linear_classifier_flow(train, 1e-6, true, false);
    if (!require_ok(flow_or, "lymph::linear_classifier_flow")) return 1;

    auto metric_or = flow_or.value().assess(train, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::LogLoss}});
    if (!metric_or.ok()) {
      std::cerr << "lymph classification flow assess failed: " << metric_or.status().message << "\n";
      return 1;
    }
    const auto v = metric_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::cout << "classification flow metrics [accuracy, logloss]: " << v[0] << " " << v[1] << "\n";
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

    auto flow_or = pluss::ai::lymph::quick::ridge_regressor_flow(train, 1e-8, true, true);
    if (!require_ok(flow_or, "lymph::ridge_regressor_flow")) return 1;

    auto metric_or = flow_or.value().assess(train, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
    if (!metric_or.ok()) {
      std::cerr << "lymph regression flow assess failed: " << metric_or.status().message << "\n";
      return 1;
    }
    const auto v = metric_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
    std::cout << "regression flow metrics [mse, r2]: " << v[0] << " " << v[1] << "\n";
  }

  std::cout << "lymph Flow demo completed.\n";
  return 0;
}

