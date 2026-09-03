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

  const pluss::Series x1 = pluss::make_series({1.,2.,3.,4.,5.,6.}, {6}, f64);
  const pluss::Series x2 = pluss::make_series({3.,6.,NaN,12.,15.,18.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0,1,2,0,1,2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"x1", Thread(x1)}, {"x2", Thread(x2)}, {"region", Thread(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::iterative_imputer(), lymph::prep::route::names({"x1", "x2"}))
       .append(lymph::prep::stage::minmax_scaler(), lymph::prep::route::names({"x1", "x2"}))
       .append(lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"x2", Thread(x2)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
