// What a jump does to the state depends on the coefficient h: the Marcus rule moves the state along the flow of
// dy/du = h(y) z for u in [0, 1]. Three coefficients, each with a closed-form flow:
//   h = 1          : y + z                       (additive jump)
//   h = y          : y exp(z)                    (multiplicative jump; log y moves by exactly z)
//   h = y (1 - y)  : y / (y + (1 - y) exp(-z))   (logistic: the state stays inside (0, 1))
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const pluss::Series y = pluss::make_series(std::vector<double>{0.2, 0.5, 0.9}, {3}).to(f64);
  const pluss::Series z = pluss::make_series(std::vector<double>{1.5, -2.0, 3.0}, {3}).to(f64);
  const std::vector<double> yv = {0.2, 0.5, 0.9}, zv = {1.5, -2.0, 3.0};

  const auto add = y.calc.diff.marcus_flow([](const pluss::Series& s) { return s.mul(0.0).add(1.0); }, z).to_vec<double>();
  const auto mul = y.calc.diff.marcus_flow([](const pluss::Series& s) { return s; }, z).to_vec<double>();
  const auto logi = y.calc.diff.marcus_flow([](const pluss::Series& s) { return s.mul(s.neg().add(1.0)); }, z).to_vec<double>();

  std::cout << std::fixed << std::setprecision(6);
  for (int i = 0; i < 3; ++i) {
    std::cout << "y = " << yv[i] << ", z = " << zv[i] << ":  additive " << add[i] << " (" << yv[i] + zv[i] << ")  multiplicative "
              << mul[i] << " (" << yv[i] * std::exp(zv[i]) << ")  logistic " << logi[i] << " ("
              << yv[i] / (yv[i] + (1 - yv[i]) * std::exp(-zv[i])) << ")\n";
  }
  return 0;
}
