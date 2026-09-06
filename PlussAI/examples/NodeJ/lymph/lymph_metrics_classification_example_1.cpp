#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// metrics::classification:: groups the label-pair helpers: confusion_table(),
// summary(), report(). Each takes the truth / prediction Series directly.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classification = lymph::metrics::classification;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series truth =
      pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const pluss::Series predicted =
      pluss::make_series({0, 0, 0, 1, 0, 1, 1, 0, 1, 1}, {10}, f64);

  const lymph::ConfusionTable table = classification::confusion_table(truth, predicted);
  const lymph::ClassReport summary = classification::summary(truth, predicted);
  const pluss::Series values =
      classification::report(truth, predicted,
                             {lymph::MetricKind::Accuracy, lymph::MetricKind::F1})
          .values;

  std::cout << "confusion counts = " << table.counts << "\n";
  std::cout << "summary.f1       = " << summary.f1 << "\n";
  std::cout << "values (acc, f1) = " << values << "\n";

  return 0;
}
