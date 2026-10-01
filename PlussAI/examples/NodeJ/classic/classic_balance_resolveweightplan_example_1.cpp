#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const cls::WeightPlan plan = cls::resolve_weight_plan(labels, pluss::series::undefined(), cls::TrainOptions{}).value();
  std::cout << "weights = " << plan.weights << "\n";
  std::cout << "class_weights = " << plan.class_weights << "\n";
  std::cout << "mean_weight = " << plan.mean_weight << "\n";

  return 0;
}
