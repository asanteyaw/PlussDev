#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <cmath>
#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const double NaN = std::nan("");

  // A numeric column with two gaps. The default stage fills them with the mean
  // of the observed values (here 32.5), then the estimator trains on the
  // completed column.
  const pluss::Series amount = pluss::make_series({10., 20., NaN, 40., NaN, 60.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::simple_imputer(), lymph::prep::route::names({"amount"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"amount", Slate(amount)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
