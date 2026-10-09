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

  const pluss::Series income = pluss::make_series({1.0, 1.6, 2.2, 2.8, 3.4, 4.0, 4.6, 5.2}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.4, 0.9, 1.3, 1.6, 2.0, 2.4, 2.8, 3.2}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 0, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x, y);

  // AssessOptions.metrics restricts the report to a chosen subset (the other
  // fields stay undefined). positive_threshold moves the hard-label boundary
  // that Accuracy and F1 depend on, exactly as it does for infer.
  lymph::options::Assess options{};
  options.metrics = {lymph::MetricKind::Accuracy, lymph::MetricKind::F1, lymph::MetricKind::RocAuc};

  const lymph::MetricReport at_default = model.assess(x, y, options);

  options.positive_threshold = 0.8;
  const lymph::MetricReport at_strict = model.assess(x, y, options);

  std::cout << "threshold 0.5:  accuracy = " << at_default.accuracy.item<double>()
            << ", f1 = " << at_default.f1.item<double>()
            << ", roc_auc = " << at_default.roc_auc.item<double>() << "\n";
  std::cout << "threshold 0.8:  accuracy = " << at_strict.accuracy.item<double>()
            << ", f1 = " << at_strict.f1.item<double>()
            << ", roc_auc = " << at_strict.roc_auc.item<double>()
            << "   (roc_auc is threshold-free, so it does not move)\n";

  return 0;
}
