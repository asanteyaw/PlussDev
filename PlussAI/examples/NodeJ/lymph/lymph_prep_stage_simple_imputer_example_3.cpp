#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <cmath>
#include <iostream>

int main() {

  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const double NaN = std::nan("");

  const pluss::Series a = pluss::make_series({10., 20., NaN, 40., 50., 60.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"a", Thread(a)}, {"region", Thread(region)}});

  // impute then scale the numeric column, one-hot the categorical one.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::simple_imputer(), lymph::prep::route::names({"a"}))
       .append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"a"}))
       .append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"a", Thread(a)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
