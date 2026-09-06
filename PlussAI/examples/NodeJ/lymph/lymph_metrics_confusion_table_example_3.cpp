#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// The {label_order} overload pins the row/column order explicitly instead of
// letting the table discover it -- useful when a class is absent from one of
// the two Series, or when a fixed reporting order matters.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({2, 2, 0, 0, 1, 1, 2, 0, 1, 2}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({2, 0, 0, 0, 1, 2, 2, 1, 1, 2}, {10}, f64);

  const lymph::ConfusionTable table =
      lymph::metrics::classification::confusion_table(truth, predicted, {2.0, 1.0, 0.0});

  std::cout << "labels (pinned order) = " << table.labels << "\n";
  std::cout << "counts                = " << table.counts << "\n";

  return 0;
}
