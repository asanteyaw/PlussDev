#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  const cls::ClassMix mix = cls::class_mix(labels).value();
  std::cout << "imbalance_ratio = " << mix.imbalance_ratio << "\n";
  std::cout << "inverse_frequency = " << mix.inverse_frequency << "\n";

  return 0;
}
