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

  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"region", Thread(region)},
  });

  // A standard-scaler stage is one step among several. Here it scales the two
  // numeric columns while a one-hot stage handles the categorical one. The
  // fully qualified factory spelling: lymph::prep::stage::standard_scaler.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append({
      {lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"amount", "ratio"})},
      {lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"})},
  });
  model.learn_(x, y);

  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  const DataFrame review({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"region", Thread(region)},
      {"actual", Thread(y)},
      {"predicted", Thread(model.infer(x))},
  });
  std::cout << review;

  return 0;
}
