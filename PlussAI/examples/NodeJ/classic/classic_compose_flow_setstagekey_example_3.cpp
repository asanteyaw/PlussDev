// compose::Flow::set_stage_key example 3 -- renaming (or clearing, with "")
// a stage's key does NOT reset trained(), unlike set_stage_route /
// set_stage_remainder.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  flow.set_stage_key(0, "income_scaler");

  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  cls::FeatureBatch batch{};
  batch.x = x;
  const cls::StatusOr<pluss::Series> mapped = flow.learn_map(batch);
  std::cout << "learn_map ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "trained() after learn_map() = " << flow.trained() << "\n";

  flow.set_stage_key(0, "");  // clear the name
  std::cout << "stage_key after clearing = \"" << flow.transform_specs().front().stage_key << "\"\n";
  std::cout << "trained() after set_stage_key() = " << flow.trained() << "\n";

  return 0;
}
