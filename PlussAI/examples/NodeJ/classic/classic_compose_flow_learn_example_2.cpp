#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;  // empty -- no model
  flow.add_standard_scaler();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({0.0, 0.0, 0.0, 0.0}, {4}, f64);  // ignored -- no model to fit

  const cls::Status learned = flow.learn(batch);
  std::cout << "learn ok    = " << std::boolalpha << learned.ok() << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";
  std::cout << "trained()   = " << flow.trained() << "\n";

  return 0;
}
