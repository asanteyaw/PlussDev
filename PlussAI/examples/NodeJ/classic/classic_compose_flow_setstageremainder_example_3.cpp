// compose::Flow::set_stage_remainder example 3 -- switching to Drop changes
// the mapped output width, and resets trained() like set_stage_route.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});  // column 1 passes through by default

  const pluss::Series x = pluss::make_series({1.0, 10.0, 2.0, 20.0, 3.0, 30.0}, {6}, f64).reshape({3, 2});
  cls::FeatureBatch batch{};
  batch.x = x;
  const pluss::Series pass_through = flow.learn_map(batch).value();
  std::cout << "PassThrough mapped = " << pass_through << "\n";

  flow.set_stage_remainder(0, cls::RouteRemainderPolicy::Drop);
  std::cout << "trained() after set_stage_remainder() = " << std::boolalpha << flow.trained() << "\n";
  const pluss::Series dropped = flow.learn_map(batch).value();
  std::cout << "Drop mapped = " << dropped << "\n";

  return 0;
}
