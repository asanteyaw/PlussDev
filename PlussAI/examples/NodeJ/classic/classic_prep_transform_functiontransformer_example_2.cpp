#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::prep::FunctionTransformer transformer(
      [&](const pluss::Series& x) -> cls::StatusOr<pluss::Series> {
        (void)x;
        // returns one row no matter how many the input had -- a deliberately
        // broken callable, to demonstrate the guard it trips.
        return cls::StatusOr<pluss::Series>(pluss::make_series({1.0}, {1}, f64).reshape({1, 1}));
      });

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  const cls::StatusOr<pluss::Series> mapped = transformer.learn_map(batch);
  std::cout << "ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "message = \"" << mapped.status().message << "\"\n";

  return 0;
}
