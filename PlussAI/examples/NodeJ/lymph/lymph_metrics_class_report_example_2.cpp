#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The averaged rows: accuracy is shape [1]; micro/macro/weighted_average are
// each shape [3] -> [precision, recall, f1].
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2}, {12}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 1, 1, 2, 1, 2, 2, 0, 2}, {12}, f64);

  const lymph::ClassReport report =
      lymph::metrics::classification::summary(truth, predicted);

  std::cout << "accuracy         = " << report.accuracy.item<double>() << "\n";
  std::cout << "macro_average    = " << report.macro_average << "\n";
  std::cout << "weighted_average = " << report.weighted_average << "\n";
  std::cout << "micro_average    = " << report.micro_average << "\n";

  return 0;
}
