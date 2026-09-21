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

  auto fit_offset = [&](double contamination) {
    cls::prep::OutlierIsolatorOptions options{};
    options.contamination_auto = false;
    options.contamination = contamination;
    cls::prep::OutlierIsolator isolator(options);
    isolator.learn(train);
    return isolator.decision_offset();
  };

  std::cout << "contamination_auto=true (default): offset = -0.5\n";
  std::cout << "contamination=0.1: offset = " << fit_offset(0.1) << "\n";
  std::cout << "contamination=0.5: offset = " << fit_offset(0.5) << "\n";

  return 0;
}
