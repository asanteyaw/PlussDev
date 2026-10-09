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

  // Eight applicants: two numeric features and a 0/1 default outcome.
  const pluss::Series income = pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64);
  const pluss::Series balance = pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"income", Slate(income)}, {"balance", Slate(balance)}});

  // A logistic-regression template: an algorithm choice, nothing learned yet.
  lymph::models::Model model = lymph::models::logistic_regression();
  std::cout << "before: trained = " << std::boolalpha << model.trained() << "\n";

  // learn_ fits in place; the name `model` now IS the fitted artifact.
  model.learn_(x, y);
  std::cout << "after:  trained = " << model.trained() << "\n\n";

  // Inference reuses the same feature contract training saw.
  const pluss::Series predicted = model.infer(x);
  const DataFrame review({
      {"income", Slate(income)},
      {"balance", Slate(balance)},
      {"actual", Slate(y)},
      {"predicted", Slate(predicted)},
  });
  std::cout << review;

  return 0;
}
