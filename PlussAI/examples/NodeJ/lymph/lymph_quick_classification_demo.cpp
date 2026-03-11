#include <pluss/lymph.h>

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

void run_and_report(const std::string& name,
                    pluss::ai::lymph::StatusOr<std::unique_ptr<ClassifierBase>>&& model_or,
                    const LabeledBatch& train) {
  if (!require_ok(model_or, name)) return;
  auto model = model_or.move_value();
  auto metrics_or = model->assess(train, AssessOptions{.metrics = {MetricKind::Accuracy, MetricKind::LogLoss}});
  if (!metrics_or.ok()) {
    std::cerr << name << " assess failed: " << metrics_or.status().message << "\n";
    return;
  }
  const auto m = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  std::cout << "  " << name << " (acc, logloss): " << m[0] << " " << m[1] << "\n";
}

} // namespace

int main() {
  std::cout << "lymph quick classification demo\n";

  const Series x = pluss::make_series(std::vector<double>{
                                          -2.0, -1.0,
                                          -1.5, -1.0,
                                          -2.0, -2.0,
                                           1.0,  1.0,
                                           2.0,  1.0,
                                           1.0,  2.0},
                                      {6, 2},
                                      f64());
  const Series y = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
  const LabeledBatch train{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };

  run_and_report("linear_logistic", pluss::ai::lymph::linear_logistic_classifier(train, 1e-6), train);
  run_and_report("linear_hinge", pluss::ai::lymph::linear_hinge_classifier(train, 1e-6), train);
  run_and_report("tree", pluss::ai::lymph::tree_classifier(train), train);
  run_and_report("forest", pluss::ai::lymph::forest_classifier(train, 21, 0.9), train);
  run_and_report("neighbor", pluss::ai::lymph::neighbor_classifier(
                                 train,
                                 NeighborFamilyOptions{
                                     .k = 3,
                                     .distance_weighted = true,
                                     .standardize = true,
                                 }),
                 train);
  run_and_report("rbf_kernel", pluss::ai::lymph::rbf_kernel_classifier(train, 30.0, 1.0, 0.0), train);
  run_and_report("gaussian_bayes", pluss::ai::lymph::gaussian_bayes_classifier(train, 1e-9), train);

  std::cout << "lymph quick classification demo completed.\n";
  return 0;
}
