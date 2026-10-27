// Linear interpolation. `interp::linear(coordinate, values, query)` evaluates the piecewise-linear curve through the points
// (coordinate[i], values[i]) at the query points. `coordinate` must be one-dimensional and strictly increasing (not necessarily equally
// spaced); `query` is a scalar or a vector, and the result has the same shape as the query. Here: a table of the cooling of a
// cup of coffee at irregular times, read at a single time and at several.
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
  const Series minutes = vec({0.0, 2.0, 5.0, 10.0, 20.0});
  const Series temperature = vec({90.0, 78.0, 66.0, 52.0, 38.0});
  std::cout << std::fixed << std::setprecision(2);
  std::cout << "at 3.5 min : " << interp::linear(minutes, temperature, pluss::make_series(3.5).to(f64)).item<double>() << "\n";
  const auto several = interp::linear(minutes, temperature, vec({0.0, 1.0, 5.0, 7.5, 20.0})).host<double>();
  std::cout << "at 0, 1, 5, 7.5, 20 min :";
  for (double v : several) std::cout << " " << v;
  std::cout << "\n";
}
