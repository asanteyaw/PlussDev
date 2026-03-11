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

void report_classifier(const std::string& name,
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

void report_regressor(const std::string& name,
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
  std::cout << "lymph presets demo\n";

  const Series x_cls = pluss::make_series(std::vector<double>{
                                              -2.0, -1.0,
                                              -1.5, -1.0,
                                              -2.0, -2.0,
                                               1.0,  1.0,
                                               2.0,  1.0,
                                               1.0,  2.0},
                                          {6, 2},
                                          f64());
  const Series y_cls = pluss::make_series(std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64());
  const LabeledBatch cls_train{
      .features = FeatureBatch{.x = x_cls, .layout = ObservationLayout::Rows},
      .y = y_cls,
  };

  std::cout << "Classification presets:\n";
  report_classifier("linear_logistic", pluss::ai::lymph::linear_logistic_classifier(cls_train), cls_train);
  report_classifier("linear_hinge", pluss::ai::lymph::linear_hinge_classifier(cls_train), cls_train);
  report_classifier("forest", pluss::ai::lymph::forest_classifier(cls_train, 21, 0.9), cls_train);
  report_classifier("rbf_kernel", pluss::ai::lymph::rbf_kernel_classifier(cls_train, 30.0, 1.0, 0.0), cls_train);
  report_classifier("gaussian_bayes", pluss::ai::lymph::gaussian_bayes_classifier(cls_train, 1e-9), cls_train);

  const Series x_reg = pluss::make_series(std::vector<double>{
                                              -2.0, -1.0,
                                              -1.0, -0.5,
                                               0.0,  0.0,
                                               1.0,  0.5,
                                               2.0,  1.0,
                                               3.0,  1.5},
                                          {6, 2},
                                          f64());
  const Series y_reg = pluss::make_series(std::vector<double>{-1.0, -0.5, 0.0, 0.5, 1.0, 1.5}, {6}, f64());
  const LabeledBatch reg_train{
      .features = FeatureBatch{.x = x_reg, .layout = ObservationLayout::Rows},
      .y = y_reg,
  };

  std::cout << "Regression presets:\n";
  report_regressor("ridge", pluss::ai::lymph::ridge_regressor(reg_train, 1e-8), reg_train);
  report_regressor("forest", pluss::ai::lymph::forest_regressor(reg_train, 31, 0.9), reg_train);
  report_regressor("rbf_kernel", pluss::ai::lymph::rbf_kernel_regressor(reg_train, 50.0, 0.7, 0.0), reg_train);
  report_regressor("gaussian_bayes", pluss::ai::lymph::gaussian_bayes_regressor(reg_train, 1e-6), reg_train);

  std::cout << "lymph presets demo completed.\n";
  return 0;
}
