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

  // quantile_map re-maps a column onto its own rank distribution -- uniform by
  // default. Skew and outliers are flattened out.
  const pluss::Series amount = pluss::make_series({1., 2., 3., 4., 5., 500.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::quantile_map(), lymph::prep::route::names({"amount"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"amount", Thread(amount)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
