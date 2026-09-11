// core::TrainOptions example 3 -- warm_start continues an existing fit
// instead of discarding it.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, {6}, f64).reshape({6, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0, 10.0, 12.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  cls::TrainOptions fresh{};
  fresh.max_iterations = 1;  // deliberately under-converged
  made.value()->learn(batch, fresh);
  const bool trained_after_one_pass = made.value()->trained();

  cls::TrainOptions warm{};
  warm.warm_start = true;
  warm.max_iterations = 1;
  made.value()->learn(batch, warm);  // continues from the under-converged fit

  std::cout << "trained after 1 iteration = " << std::boolalpha << trained_after_one_pass << "\n";
  std::cout << "still trained after warm-started pass = " << made.value()->trained() << "\n";

  return 0;
}
