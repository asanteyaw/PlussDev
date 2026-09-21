#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow original;
  original.add_standard_scaler({}, {0});
  original.add_one_hot_encoder({}, {1});
  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 0.0, 20.0, 1.0}, {4}, f64).reshape({2, 2});
  original.learn_map(train);
  const std::vector<std::vector<pluss::Series>> states = original.dump_transform_states().value();

  cls::Flow one_stage;
  one_stage.add_standard_scaler();  // only 1 stage -- states has 2
  const cls::Status loaded = one_stage.load_transform_states(states);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.message << "\"\n";

  return 0;
}
