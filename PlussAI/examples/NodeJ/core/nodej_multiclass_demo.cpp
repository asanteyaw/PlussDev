#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::ai::nodej::FeatureBatch;
using pluss::ai::nodej::LabeledBatch;
using pluss::ai::nodej::LinearFamilyOptions;
using pluss::ai::nodej::LinearObjective;
using pluss::ai::nodej::MetricKind;
using pluss::ai::nodej::ModelFamily;
using pluss::ai::nodej::ModelSpec;
using pluss::ai::nodej::ObservationLayout;
using pluss::ai::nodej::RegularizationKind;
using pluss::ai::nodej::TaskKind;

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
  const auto vals = v.to(pluss::pCPU, ScalarType::Float64).host<double>();
  for (std::size_t i = 0; i < vals.size(); ++i) {
    std::cout << vals[i] << (i + 1 == vals.size() ? '\n' : ' ');
  }
}

} // namespace

int main() {
  const LabeledBatch batch = make_batch();

  const ModelSpec spec{
      .task = TaskKind::Classification,
      .family = ModelFamily::Linear,
      .options = LinearFamilyOptions{
          .objective = LinearObjective::Logistic,
          .regularization = RegularizationKind::L2,
          .regularization_strength = 1e-6,
          .fit_intercept = true,
      },
  };

  auto cls_or = pluss::ai::nodej::make_classifier(spec);
  if (!cls_or.ok()) {
    std::cerr << "make_classifier failed: " << cls_or.status().message << '\n';
    return 1;
  }
  auto cls = cls_or.move_value();
  const auto learn_status = cls->learn(batch);
  if (!learn_status.ok()) {
    std::cerr << "learn failed: " << learn_status.message << '\n';
    return 1;
  }

  auto score_or = cls->infer_scores(batch.features);
  if (!score_or.ok()) {
    std::cerr << "infer_scores failed: " << score_or.status().message << '\n';
    return 1;
  }
  std::cout << "scores shape: [" << score_or.value().sizes()[0] << ", " << score_or.value().sizes()[1] << "]\n";

  pluss::ai::nodej::AssessOptions assess{};
  assess.metrics = {MetricKind::Accuracy, MetricKind::F1, MetricKind::LogLoss};
  auto metric_or = cls->assess(batch, assess);
  if (!metric_or.ok()) {
    std::cerr << "assess failed: " << metric_or.status().message << '\n';
    return 1;
  }
  std::cout << "metrics [accuracy, f1, logloss]: ";
  print_vec(metric_or.value());

  auto flow_or = pluss::ai::nodej::make_flow(spec);
  if (!flow_or.ok()) {
    std::cerr << "make_flow failed: " << flow_or.status().message << '\n';
    return 1;
  }
  auto flow = flow_or.move_value();
  const auto scaler_status = flow.add_standard_scaler();
  if (!scaler_status.ok()) {
    std::cerr << "add_standard_scaler failed: " << scaler_status.message << '\n';
    return 1;
  }
  const auto flow_learn = flow.learn(batch);
  if (!flow_learn.ok()) {
    std::cerr << "flow learn failed: " << flow_learn.message << '\n';
    return 1;
  }

  auto flow_metrics_or = flow.assess(batch, assess);
  if (!flow_metrics_or.ok()) {
    std::cerr << "flow assess failed: " << flow_metrics_or.status().message << '\n';
    return 1;
  }
  std::cout << "flow metrics [accuracy, f1, logloss]: ";
  print_vec(flow_metrics_or.value());

  return 0;
}
