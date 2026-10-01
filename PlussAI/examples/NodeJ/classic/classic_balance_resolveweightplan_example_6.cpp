#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);
  cls::TrainOptions options{};
  options.class_weight = cls::ClassWeightMode::Custom;
  options.class_weights = pluss::make_series({1.0, 20.0}, {2}, f64);

  const cls::WeightPlan plan = cls::resolve_weight_plan(labels, pluss::series::undefined(), options).value();
  std::cout << "mean_weight (pre-normalization mean) = " << plan.mean_weight << "\n";
  std::cout << "weights.mean() (always 1.0) = " << plan.weights.mean().item<double>() << "\n";

  return 0;
}
