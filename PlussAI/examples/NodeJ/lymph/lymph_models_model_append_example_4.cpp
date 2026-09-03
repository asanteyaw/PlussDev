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

  lymph::models::Model model = lymph::models::logistic_regression();

  // Bulk append: one call, a braced list of {stage, route} pairs, applied in
  // order. Equivalent to three chained append() calls but without the chain.
  model.append({
      {lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"amount"})},
      {lymph::prep::stage::minmax_scaler(), lymph::prep::route::names({"ratio"})},
      {lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"})},
  });
  std::cout << "stage_count before learn = " << model.stage_count() << "\n";

  model.learn_(x, y);
  std::cout << "stage_count after learn  = " << model.stage_count() << "\n";
  std::cout << "trained                  = " << std::boolalpha << model.trained() << "\n\n";

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
