#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// reports::threshold_curve(model, x, y) sweeps every achievable decision
// threshold and reports precision / recall / f1 at each -- the operating-
// point curve, with scores materialized for you.
int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame x({
      {"income", Thread(pluss::make_series(
          {1.0, 1.3, 1.7, 2.1, 2.4, 2.8, 3.2, 3.6, 4.0, 4.4}, {10}, f64))},
      {"balance", Thread(pluss::make_series(
          {0.4, 0.7, 1.0, 1.2, 1.5, 1.8, 2.1, 2.4, 2.7, 3.0}, {10}, f64))},
  });
  const Series y = pluss::make_series({0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.learn_(x, y);

  const lymph::ThresholdCurve curve = lymph::reports::threshold_curve(model, x, y);
  std::cout << "threshold count = " << curve.thresholds.numel() << "\n";
  std::cout << "thresholds[0..4] = " << curve.thresholds.slice(0, 0, 4) << "\n";
  std::cout << "precision[0..4]  = " << curve.precision.slice(0, 0, 4) << "\n";
  std::cout << "recall[0..4]     = " << curve.recall.slice(0, 0, 4) << "\n";
  std::cout << "f1[0..4]         = " << curve.f1.slice(0, 0, 4) << "\n";

  return 0;
}
