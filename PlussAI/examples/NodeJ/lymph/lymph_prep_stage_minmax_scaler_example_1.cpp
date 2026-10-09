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

  // Two features on unrelated scales: an amount in the thousands, a ratio near 1.
  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // The default stage rescales each routed column into [0, 1]. No options
  // argument means a default-constructed lymph::options::MinMaxScaler.
  model.append(lymph::prep::stage::minmax_scaler(),
               lymph::prep::route::names({"amount", "ratio"}));
  model.learn_(x, y);

  std::cout << "stage_count = " << model.stage_count() << "\n";
  std::cout << "trained     = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"amount", Slate(amount)},
      {"ratio", Slate(ratio)},
      {"actual", Slate(y)},
      {"predicted", Slate(model.infer(x))},
  });
  std::cout << review;

  return 0;
}
