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

  const pluss::Series income = pluss::make_series({1.0, 1.4, 1.8, 3.4, 3.8, 4.2}, {6}, f64);
  const pluss::Series balance = pluss::make_series({0.3, 0.5, 0.7, 1.8, 2.1, 2.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x_train({{"income", Slate(income)}, {"balance", Slate(balance)}});

  const lymph::models::Model model = lymph::models::logistic_regression().learn(x_train, y);

  // Predict for three applicants the model has never seen.
  const pluss::Series new_income = pluss::make_series({1.2, 2.6, 4.0}, {3}, f64);
  const pluss::Series new_balance = pluss::make_series({0.4, 1.2, 2.2}, {3}, f64);
  const DataFrame x_new({{"income", Slate(new_income)}, {"balance", Slate(new_balance)}});

  const DataFrame result({
      {"income", Slate(new_income)},
      {"balance", Slate(new_balance)},
      {"predicted_label", Slate(model.infer(x_new))},
  });
  std::cout << result;

  return 0;
}
