#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;  // no model -- transform-only
  flow.add_standard_scaler({}, {0});
  flow.add_one_hot_encoder({}, {1});

  // column 0: numeric; column 1: a 2-category code (0 or 1).
  const cls::Series x = pluss::make_series(
      {10.0, 0.0, 20.0, 1.0, 30.0, 0.0}, {6}, f64).reshape({3, 2});
  cls::FeatureBatch batch{};
  batch.x = x;

  flow.learn_map(batch);  // fit both stages first
  const cls::StatusOr<pluss::Series> mapped = flow.map(batch);
  std::cout << "map ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "mapped = " << mapped.value() << "\n";

  return 0;
}
