#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// summary(truth, pred, ClassReportOptions{...}) -- here zero_division = 1.0,
// which is the value precision/recall take when their denominator is empty.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classification = lymph::metrics::classification;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // The model never predicts class 1, so class 1's precision denominator is 0.
  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {10}, f64);

  const lymph::ClassReportOptions opts{.zero_division = 1.0};
  const lymph::ClassReport summary = classification::summary(truth, predicted, opts);

  std::cout << "labels    = " << summary.labels << "\n";
  std::cout << "precision = " << summary.precision << "  (class 1 uses zero_division)\n";
  std::cout << "recall    = " << summary.recall << "\n";

  return 0;
}
