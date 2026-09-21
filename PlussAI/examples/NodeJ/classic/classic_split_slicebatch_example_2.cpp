#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64);
  batch.sample_weights = pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64);

  const pluss::Config i64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Int64).with_platform(pluss::pCPU);
  const std::vector<std::int64_t> index_values{0, 2};
  const pluss::Series indices = pluss::make_series(index_values, {2}, i64);
  const cls::LabeledBatch sliced = cls::slice_labeled_batch(batch, indices).value();
  std::cout << "features.x       = " << sliced.features.x << "\n";
  std::cout << "y                = " << sliced.y << "\n";
  std::cout << "sample_weights   = " << sliced.sample_weights << "\n";

  return 0;
}
