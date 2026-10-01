#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);

  cls::TrainOptions options{};
  options.class_weight = cls::ClassWeightMode::Custom;
  options.class_weights = pluss::make_series({1.0, 5.0}, {2}, f64);  // class 0 -> 1.0, class 1 -> 5.0
  const cls::WeightPlan plan = cls::resolve_weight_plan(labels, pluss::series::undefined(), options).value();
  std::cout << "weights (final, mean-normalized -- raw ratios were [1,1,5,5]) = " << plan.weights << "\n";

  return 0;
}
