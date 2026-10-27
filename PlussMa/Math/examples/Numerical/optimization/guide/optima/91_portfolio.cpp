// A minimum-variance portfolio of four assets. The unknowns are the weights. The risk is the quadratic form w' S w for a
// covariance matrix S; the weights must sum to 1 (a linear equality), the expected return must reach a target (a linear
// inequality) and no asset may exceed 60% or go below zero (bounds). The program solves it, checks every requirement, and
// then reads the multiplier of the return constraint as a price: moving the target by a small amount should move the
// minimum variance by about the multiplier times that amount, and re-solving at targets just above and just below
// shows it does.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

namespace optim = pluss::subsp::math::optim;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(f64); }

pluss::Series covariance() {
  return mat({0.10, 0.02, 0.04, 0.00,
              0.02, 0.08, 0.01, 0.02,
              0.04, 0.01, 0.20, 0.03,
              0.00, 0.02, 0.03, 0.06}, 4, 4);
}
const std::vector<double> expected = {0.12, 0.10, 0.15, 0.05};

optim::OptimaState solve(double target) {
  auto risk = [](const pluss::Series& w) {
    const auto column = w.reshape({4, 1});  // matmul wants matrices
    return (column * covariance().matmul(column)).sum();
  };
  // The return constraint expected . w >= target is written as -expected . w <= -target.
  const auto constraints = optim::constraints()
                               .lbs(vec({0, 0, 0, 0}))
                               .ubs(vec({0.6, 0.6, 0.6, 0.6}))
                               .ln_eq(mat({1, 1, 1, 1}, 1, 4), vec({1.0}))
                               .ln_ineq(mat({-0.12, -0.10, -0.15, -0.05}, 1, 4), vec({-target}));
  return optim::optima(risk, vec({0.25, 0.25, 0.25, 0.25}), {}, constraints);
}

int main() {
  const auto r = solve(0.09);
  const auto w = r.params.at("x").host<double>();
  double total = 0.0, ret = 0.0;
  for (std::size_t i = 0; i < 4; ++i) { total += w[i]; ret += expected[i] * w[i]; }
  std::cout << std::fixed << std::setprecision(4) << "exit flag " << static_cast<int>(r.exit_flag) << "\n";
  std::cout << "weights: " << w[0] << " " << w[1] << " " << w[2] << " " << w[3] << "\n";
  std::cout << "sum of weights " << total << ", expected return " << ret << " (target 0.0900)\n";
  std::cout << "variance " << std::setprecision(6) << r.evaluation.value.item<double>() << "\n";

  const double price = r.multipliers.linear_inequalities.host<double>()[0];
  const double above = solve(0.0905).evaluation.value.item<double>();
  const double below = solve(0.0895).evaluation.value.item<double>();
  std::cout << std::setprecision(4) << "\nmultiplier of the return constraint " << price << "\n";
  std::cout << "change in variance per unit of target, by re-solving at 0.0905 and 0.0895: " << (above - below) / 0.001 << "\n";
  return 0;
}
