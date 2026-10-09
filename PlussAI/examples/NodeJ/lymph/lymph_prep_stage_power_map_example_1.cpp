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

  // power_map applies a fitted Yeo-Johnson power transform to make a skewed
  // column more symmetric, then standardises it.
  const pluss::Series income = pluss::make_series({1., 2., 4., 8., 16., 64.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"income", Slate(income)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::power_map(), lymph::prep::route::names({"income"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"income", Slate(income)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
