// The action of a matrix exponential. `krylov::exp_action(apply, v, solve_dims, time)` computes exp(time * A) v by an Arnoldi projection
// onto a small Krylov space; the matrix exponential itself, which would be dense, is never formed. This is the solution at time `time` of the
// linear system y' = A y started from v, and it is the tool for diffusion and Schrodinger-type evolution on large grids. The result has the
// `value`, an a-posteriori `residual_estimate` and `converged` flag, and the iteration (basis size) used. The size of the Krylov space
// (`max_iterations`) sets the accuracy. Here: heat flow, y' = -L y, with L the second-difference operator on n = 200 points, started from a unit
// spike in the middle and run to t = 20. The exact answer is the eigen-expansion sum_k exp(-t lambda_k) phi_k(j) phi_k(c) (2 / (n + 1)) with
// phi_k(j) = sin(pi k j / (n + 1)) and lambda_k = 2 - 2 cos(pi k / (n + 1)), summed here in double precision.
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
  const std::int64_t n = 200;
  const double pi = 3.14159265358979323846;
  const double t = 20.0;
  const std::int64_t center = 100;
  std::vector<double> spike(n, 0.0), exact(n, 0.0);
  spike[center - 1] = 1.0;
  for (std::int64_t j = 1; j <= n; ++j)
    for (std::int64_t k = 1; k <= n; ++k)
      exact[j - 1] += std::exp(-t * (2.0 - 2.0 * std::cos(pi * k / (n + 1)))) * std::sin(pi * k * j / (n + 1)) * std::sin(pi * k * center / (n + 1)) * 2.0 / (n + 1);
  std::cout << std::left << std::setw(12) << "basis size" << std::setw(12) << "converged" << std::setw(18) << "residual est." << "error vs exact\n";
  for (std::int64_t size : {4, 8, 16, 32, 64}) {
    krylov::ExpOptions o;
    o.max_iterations = size;
    const auto r = krylov::exp_action(laplacian, vec(spike), {-1}, -t, o);
    const auto y = r.value.to_vec<double>();
    double worst = 0.0;
    for (std::int64_t j = 0; j < n; ++j) worst = std::max(worst, std::abs(y[j] - exact[j]));
    std::cout << std::left << std::setw(12) << size << std::setw(12) << r.converged.item<bool>() << std::scientific << std::setprecision(1) << std::setw(18) << r.residual_estimate.item<double>() << worst << "\n";
  }
}
