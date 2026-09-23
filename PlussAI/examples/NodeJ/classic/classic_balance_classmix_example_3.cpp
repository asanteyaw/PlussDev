#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // 5.0 appears first in the data, but 1.0 is the smaller value.
  const pluss::Series labels = pluss::make_series({5.0, 1.0, 5.0, 3.0, 1.0}, {5}, f64);
  const cls::ClassMix mix = cls::class_mix(labels).value();
  std::cout << "labels (expect [1, 3, 5], sorted ascending) = " << mix.labels << "\n";
  std::cout << "counts (expect [2, 1, 2], matching that order) = " << mix.counts << "\n";

  return 0;
}
