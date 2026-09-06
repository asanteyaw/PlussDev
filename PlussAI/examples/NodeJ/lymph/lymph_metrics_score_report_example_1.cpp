#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// metrics::classification::score_report(scores, labels, {kinds}) computes the
// score-based metrics -- RocAuc, Gini, AveragePrecision, LogLoss, Brier --
// straight from a probability Series you already hold. No model, no inference.
int main() {
  namespace lymph = pluss::ai::lymph;
  namespace classification = lymph::metrics::classification;
  using pluss::ScalarType;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Positive-class probabilities and the matching 0/1 labels.
  const pluss::Series scores =
      pluss::make_series({0.9, 0.8, 0.7, 0.6, 0.5}, {5}, f64);
  const pluss::Series labels =
      pluss::make_series({1, 0, 1, 1, 0}, {5}, f64);

  const lymph::metrics::MetricReport report = classification::score_report(
      scores, labels,
      {lymph::MetricKind::RocAuc, lymph::MetricKind::Gini,
       lymph::MetricKind::AveragePrecision, lymph::MetricKind::LogLoss,
       lymph::MetricKind::Brier});

  std::cout << "roc_auc           = " << report.roc_auc.item<double>() << "\n";
  std::cout << "gini              = " << report.gini.item<double>() << "\n";
  std::cout << "average_precision = " << report.average_precision.item<double>() << "\n";
  std::cout << "logloss           = " << report.logloss.item<double>() << "\n";
  std::cout << "brier             = " << report.brier.item<double>() << "\n";
  std::cout << "values            = " << report.values << "\n";

  return 0;
}
