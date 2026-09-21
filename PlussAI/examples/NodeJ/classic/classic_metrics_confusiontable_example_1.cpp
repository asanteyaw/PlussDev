#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0}, {9}, f64);
  const pluss::Series pred = pluss::make_series(
      {0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 2.0, 2.0, 2.0}, {9}, f64);

  const cls::ConfusionTable table = cls::confusion_table(truth, pred).value();
  std::cout << "labels = " << table.labels << "\n";
  std::cout << "counts = " << table.counts << "\n";

  cls::ConfusionOptions normalized{};
  normalized.normalize = cls::ConfusionNormalize::ByTruth;
  const cls::ConfusionTable by_truth = cls::confusion_table(truth, pred, normalized).value();
  std::cout << "ByTruth counts (diagonal = per-class recall) = " << by_truth.counts << "\n";

  return 0;
}
