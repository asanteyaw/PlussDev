#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// model.reports() returns a ModelReports accessor bound to that model. It
// bundles every model-backed evaluation helper, so you evaluate one model
// against several datasets without re-passing it each time.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"income", Slate(pluss::make_series(
          {1.0, 1.3, 1.7, 2.1, 2.4, 2.8, 3.2, 3.6, 4.0, 4.4}, {10}, f64))},
      {"balance", Slate(pluss::make_series(
          {0.4, 0.7, 1.0, 1.2, 1.5, 1.8, 2.1, 2.4, 2.7, 3.0}, {10}, f64))},
  });
  const Series y = pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  const lymph::reports::Model r = model.reports();

  const lymph::ClassReport summary = r.classification_summary(x, y);
  std::cout << "accuracy       = " << summary.accuracy.item<double>() << "\n";

  const lymph::ConfusionTable table = r.confusion_table(x, y);
  std::cout << "confusion counts =\n" << table.counts << "\n";

  const lymph::ThresholdCurve curve = r.threshold_curve(x, y);
  std::cout << "threshold count = " << curve.thresholds.numel() << "\n";

  std::cout << "best_threshold(F1) = " << r.best_threshold(x, y, lymph::MetricKind::F1) << "\n";
  std::cout << "r.model().trained() = " << std::boolalpha << r.model().trained() << "\n";

  return 0;
}
