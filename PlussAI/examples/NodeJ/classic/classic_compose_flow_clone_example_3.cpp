#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow original;  // transform-only, never fit
  original.add_standard_scaler();
  std::cout << "original.trained() before clone = " << std::boolalpha << original.trained() << "\n";

  cls::Flow clone = original.clone().move_value();
  std::cout << "clone.trained()    = " << clone.trained() << "\n";

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  clone.learn_map(train);
  std::cout << "clone.trained() after fitting the clone = " << clone.trained() << "\n";
  std::cout << "original.trained() (untouched)           = " << original.trained() << "\n";

  return 0;
}
