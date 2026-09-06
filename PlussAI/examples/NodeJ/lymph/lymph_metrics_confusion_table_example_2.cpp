#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// ConfusionOptions.normalize = ByTruth divides each row by its truth total, so
// `counts` becomes per-true-class rates (diagonal = recall).
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 0, 1, 1, 0, 1, 1}, {10}, f64);

  const lymph::ConfusionTable raw = lymph::metrics::classification::confusion_table(truth, predicted);
  const lymph::ConfusionTable rates = lymph::metrics::classification::confusion_table(
      truth, predicted, lymph::ConfusionOptions{.normalize = lymph::ConfusionNormalize::ByTruth});

  std::cout << "raw counts   = " << raw.counts << "\n";
  std::cout << "row-normed   = " << rates.counts << "\n";
  std::cout << "normalize mode = " << static_cast<int>(rates.normalize) << "\n";

  return 0;
}
