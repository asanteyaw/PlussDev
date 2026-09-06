#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// confusion_table(truth, pred) returns a ConfusionTable: the discovered
// `labels`, the flattened row-major `counts` matrix (truth on rows, prediction
// on columns), and the `normalize` mode it was built with.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 0, 1, 1, 0, 1, 1}, {10}, f64);

  const lymph::ConfusionTable table =
      lymph::metrics::classification::confusion_table(truth, predicted);

  std::cout << "labels = " << table.labels << "\n";
  std::cout << "counts (row-major, truth x pred) = " << table.counts << "\n";
  std::cout << "normalize = " << static_cast<int>(table.normalize) << "\n";

  return 0;
}
