// Integrating samples you already have. `integ::trapezoid(y, x)` applies the trapezoid rule to values y at points x (any spacing,
// even descending), `integ::trapezoid(y, dx)` to equally spaced values, and `cumulative_trapezoid` returns the running integral.
// The default axis is the last one, so a 2-D Series integrates every row at once. Here: y = x^2 sampled at 0, 1, 2, 3, 4, whose
// exact integral over [0, 4] is 64/3 = 21.333; the trapezoid rule overestimates a convex function, and a finer grid shrinks the error.
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
  const Series x = vec({0, 1, 2, 3, 4});
  const Series y = x * x;
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "trapezoid(y, x)       : " << integ::trapezoid(y, x).item<double>() << "   (exact 21.3333)\n";
  std::cout << "trapezoid(y, dx = 1)  : " << integ::trapezoid(y, 1.0).item<double>() << "\n";
  const auto running = integ::cumulative_trapezoid(y, x).host<double>();
  std::cout << "cumulative            :";
  for (double v : running) std::cout << " " << v;
  std::cout << "   (exact x^3/3: 0.3333 2.6667 9.0000 21.3333)\n";

  const Series fine_x = Series::arange(0.0, 4.0 + 1e-12, 0.05, f64);
  std::cout << "81 points             : " << integ::trapezoid(fine_x * fine_x, fine_x).item<double>() << "\n";

  const Series rows = pluss::make_series(std::vector<double>{0, 1, 4, 9, 1, 2, 5, 10}, {2, 4}).to(f64);
  const auto per_row = integ::trapezoid(rows, vec({0, 1, 2, 3})).host<double>();
  std::cout << "two rows at once      : " << per_row[0] << ", " << per_row[1] << "\n";
}
