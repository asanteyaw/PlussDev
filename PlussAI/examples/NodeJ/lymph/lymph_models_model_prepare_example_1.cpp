#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // Two features on very different scales: amounts in the thousands, ratios
  // near one.
  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // prepare() opens a fluent builder that records preparation onto the model.
  // Nothing is fitted yet -- these are intentions.
  model.prepare().scale({"amount", "ratio"});
  std::cout << "stage_count before learn = " << model.stage_count() << "\n";

  // learn resolves and fits the recorded stages, then fits the estimator.
  model.learn_(x, y);
  std::cout << "stage_count after learn  = " << model.stage_count() << "\n";
  std::cout << "trained                  = " << std::boolalpha << model.trained() << "\n\n";

  // infer takes the raw, unscaled columns; the fitted scaler runs inside.
  const DataFrame review({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"actual", Thread(y)},
      {"predicted", Thread(model.infer(x))},
  });
  std::cout << review;

  return 0;
}
