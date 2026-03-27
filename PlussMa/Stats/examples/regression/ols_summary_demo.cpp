#include <pluss/ma.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      std::vector<double>{
          1.0, 2.0,
          2.0, 1.0,
          3.0, 0.0,
          4.0, 2.0,
          5.0, 1.0,
          6.0, 3.0,
          7.0, 4.0,
          8.0, 5.0,
      },
      {8, 2},
      cfg);

  const pluss::Series y =
      pluss::make_series(std::vector<double>{0.9, 4.9, 7.2, 6.7, 10.0, 10.8, 10.6, 11.9}, {8}, cfg);

  pluss::ma::reg::OLSOptions options;
  options.response_name = "target";
  options.predictor_names = {"feat1", "feat2"};

  const auto fit = pluss::ma::reg::ols_fit(y, x, options, cfg);
  std::cout << pluss::ma::reg::ols_summary(fit) << '\n';

  const pluss::Series x_test = pluss::make_series(std::vector<double>{9.0, 6.0, 10.0, 7.0}, {2, 2}, cfg);
  const pluss::Series pred = pluss::ma::reg::ols_predict(fit, x_test, cfg);

  std::cout << "Predictions\n";
  std::cout << pred << '\n';

  return 0;
}
