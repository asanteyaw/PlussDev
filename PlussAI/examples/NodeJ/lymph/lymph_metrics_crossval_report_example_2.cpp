#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// Beyond board(): report.report.folds is the per-fold detail, report.report.metrics
// the metric order, and report.mean / report.stdev are MetricReports holding the
// across-fold aggregates -- read them by name (report.mean.accuracy, ...).
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8,
       3.2, 3.6, 4.0, 4.4, 4.8, 3.4, 5.0, 3.8, 4.2, 3.0}, {20}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4,
       2.4, 2.8, 3.1, 3.4, 3.7, 1.8, 4.0, 3.6, 2.2, 3.2}, {20}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, {20}, f64);
  const DataFrame frame({{"income", Thread(income)}, {"balance", Thread(balance)},
                         {"target", Thread(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  lymph::AssessOptions assess{};
  assess.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1};

  const lymph::selection::Plan plan =
      lymph::selection::cross().folds(4).shuffle(true).seed(20260901ULL);

  const lymph::metrics::CrossValidationReport report =
      plan.run(lymph::models::logistic_regression(), frame, data_options, {}, assess);

  std::cout << "folds        = " << report.report.folds.size() << "\n";
  std::cout << "metrics      = " << report.report.metrics.size() << "\n";
  std::cout << "mean.accuracy  = " << report.mean.accuracy.item<double>() << "\n";
  std::cout << "stdev.accuracy = " << report.stdev.accuracy.item<double>() << "\n";
  std::cout << "mean.f1        = " << report.mean.f1.item<double>() << "\n";
  std::cout << "stdev.f1       = " << report.stdev.f1.item<double>() << "\n";

  return 0;
}
