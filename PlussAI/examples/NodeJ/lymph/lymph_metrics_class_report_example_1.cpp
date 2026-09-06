#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// A ClassReport holds one value per class for support / precision / recall /
// f1 / specificity, all aligned to `labels`. Multiclass here: three classes.
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

  std::cout << "labels    = " << report.labels << "\n";
  std::cout << "support   = " << report.support << "\n";
  std::cout << "precision = " << report.precision << "\n";
  std::cout << "recall    = " << report.recall << "\n";
  std::cout << "f1        = " << report.f1 << "\n";

  return 0;
}
