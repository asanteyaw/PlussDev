#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

// reports::best_threshold(model, x, y, objective) returns the single cutoff
// that maximizes `objective` over the whole sweep. Different objectives pick
// different operating points.
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

  const double best_f1 = lymph::reports::best_threshold(model, x, y, lymph::MetricKind::F1);
  const double best_recall = lymph::reports::best_threshold(model, x, y, lymph::MetricKind::Recall);
  const double best_precision =
      lymph::reports::best_threshold(model, x, y, lymph::MetricKind::Precision);

  std::cout << "best_threshold(F1)        = " << best_f1 << "\n";
  std::cout << "best_threshold(Recall)    = " << best_recall << "\n";
  std::cout << "best_threshold(Precision) = " << best_precision << "\n";

  return 0;
}
