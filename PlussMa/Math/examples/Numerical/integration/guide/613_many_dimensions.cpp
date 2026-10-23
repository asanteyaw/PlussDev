// Integrals over a box. `quadra(fn, bounds)` takes an integrand of a list of coordinate Series (one per axis, each with all the nodes
// of the grid) and a list of [lo, hi] pairs; it builds the tensor-product rule. The cost is resolution^d, so it suits two or three
// dimensions. For more, `aleatoria` (Monte Carlo) has an error that does not depend on the dimension. Here: the 2-D integral of x y
// over the unit square (exactly 1/4), a 3-D one of x y z (1/8), and the same 3-D integral by Monte Carlo with its standard error.
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
  std::cout << std::fixed << std::setprecision(8);
  integ::IntegrandNdFn xy = [](const std::vector<Series>& c) { return c[0] * c[1]; };
  const auto r2 = integ::quadra(xy, {{0.0, 1.0}, {0.0, 1.0}});
  std::cout << "2-D quadrature : " << r2.value.item<double>() << "  (exact 0.25000000), " << r2.evaluations << " evaluations\n";
  integ::IntegrandNdFn xyz = [](const std::vector<Series>& c) { return c[0] * c[1] * c[2]; };
  const auto r3 = integ::quadra(xyz, {{0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}});
  std::cout << "3-D quadrature : " << r3.value.item<double>() << "  (exact 0.12500000), " << r3.evaluations << " evaluations\n";
  integ::AleatoriaOptions mc;
  mc.samples = 200000;
  mc.seed = 7;
  const auto m = integ::aleatoria(xyz, std::vector<std::pair<double, double>>{{0.0, 1.0}, {0.0, 1.0}, {0.0, 1.0}}, mc);
  std::cout << "3-D Monte Carlo: " << m.value.item<double>() << " +- " << m.standard_error << ", " << m.samples << " samples\n";
}
