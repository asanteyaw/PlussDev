#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0}, {8}, f64);

  cls::SplitOptions options{};
  options.folds = 2;
  options.repeats = 3;
  const cls::SplitPlan plan = cls::stratified_crossval_plan(labels, options).value();
  std::cout << "folds.size() = " << plan.folds.size() << " (expected 2 * 3 = 6)\n";

  return 0;
}
