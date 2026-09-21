#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;
  flow.add_standard_scaler();

  cls::FeatureBatch first{};
  first.x = pluss::make_series({0.0, 10.0}, {2}, f64).reshape({2, 1});
  flow.learn_map(first);
  const pluss::Series first_center = flow.dump_transform_states().value()[0][1];

  cls::FeatureBatch second{};
  second.x = pluss::make_series({100.0, 200.0}, {2}, f64).reshape({2, 1});
  flow.learn_map(second);  // re-fits from scratch
  const pluss::Series second_center = flow.dump_transform_states().value()[0][1];

  std::cout << "center after first learn  = " << first_center << "\n";
  std::cout << "center after second learn = " << second_center << "\n";

  return 0;
}
