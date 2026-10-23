// How accurate is it? Linear interpolation of a smooth function has error proportional to the square of the grid spacing h: halving h
// divides the worst error by 4. Here: sin(x) on [0, pi] sampled with 5, 9, 17, 33 and 65 equally spaced points and read at 1000 points;
// the table shows the worst error and the ratio between consecutive rows.
#include <pluss/ma.h>

#include <cmath>
#include <complex>
#include <iomanip>
#include <iostream>

namespace interp = pluss::subsp::math::interp;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const double pi = 3.14159265358979323846;
  const Series queries = Series::arange(0.0, pi, pi / 1000.0, f64);
  std::cout << std::left << std::setw(10) << "points" << std::setw(16) << "worst error" << "ratio\n";
  double previous = 0.0;
  for (int n : {5, 9, 17, 33, 65}) {
    const Series x = Series::arange(0.0, pi + 1e-12, pi / (n - 1), f64);
    const auto r = interp::linear(x, x.sin(), queries);
    const double worst = (r - queries.sin()).abs().amax().item<double>();
    std::cout << std::left << std::setw(10) << n << std::scientific << std::setprecision(2) << std::setw(16) << worst;
    if (previous > 0.0) std::cout << std::fixed << std::setprecision(2) << previous / worst;
    std::cout << "\n";
    previous = worst;
  }
}
