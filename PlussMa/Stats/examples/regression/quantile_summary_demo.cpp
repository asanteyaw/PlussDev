#include <pluss/ma.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      std::vector<double>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
      {12, 1},
      cfg);

  const pluss::Series y = pluss::make_series(
      std::vector<double>{1.0, 3.1, 5.3, 7.4, 9.2, 11.8, 13.9, 16.2, 18.0, 20.1, 22.6, 24.2},
      {12},
      cfg);

  pluss::ma::reg::QuantileOptions options;
  options.tau = 0.5;
  options.response_name = "target";
  options.predictor_names = {"feature"};

  const auto fit = pluss::ma::reg::quantile_fit(y, x, options, cfg);
  std::cout << pluss::ma::reg::quantile_summary(fit) << '\n';

  const pluss::Series x_test = pluss::make_series(std::vector<double>{2.5, 6.5, 10.5}, {3, 1}, cfg);
  const pluss::Series pred = pluss::ma::reg::quantile_predict(fit, x_test, cfg);
  std::cout << "Predictions\n" << pred << '\n';
  return 0;
}
