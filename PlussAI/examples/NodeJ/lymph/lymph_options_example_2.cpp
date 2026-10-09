// options example 2 -- a model-family options struct. options::Linear carries
// the linear-model hyperparameters; regularization_strength shrinks the fitted
// slope toward zero, so a large value pulls predictions toward the mean.

#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  // y = 10 * x, mean(y) = 55.
  const DataFrame x({
      {"x", Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {10}, f64))},
  });
  const Series y = pluss::make_series(
      {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0}, {10}, f64);
  const DataFrame probe({{"x", Slate(pluss::make_series({10.0}, {1}, f64))}});

  lymph::options::Linear light{};
  light.regularization = lymph::RegularizationKind::L2;
  light.regularization_strength = 1e-6;      // effectively ordinary least squares

  lymph::options::Linear heavy{};
  heavy.regularization = lymph::RegularizationKind::L2;
  heavy.regularization_strength = 1e3;       // strong shrinkage

  lymph::models::Model ols = lymph::models::linear_regression(light);
  ols.learn_(x, y);
  lymph::models::Model ridge = lymph::models::linear_regression(heavy);
  ridge.learn_(x, y);

  std::cout << "true value at x = 10                 : 100\n";
  std::cout << "light (strength 1e-6) prediction     : " << ols.infer(probe).item<double>() << "\n";
  std::cout << "heavy (strength 1e3)  prediction     : " << ridge.infer(probe).item<double>() << "\n";

  return 0;
}
