#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 30.0,
       1.0, 11.0, 21.0, 31.0},
      {8}, f64).reshape({2, 4});
  cls::FeatureBatch batch{};
  batch.x = x;

  const cls::FeatureBatch reduced = cls::take_features(batch, {3, 1}).value();
  std::cout << "reduced.x = " << reduced.x << "\n";

  return 0;
}
