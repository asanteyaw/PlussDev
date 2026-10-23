// Many integrals in one call, and their derivatives. If the integrand returns shape [B, n] for a node vector of length n, `quadra`
// integrates along the last axis and returns B values: B independent integrals share the nodes. And because the integral is built
// from ordinary differentiable operations, a parameter captured by the integrand receives its derivative through `reverse()`.
// Here: I(k) = integral_0^1 k x^2 dx = k/3 for four values of k at once, and dI/dk = 1/3 for each, from one reverse pass.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
const double pi = 3.14159265358979323846;
const char* rule_name(integ::QuadraRule r) {
  switch (r) { case integ::QuadraRule::Trapezoidal: return "Trapezoidal"; case integ::QuadraRule::Simpson: return "Simpson"; case integ::QuadraRule::GaussLegendre: return "GaussLegendre"; default: return "Auto"; }
}
int main() {
  Series k = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4, 1}).to(f64);
  k.tracks_slope_(true);
  integ::IntegrandFn f = [&](const Series& x) { return (x * x).reshape({1, x.numel()}) * k; };
  integ::QuadraOptions o;
  o.rule = integ::QuadraRule::GaussLegendre;
  const auto r = integ::quadra(f, 0.0, 1.0, o);
  const auto v = r.value.to_vec<double>();
  std::cout << std::fixed << std::setprecision(6) << "I(k) = k/3   : " << v[0] << " " << v[1] << " " << v[2] << " " << v[3] << "   (shape " << r.value.numel() << ")\n";
  r.value.sum().reverse();
  const auto g = k.sensitivity().to_vec<double>();
  std::cout << "dI/dk each   : " << g[0] << " " << g[1] << " " << g[2] << " " << g[3] << "   (exact 0.333333)\n";
}
