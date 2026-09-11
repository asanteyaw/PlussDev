// core::TrainOptions example 1 -- defaults, then max_iterations/tolerance.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0}, {8},
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU)).reshape({8, 1});
  const cls::Series y = pluss::make_series(
      {2.1, 4.0, 6.1, 7.9, 10.2, 11.9, 14.1, 15.8}, {8},
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU));

  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});
  cls::TrainOptions defaults{};
  const cls::Status s1 = made.value()->learn(batch, defaults);
  std::cout << "default fit ok = " << std::boolalpha << s1.ok() << "\n";

  cls::TrainOptions tight{};
  tight.max_iterations = 5000;
  tight.tolerance = 1e-10;
  const cls::Status s2 = made.value()->learn(batch, tight);
  std::cout << "tight fit ok   = " << s2.ok() << "\n";

  return 0;
}
