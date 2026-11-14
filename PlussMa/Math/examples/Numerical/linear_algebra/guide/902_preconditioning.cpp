// Preconditioning. Conjugate gradients converges in a number of iterations that grows with how spread out the eigenvalues of A are. A
// preconditioner M approximately inverts A so that M A has clustered eigenvalues: `krylov::pcg(apply, precondition, rhs, solve_dims)` takes a
// second callable `precondition(r)` that returns M r. Here: a diagonal matrix with entries from 1 to 10^4 plus a weak coupling to the neighbours.
// The cheapest useful preconditioner divides by the diagonal (Jacobi). Compare the iterations.
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
  std::vector<double> d(n);
  for (std::int64_t i = 0; i < n; ++i) d[i] = std::pow(10.0, 4.0 * static_cast<double>(i) / static_cast<double>(n - 1));
  const Series diag = vec(d);
  auto apply = [&](const Series& x) { return diag * x + (laplacian(x)) * 0.1; };
  // the diagonal of A: diag + 0.2
  const Series inverse_diagonal = (diag + 0.2).reciprocal();
  const Series b = ones({n});
  krylov::CgOptions o;
  o.max_iterations = 2000;
  o.relative_tolerance = 1e-10;
  const auto plain = krylov::cg(apply, b, {-1}, o);
  const auto jacobi = krylov::pcg(apply, [&](const Series& r) { return r * inverse_diagonal; }, b, {-1}, o);
  std::cout << std::left << std::setw(26) << "method" << std::setw(12) << "iterations" << std::setw(12) << "converged" << "relative residual\n";
  auto row = [&](const char* label, const krylov::CgResult& r) {
    std::cout << std::left << std::setw(26) << label << std::setw(12) << r.iterations << std::setw(12) << r.converged.squeeze().item<bool>() << std::scientific << std::setprecision(1)
              << r.relative_residual.squeeze().item<double>() << "\n";
  };
  row("cg", plain);
  row("pcg, Jacobi", jacobi);
}
