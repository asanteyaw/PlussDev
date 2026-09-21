#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow original;
  original.add_standard_scaler();
  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  original.learn_map(train);
  const std::vector<std::vector<pluss::Series>> states = original.dump_transform_states().value();

  cls::Flow restored;
  restored.add_standard_scaler();  // same stage shape, freshly constructed
  std::cout << "trained() before restore = " << std::boolalpha << restored.trained() << "\n";
  restored.load_transform_states(states);
  restored.mark_trained(true);
  std::cout << "trained() after restore  = " << restored.trained() << "\n";

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({2.0}, {1}, f64).reshape({1, 1});
  std::cout << "map ok = " << restored.map(probe).ok() << "\n";

  return 0;
}
