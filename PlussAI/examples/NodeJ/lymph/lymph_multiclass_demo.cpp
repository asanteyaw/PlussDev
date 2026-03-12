#include <pluss/ai/ml.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::MetricKind;
using pluss::ai::nodej::ObservationLayout;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

LabeledBatch make_batch() {
  const Series x = pluss::make_series(std::vector<double>{
                                          -3.0, -3.0,
                                          -2.5, -2.0,
                                          -2.0, -2.5,
                                          -1.8, -1.7,
                                           0.0,  3.0,
                                           0.5,  2.5,
                                          -0.4,  2.2,
                                           0.3,  1.8,
                                           3.0, -1.0,
                                           2.5, -1.5,
                                           2.2, -0.4,
                                           1.8, -0.8},
                                      {12, 2},
                                      f64());
  const Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2}, {12}, f64());
  return LabeledBatch{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };
}

void print_vec(const Series& v) {
  const auto vals = v.to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  for (std::size_t i = 0; i < vals.size(); ++i) {
    std::cout << vals[i] << (i + 1 == vals.size() ? '\n' : ' ');
  }
}

} // namespace

int main() {
  const LabeledBatch batch = make_batch();

  auto cls_or = pluss::ai::lymph::quick::linear_logistic_classifier(batch, 1e-6);
  if (!cls_or.ok()) {
    std::cerr << "lymph::linear_logistic_classifier failed: " << cls_or.status().message << '\n';
    return 1;
  }

  auto score_or = cls_or.value()->infer_scores(batch.features);
  if (!score_or.ok()) {
    std::cerr << "infer_scores failed: " << score_or.status().message << '\n';
    return 1;
  }
  std::cout << "scores shape: [" << score_or.value().sizes()[0] << ", " << score_or.value().sizes()[1] << "]\n";

  pluss::ai::nodej::AssessOptions assess{};
  assess.metrics = {MetricKind::Accuracy, MetricKind::BalancedAccuracy, MetricKind::LogLoss};
  auto metrics_or = cls_or.value()->assess(batch, assess);
  if (!metrics_or.ok()) {
    std::cerr << "assess failed: " << metrics_or.status().message << '\n';
    return 1;
  }

  std::cout << "metrics [accuracy, balanced_accuracy, logloss]: ";
  print_vec(metrics_or.value());
  return 0;
}
