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

  // polynomial_lift adds squared and interaction terms so a linear model can fit
  // a curved boundary. Two inputs at degree 2 become five columns.
  const pluss::Series a = pluss::make_series({-2., -1., 0., 1., 2., 3.}, {6}, f64);
  const pluss::Series b = pluss::make_series({1., 0., -1., 0., 1., 2.}, {6}, f64);
  const pluss::Series y = pluss::make_series({1, 0, 0, 0, 1, 1}, {6}, f64);
  const DataFrame x({{"a", Thread(a)}, {"b", Thread(b)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::polynomial_lift(), lymph::prep::route::names({"a", "b"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"a", Thread(a)}, {"b", Thread(b)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
