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

  // neighbor_imputer fills a gap from the most similar complete rows, using the
  // other columns to measure similarity.
  const pluss::Series h = pluss::make_series({1.,2.,3.,4.,5.,6.}, {6}, f64);
  const pluss::Series w = pluss::make_series({1.1, 2.0, NaN, 4.1, 5.0, NaN}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"h", Thread(h)}, {"w", Thread(w)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::neighbor_imputer(), lymph::prep::route::names({"h", "w"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"h", Thread(h)}, {"w", Thread(w)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
