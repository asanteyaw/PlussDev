// Many systems at once. Axes in front of `solve_dims` are independent lanes: a right-hand side of shape [B, n] solves B systems with the same
// operator in one call, all using the same array operations. `converged` and `relative_residual` are then per lane. The loop runs until every lane
// is done or the iteration cap is reached; `check_interval` (default 8) is how often the lanes are tested, and 0 runs the whole iteration budget
// without a host check. Here: the same Laplacian system with four right-hand sides of different sizes (b scaled by 1, 10, 100, 1000), whose
// solutions scale in the same way.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace krylov = pluss::subsp::math::krylov;
namespace lo = pluss::subsp::math::linear_operator;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
Series ones(std::vector<std::int64_t> shape) { return Series::zeros(shape, f64) + 1.0; }
// the 1-D second-difference operator (2 on the diagonal, -1 beside it) applied to the last axis, without forming a matrix
Series shift_in(const Series& x, bool from_left) {
  const std::int64_t n = x.size(-1);
  const Series zero = pluss::series::zeros_like(x.slice(-1, 0, 1, 1));
  return from_left ? zero.unite({x.slice(-1, 0, n - 1, 1)}, -1) : x.slice(-1, 1, n, 1).unite({zero}, -1);
}
Series laplacian(const Series& x) { return x * 2.0 - shift_in(x, true) - shift_in(x, false); }
int main() {
  const std::int64_t n = 60;
  const Series scales = vec({1.0, 10.0, 100.0, 1000.0}).reshape({4, 1});
  const Series b = ones({4, n}) * scales;
  krylov::CgOptions o;
  o.max_iterations = 200;
  o.relative_tolerance = 1e-10;
  const auto r = krylov::cg(laplacian, b, {-1}, o);
  const auto mid = r.solution.select(-1, n / 2).host<double>();
  const auto conv = r.converged.host<bool>();
  const auto rel = r.relative_residual.host<double>();
  std::cout << "iterations: " << r.iterations << " (all lanes together)\n";
  std::cout << std::left << std::setw(8) << "lane" << std::setw(12) << "converged" << std::setw(20) << "relative residual" << "x at the midpoint\n";
  for (int i = 0; i < 4; ++i)
    std::cout << std::left << std::setw(8) << i << std::setw(12) << conv[i] << std::scientific << std::setprecision(1) << std::setw(20) << rel[i] << std::fixed << std::setprecision(2) << mid[i] << "\n";
}
