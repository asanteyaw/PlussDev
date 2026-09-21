#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series perfect_scores = pluss::make_series({0.9, 0.8, 0.3, 0.1}, {4}, f64);
  const pluss::Series perfect_labels = pluss::make_series({1.0, 1.0, 0.0, 0.0}, {4}, f64);
  std::cout << "perfect ranking AP = " << cls::average_precision(perfect_scores, perfect_labels).value() << "\n";

  const pluss::Series no_pos_scores = pluss::make_series({0.9, 0.5, 0.1}, {3}, f64);
  const pluss::Series no_pos_labels = pluss::make_series({0.0, 0.0, 0.0}, {3}, f64);
  std::cout << "no positives AP    = " << cls::average_precision(no_pos_scores, no_pos_labels).value() << "\n";

  return 0;
}
