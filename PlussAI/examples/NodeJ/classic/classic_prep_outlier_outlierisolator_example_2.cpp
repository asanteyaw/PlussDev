#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {0.0, 0.1, -0.1, 0.05, -0.05, 0.2, -0.2, 0.0, 0.1, 100.0},
      {10}, f64).reshape({10, 1});

  cls::prep::OutlierIsolator isolator;
  const pluss::Series labels = isolator.learn_infer(train).value();
  std::cout << "decision_offset() = " << isolator.decision_offset() << "\n";
  std::cout << "labels = " << labels << "\n";

  return 0;
}
