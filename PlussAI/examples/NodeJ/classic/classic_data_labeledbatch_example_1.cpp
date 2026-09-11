// data::LabeledBatch example 1 -- building by hand, with and without weights.

#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LabeledBatch unweighted{};
  unweighted.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  unweighted.y = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  std::cout << "unweighted.has_labels() = " << std::boolalpha << unweighted.has_labels() << "\n";
  std::cout << "unweighted.has_weights() = " << unweighted.has_weights() << "\n";

  cls::LabeledBatch weighted = unweighted;
  weighted.sample_weights = pluss::make_series({1.0, 1.0, 1.0, 3.0}, {4}, f64);
  std::cout << "weighted.has_weights() = " << weighted.has_weights() << "\n";

  const cls::LabeledBatch empty{};
  std::cout << "empty.has_labels() = " << empty.has_labels() << "\n";

  return 0;
}
