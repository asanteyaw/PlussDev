#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;
  flow.add_target_encoder();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 1.0, 2.0, 2.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({0.0, 0.0, 10.0, 10.0}, {4}, f64);
  flow.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({99.0}, {1}, f64).reshape({1, 1});
  std::cout << "unseen category (any unknown_categorical) -> " << flow.map(probe).value() << "\n";

  return 0;
}
