#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});

  const cls::StatusOr<cls::FeatureBatch> sliced = cls::slice_feature_batch(batch, pluss::series::undefined());
  std::cout << "ok = " << std::boolalpha << sliced.ok() << "\n";
  std::cout << "message = \"" << sliced.status().message << "\"\n";

  return 0;
}
