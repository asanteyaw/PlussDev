#include <pluss/ma.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const auto x = pluss::make_series(
      std::vector<double>{-2.0, -1.5, -1.0, -0.5, 0.0, 0.5, 1.0, 1.5, 2.0},
      {9, 1},
      cfg);
  const auto y = pluss::make_series(
      std::vector<double>{-0.2, -0.4, -0.5, -0.3, 0.2, 1.0, 2.1, 3.5, 5.2},
      {9},
      cfg);

  pluss::subsp::stats::regression::PolyOptions opt;
  opt.degree = 3;
  opt.predictor_names = {"feature"};
  const auto fit = pluss::subsp::stats::regression::poly_fit(y, x, opt, cfg);

  std::cout << pluss::subsp::stats::regression::poly_summary(fit) << '\n';
  std::cout << "Predictions:\n"
            << pluss::subsp::stats::regression::poly_predict(
                   fit,
                   pluss::make_series(std::vector<double>{-1.25, 0.25, 1.75}, {3, 1}, cfg),
                   cfg)
            << '\n';
  return 0;
}
