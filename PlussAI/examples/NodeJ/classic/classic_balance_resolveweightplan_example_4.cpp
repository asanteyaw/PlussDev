#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const pluss::Series batch_weights = pluss::make_series({2.0, 2.0, 2.0, 2.0}, {4}, f64);

  cls::TrainOptions options{};
  options.sample_weights = pluss::make_series({1.0, 3.0, 1.0, 3.0}, {4}, f64);
  const cls::WeightPlan plan = cls::resolve_weight_plan(labels, batch_weights, options).value();
  std::cout << "sample_weights (2*[1,3,1,3] = [2,6,2,6]) = " << plan.sample_weights << "\n";

  return 0;
}
