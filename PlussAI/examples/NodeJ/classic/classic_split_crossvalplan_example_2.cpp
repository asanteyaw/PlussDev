#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);

  cls::SplitOptions options{};
  options.folds = 2;
  const cls::SplitPlan plan = cls::stratified_crossval_plan(labels, options).value();

  for (std::size_t i = 0; i < plan.folds.size(); ++i) {
    std::cout << "fold " << i << " test_indices = " << plan.folds[i].test_indices << "\n";
  }

  return 0;
}
