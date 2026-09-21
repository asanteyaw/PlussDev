#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <limits>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const double nan = std::numeric_limits<double>::quiet_NaN();

  // three features, all roughly equal to each other; two missing values in different rows.
  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {1.0, 1.0, 1.0,
       2.0, 2.0, 2.0,
       3.0, nan, 3.0,
       4.0, 4.0, nan}, {12}, f64).reshape({4, 3});

  auto fit_and_fill = [&](std::int64_t rounds) {
    cls::prep::IterativeImputerOptions options{};
    options.rounds = rounds;
    options.ridge = 1e-6;
    cls::prep::IterativeImputer imputer(options);
    return imputer.learn_map(train).value();
  };

  std::cout << "rounds=1:  " << fit_and_fill(1) << "\n";
  std::cout << "rounds=10: " << fit_and_fill(10) << "\n";

  return 0;
}
