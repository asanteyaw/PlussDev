// Linear constraints are given as matrices. ln_ineq(A, b) means A x <= b row by row, and
// ln_eq(A, b) means A x = b. Minimize |x - (2, 2, 2)|^2 subject to x1 + x2 + x3 = 3 (equality)
// and x1 - x2 <= -0.5 (inequality).
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto mat = [&](std::vector<double> v, std::int64_t rows, std::int64_t cols) {
    return pluss::make_series(v, {rows, cols}).to(f64);
  };

  const auto target = vec({2.0, 2.0, 2.0});
  auto objective = [&](const pluss::Series& x) { return (x - target).square().sum(); };

  const auto constraints = optim::constraints()
                               .ln_eq(mat({1, 1, 1}, 1, 3), vec({3.0}))
                               .ln_ineq(mat({1, -1, 0}, 1, 3), vec({-0.5}));
  const auto result = optim::optima(objective, vec({0.0, 0.0, 0.0}), {}, constraints);

  const auto x = result.params.at("x").host<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "x = [" << x[0] << ", " << x[1] << ", " << x[2] << "]\n";
  std::cout << "sum = " << x[0] + x[1] + x[2] << " (equality target 3)\n";
  std::cout << "x1 - x2 = " << x[0] - x[1] << " (inequality limit -0.5)\n";
  return 0;
}
