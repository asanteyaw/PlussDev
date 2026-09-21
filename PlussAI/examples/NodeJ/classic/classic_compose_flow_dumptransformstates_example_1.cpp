#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow original;  // transform-only
  original.add_standard_scaler({}, {0});
  original.add_one_hot_encoder({}, {1});

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 0.0, 20.0, 1.0, 30.0, 0.0}, {6}, f64).reshape({3, 2});
  original.learn_map(train);

  const std::vector<std::vector<pluss::Series>> states = original.dump_transform_states().value();
  std::cout << "stage count in dump = " << states.size() << "\n";
  std::cout << "stage 0 tensors     = " << states[0].size() << "\n";
  std::cout << "stage 1 tensors     = " << states[1].size() << "\n";

  return 0;
}
