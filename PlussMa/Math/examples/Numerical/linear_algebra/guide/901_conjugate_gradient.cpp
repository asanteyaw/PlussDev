// Solving A x = b without forming A. `krylov::cg(apply, rhs, solve_dims)` solves a symmetric positive definite system by conjugate gradients.
// `apply` is a callable that returns A times a vector (here the 1-D second-difference operator, applied to shifted copies of x), `rhs` is b,
// and `solve_dims` names the axes that hold the unknowns (the last axis here). The operator is never stored, which is what makes the method
// usable on large problems. The result has the solution, the final residual norm, the relative residual, per-lane `converged` and `breakdown`
// flags, and the iteration count. Here: n = 100 unknowns with b = (1, ..., 1); the exact solution is x_i = i (n + 1 - i) / 2. In exact arithmetic CG finishes in as many steps as the right-hand side has
// distinct eigencomponents, here about 50 because b only excites the odd eigenvectors, so 56 iterations for 100 unknowns is not an accident.
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
  const std::int64_t n = 100;
  const Series b = ones({n});
  krylov::CgOptions o;
  o.max_iterations = 400;
  o.relative_tolerance = 1e-10;
  const auto r = krylov::cg(laplacian, b, {-1}, o);
  const auto x = r.solution.host<double>();
  double worst = 0.0;
  for (std::int64_t i = 1; i <= n; ++i) worst = std::max(worst, std::abs(x[i - 1] - 0.5 * i * (n + 1 - i)));
  std::cout << "iterations        : " << r.iterations << " (the system has " << n << " unknowns)\n";
  std::cout << std::scientific << std::setprecision(2) << "relative residual : " << r.relative_residual.item<double>() << "\n";
  std::cout << "converged         : " << r.converged.item<bool>() << ", breakdown " << r.breakdown.item<bool>() << "\n";
  std::cout << "largest error     : " << worst << "\n";
}
