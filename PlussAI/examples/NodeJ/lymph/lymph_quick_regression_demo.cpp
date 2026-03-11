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
                    pluss::ai::lymph::StatusOr<std::unique_ptr<RegressorBase>>&& model_or,
                    const LabeledBatch& train) {
  if (!require_ok(model_or, name)) return;
  auto model = model_or.move_value();
  auto metrics_or = model->assess(train, AssessOptions{.metrics = {MetricKind::Mse, MetricKind::R2}});
  if (!metrics_or.ok()) {
    std::cerr << name << " assess failed: " << metrics_or.status().message << "\n";
    return;
  }
  const auto m = metrics_or.value().to(pluss::pCPU, ScalarType::Float64).to_vec<double>();
  std::cout << "  " << name << " (mse, r2): " << m[0] << " " << m[1] << "\n";
}

} // namespace

int main() {
  std::cout << "lymph quick regression demo\n";

  const Series x = pluss::make_series(std::vector<double>{
                                          -2.0, -1.0,
                                          -1.0, -0.5,
                                           0.0,  0.0,
                                           1.0,  0.5,
                                           2.0,  1.0,
                                           3.0,  1.5},
                                      {6, 2},
                                      f64());
  const Series y = pluss::make_series(std::vector<double>{-1.0, -0.5, 0.0, 0.5, 1.0, 1.5}, {6}, f64());
  const LabeledBatch train{
      .features = FeatureBatch{.x = x, .layout = ObservationLayout::Rows},
      .y = y,
  };

  run_and_report("ridge", pluss::ai::lymph::ridge_regressor(train, 1e-8), train);
  run_and_report("tree", pluss::ai::lymph::tree_regressor(
                             train,
                             TreeFamilyOptions{
                                 .criterion = TreeCriterion::Mse,
                             }),
                 train);
  run_and_report("forest", pluss::ai::lymph::forest_regressor(train, 31, 0.9), train);
  run_and_report("neighbor", pluss::ai::lymph::neighbor_regressor(
                                 train,
                                 NeighborFamilyOptions{
                                     .k = 3,
                                     .distance_weighted = true,
                                     .standardize = true,
                                 }),
                 train);
  run_and_report("rbf_kernel", pluss::ai::lymph::rbf_kernel_regressor(train, 50.0, 0.7, 0.0), train);
  run_and_report("gaussian_bayes", pluss::ai::lymph::gaussian_bayes_regressor(train, 1e-6), train);

  std::cout << "lymph quick regression demo completed.\n";
  return 0;
}
