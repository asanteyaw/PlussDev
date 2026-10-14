// A nonlinear inequality constraint g(x) <= 0 keeps the answer on one side of a curve.
// Here the unconstrained minimum (1, 2) lies outside the disc x^2 + y^2 <= 1, so the
// solution lands on the circle, at the point of the disc closest to (1, 2).
#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  const auto target = vec({1.0, 2.0});
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); };

  const auto disc = optim::constraints().nl_ineq(
      [](const optim::ParameterPack<optim::Trace>& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });
  const auto result = optim::optima(objective, vec({0.0, 0.0}), {}, disc);

  const auto x = result.params.at("x").to_vec<double>();
  const double norm = std::sqrt(x[0] * x[0] + x[1] * x[1]);
  std::cout << "x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "distance from origin = " << norm << "\n";
  std::cout << "closest disc point to (1, 2) is (1, 2)/sqrt(5) = [" << 1.0 / std::sqrt(5.0) << ", "
            << 2.0 / std::sqrt(5.0) << "]\n";
  return 0;
}
