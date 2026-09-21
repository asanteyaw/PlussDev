#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 10 rows of class 0, 2 rows of class 1.
  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {12}, f64);

  cls::SplitOptions options{};
  options.holdout_fraction = 0.5;
  const cls::IndexSplit split = cls::strat_holdout_split(labels, options).value();
  std::cout << "train count = " << split.train_indices.numel() << "\n";
  std::cout << "test count  = " << split.test_indices.numel() << "\n";
  std::cout << "test_indices = " << split.test_indices << "\n";

  return 0;
}
