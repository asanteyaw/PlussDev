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
  const pluss::Series first_mapped = flow.learn_map(first).value();

  cls::FeatureBatch second{};
  second.x = pluss::make_series({100.0, 300.0}, {2}, f64).reshape({2, 1});
  const pluss::Series second_mapped = flow.learn_map(second).value();

  std::cout << "first learn_map  = " << first_mapped << "\n";
  std::cout << "second learn_map = " << second_mapped << "\n";

  return 0;
}
