// Systems that are not symmetric. Conjugate gradients needs a symmetric positive definite A; for anything else use restarted GMRES:
// `krylov::gmres(apply, rhs, solve_dims, options)`. It builds an orthonormal basis of up to `restart` vectors, minimises the residual over
// it, and restarts with the improved guess until `max_iterations` or the tolerance. The result also reports `operator_evaluations` and
// `restarts`. A longer basis costs more memory and more work per iteration and, on some problems, converges in fewer operator
// applications; on others a short basis is cheaper, so the best restart length depends on the operator and is worth trying. Here: a convection-diffusion operator (second difference plus a first-difference term that makes
// it non-symmetric), n = 150, solved with restart lengths 5, 15, 40.
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
  const std::int64_t n = 150;
  auto convection = [&](const Series& x) {
    return laplacian(x) + (shift_in(x, false) - shift_in(x, true)) * 0.7;
  };
  const Series b = ones({n});
  std::cout << std::left << std::setw(10) << "restart" << std::setw(14) << "iterations" << std::setw(12) << "restarts" << std::setw(20) << "operator calls" << "relative residual\n";
  for (std::int64_t restart : {5, 15, 40}) {
    krylov::GmresOptions o;
    o.restart = restart;
    o.max_iterations = 5000;
    o.relative_tolerance = 1e-10;
    const auto r = krylov::gmres(convection, b, {-1}, o);
    std::cout << std::left << std::setw(10) << restart << std::setw(14) << r.iterations << std::setw(12) << r.restarts << std::setw(20) << r.operator_evaluations << std::scientific
              << std::setprecision(1) << r.relative_residual.item<double>() << "\n";
  }
}
