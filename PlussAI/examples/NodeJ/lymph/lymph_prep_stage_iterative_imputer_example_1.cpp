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

  // iterative_imputer models each column with gaps as a ridge regression on the
  // others and repeats for a fixed number of rounds.
  const pluss::Series a = pluss::make_series({1.,2.,3.,4.,5.,6.,7.,8.}, {8}, f64);
  const pluss::Series b = pluss::make_series({2., 4., NaN, 8., 10., 12., NaN, 16.}, {8}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,0,1,1,1,1}, {8}, f64);
  const DataFrame x({{"a", Thread(a)}, {"b", Thread(b)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::iterative_imputer(), lymph::prep::route::names({"a", "b"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"b", Thread(b)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
