#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Eight rows whose two classes overlap in the middle (rows 3 and 4 carry
  // swapped labels), so no straight boundary fits them all and the metrics
  // land below a perfect 1.0.
  const pluss::Series income = pluss::make_series({1.0, 1.6, 2.2, 2.8, 3.0, 3.6, 4.2, 4.8}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.4, 0.8, 1.2, 1.5, 1.6, 2.0, 2.4, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 0, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x, y);

  // assess runs inference, compares against the given labels, and returns a
  // MetricReport. Ask for the metrics you intend to read; unrequested
  // fields stay undefined. Each requested field is a one-element Series.
  lymph::options::Assess options{};
  options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::Precision,
                     lymph::MetricKind::Recall, lymph::MetricKind::F1};
  const lymph::MetricReport report = model.assess(x, y, options);
  std::cout << "accuracy  = " << report.accuracy.item<double>() << "\n";
  std::cout << "precision = " << report.precision.item<double>() << "\n";
  std::cout << "recall    = " << report.recall.item<double>() << "\n";
  std::cout << "f1        = " << report.f1.item<double>() << "\n";

  return 0;
}
