#pragma once
// INTERNAL: finite-difference weights on non-uniform grids, shared by the Heston PDE solver and its tests.
#include <cstddef>
#include <vector>

namespace pluss::subsp::math::finance::internal::pde {

struct Weights {
  double lo = 0.0, mid = 0.0, hi = 0.0;
};

// Central three-point weights of f' and f'' at a node with left spacing hm and right spacing hp.
inline Weights first_central(double hm, double hp) {
  return {-hp / (hm * (hm + hp)), (hp - hm) / (hm * hp), hm / (hp * (hm + hp))};
}
inline Weights second_central(double hm, double hp) {
  return {2.0 / (hm * (hm + hp)), -2.0 / (hm * hp), 2.0 / (hp * (hm + hp))};
}

// Forward three-point weights of f' at the first node: nodes x0, x1, x2 with spacings d1 = x1 - x0 and d2 = x2 - x1.
inline Weights first_forward(double d1, double d2) {
  return {-(2.0 * d1 + d2) / (d1 * (d1 + d2)), (d1 + d2) / (d1 * d2), -d1 / (d2 * (d1 + d2))};
}

// Backward three-point weights of f' at the last node: nodes x_{n-3}, x_{n-2}, x_{n-1} with spacings h2 = x_{n-2} - x_{n-3} and h1 = x_{n-1} - x_{n-2}.
// {lo, mid, hi} multiply f_{n-3}, f_{n-2}, f_{n-1}.
inline Weights last_backward(double h1, double h2) {
  return {h1 / (h2 * (h1 + h2)), -(h1 + h2) / (h1 * h2), (2.0 * h1 + h2) / (h1 * (h1 + h2))};
}

// Applies the central weights across a whole grid (one-sided two-point at the ends), for tests.
inline std::vector<double> apply_first(const std::vector<double>& x, const std::vector<double>& f) {
  const std::size_t n = x.size();
  std::vector<double> out(n);
  out[0] = (f[1] - f[0]) / (x[1] - x[0]);
  out[n - 1] = (f[n - 1] - f[n - 2]) / (x[n - 1] - x[n - 2]);
  for (std::size_t i = 1; i + 1 < n; ++i) {
    const auto w = first_central(x[i] - x[i - 1], x[i + 1] - x[i]);
    out[i] = w.lo * f[i - 1] + w.mid * f[i] + w.hi * f[i + 1];
  }
  return out;
}
inline std::vector<double> apply_second(const std::vector<double>& x, const std::vector<double>& f) {
  const std::size_t n = x.size();
  std::vector<double> out(n, 0.0);
  for (std::size_t i = 1; i + 1 < n; ++i) {
    const auto w = second_central(x[i] - x[i - 1], x[i + 1] - x[i]);
    out[i] = w.lo * f[i - 1] + w.mid * f[i] + w.hi * f[i + 1];
  }
  return out;
}

}  // namespace pluss::subsp::math::finance::internal::pde
