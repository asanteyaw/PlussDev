// A sparse matrix as an operator. `linear_operator::coo(rows, columns, row_index, column_index, values)` builds a matrix from its nonzeros
// (index vectors are Int64, `values` is a Series); `apply(x)` multiplies it by a vector or a batch [..., columns], `adjoint()` gives the
// conjugate transpose, and the values stay a Series, so the products are differentiable with respect to them. The map is a ready operator
// for the Krylov solvers. Here: the tridiagonal Laplacian as 3n - 2 nonzeros, applied to a vector, solved with cg, and its adjoint formed.
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
  const std::int64_t n = 30;
  std::vector<std::int64_t> rows, cols;
  std::vector<double> vals;
  for (std::int64_t i = 0; i < n; ++i) {
    rows.push_back(i); cols.push_back(i); vals.push_back(2.0);
    if (i > 0) { rows.push_back(i); cols.push_back(i - 1); vals.push_back(-1.0); }
    if (i + 1 < n) { rows.push_back(i); cols.push_back(i + 1); vals.push_back(-1.0); }
  }
  const auto idx = [](const std::vector<std::int64_t>& v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}); };
  Series values = vec(vals);
  const auto A = lo::coo(n, n, idx(rows), idx(cols), values);
  std::cout << A.rows() << " x " << A.columns() << " map with " << A.nnz() << " nonzeros\n";

  const Series unit = ones({n});
  const auto y = A.apply(unit).host<double>();
  std::cout << "A * ones: first " << y[0] << ", middle " << y[n / 2] << ", last " << y[n - 1] << "   (1, 0, 1 for the Laplacian)\n";

  krylov::CgOptions o;
  o.max_iterations = 200;
  o.relative_tolerance = 1e-12;
  const auto r = krylov::cg([&](const Series& x) { return A.apply(x); }, unit, {-1}, o);
  const double mid_exact = 0.5 * (n / 2 + 1) * (n + 1 - (n / 2 + 1));
  std::cout << std::fixed << std::setprecision(6) << "solution at the middle (i = " << n / 2 + 1 << "): " << r.solution.select(0, n / 2).item<double>() << "  (exact " << mid_exact << ")\n";
  const auto B = A.adjoint();
  std::cout << "adjoint is " << B.rows() << " x " << B.columns() << "\n";
}
