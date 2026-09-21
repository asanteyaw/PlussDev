#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FunctionTransformer transformer(
      [](const pluss::Series& x) -> cls::StatusOr<pluss::Series> { return cls::StatusOr<pluss::Series>(x.abs()); });

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({-3.0, 2.0, -1.0}, {3}, f64).reshape({3, 1});
  std::cout << "map = " << transformer.learn_map(batch).value() << "\n";

  return 0;
}
