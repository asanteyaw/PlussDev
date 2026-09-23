#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 0: 6 rows, class 1: 2 rows.
  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);

  cls::TrainOptions options{};
  options.class_weight = cls::ClassWeightMode::Balanced;
  const cls::WeightPlan plan = cls::resolve_weight_plan(labels, pluss::Series::undefined(), options).value();
  std::cout << "class_weights = " << plan.class_weights << "\n";
  std::cout << "mix.inverse_frequency (expect identical) = " << plan.mix.inverse_frequency << "\n";
  std::cout << "weights (minority rows weigh more) = " << plan.weights << "\n";

  return 0;
}
