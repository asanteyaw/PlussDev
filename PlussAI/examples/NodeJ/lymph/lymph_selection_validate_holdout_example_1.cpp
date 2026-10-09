#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// holdout() is a single train/test split, not k folds. split(frame) returns a
// prep::HoldoutSplit with train_set / test_set frames; the row count is
// preserved.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {1., 2., 3., 4., 5., 6., 7., 8., 9., 10., 11., 12., 13., 14., 15., 16.}, {16}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, {16}, f64);
  const DataFrame frame({{"x", Slate(x)}, {"target", Slate(y)}});

  const lymph::selection::HoldoutPlan plan =
      lymph::selection::holdout().test_size(0.25).shuffle(true).seed(20260901ULL);

  const lymph::prep::HoldoutSplit split = plan.split(frame);
  std::cout << "train_rows = " << split.train_set.ncases()
            << ", test_rows = " << split.test_set.ncases()
            << ", total = " << (split.train_set.ncases() + split.test_set.ncases()) << "\n";
  std::cout << "test_size option = " << plan.options().test_size << "\n";

  return 0;
}
