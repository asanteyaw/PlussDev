// compose::Flow::clear_transforms example 2 -- resets trained() on a
// transform-only flow that had already been fit.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  cls::FeatureBatch batch{};
  batch.x = x;
  const cls::StatusOr<pluss::Series> mapped = flow.learn_map(batch);
  std::cout << "learn_map ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "trained() after learn_map() = " << flow.trained() << "\n";

  flow.clear_transforms();
  std::cout << "trained() after clear_transforms() = " << flow.trained() << "\n";
  std::cout << "stage_count() after clear_transforms() = " << flow.stage_count() << "\n";

  return 0;
}
