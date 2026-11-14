#include <pluss/ma.h>

#include <iostream>

namespace regression = pluss::ma::regression;

int main() {
  const pluss::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  const pluss::Series y = pluss::make_series({2.1, 4.0, 6.2, 8.1, 10.0, 12.2});

  regression::OLSOptions options;
  options.response_name = "sales";
  options.predictor_names = {"campaign"};
  const regression::OLSFitResult fit = regression::ols_fit(y, x, options);

  std::cout << regression::ols_summary(fit);
  std::cout << "\n== streamed directly ==\n";
  regression::print_ols_summary(std::cout, fit);
}
