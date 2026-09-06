#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Multiclass: classification_summary discovers all three labels and reports
// per-class rows plus the weighted average (support-weighted mean of the
// per-class precision / recall / f1).
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2}, {12}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 0, 1, 2, 1, 2, 2, 2, 0}, {12}, f64);

  const lymph::ClassReport report =
      lymph::metrics::classification::summary(truth, predicted);

  std::cout << "labels           = " << report.labels << "\n";
  std::cout << "support          = " << report.support << "\n";
  std::cout << "f1 per class     = " << report.f1 << "\n";
  std::cout << "weighted_average = " << report.weighted_average << "\n";

  return 0;
}
