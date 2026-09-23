#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 0: 6 rows, class 1: 3 rows, class 2: 1 row -- 10 total.
  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0}, {10}, f64);

  const cls::ClassMix mix = cls::class_mix(labels).value();
  std::cout << "labels = " << mix.labels << "\n";
  std::cout << "counts = " << mix.counts << "\n";
  std::cout << "proportions (expect [0.6, 0.3, 0.1]) = " << mix.proportions << "\n";
  std::cout << "inverse_frequency (expect [10/(3*6), 10/(3*3), 10/(3*1)] = [0.5556, 1.1111, 3.3333]) = " << mix.inverse_frequency << "\n";
  std::cout << "sample_count = " << mix.sample_count << "\n";
  std::cout << "class_count = " << mix.class_count << "\n";
  std::cout << "imbalance_ratio (expect 6/1 = 6.0) = " << mix.imbalance_ratio << "\n";

  return 0;
}
