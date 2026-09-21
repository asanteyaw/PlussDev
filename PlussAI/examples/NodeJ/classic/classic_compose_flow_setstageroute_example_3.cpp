// compose::Flow::set_stage_route example 3 -- re-routing resets trained(),
// unlike set_stage_key.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});

  const pluss::Series x = pluss::make_series({1.0, 2.0, 2.0, 4.0, 3.0, 6.0}, {6}, f64).reshape({3, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const cls::StatusOr<pluss::Series> mapped = flow.learn_map(batch);
  std::cout << "learn_map ok = " << std::boolalpha << mapped.ok() << "\n";
  std::cout << "trained() after learn_map() = " << flow.trained() << "\n";

  flow.set_stage_route(0, {0, 1});
  std::cout << "trained() after set_stage_route() = " << flow.trained() << "\n";

  return 0;
}
