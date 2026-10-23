// Solving many problems at once and reusing the setup. Leading dimensions are batch dimensions: a q of shape [B, n] solves B problems
// that share P, A and the bounds in one call. When only q (or the bounds) changes between solves, `qp::prepare(P, A)` factorises the
// invariant part once and returns a `Solver`; `solver.solve(q, lower, upper)` reuses it. Both give the same answers. Here: the
// 2-variable problem  minimise 0.5|x|^2 + q'x  with  -1 <= x <= 1  for four different q; the answer is x = clamp(-q, -1, 1).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
const double inf = std::numeric_limits<double>::infinity();
Series mat(std::vector<std::vector<double>> rows) {
  std::vector<double> flat;
  for (const auto& r : rows) flat.insert(flat.end(), r.begin(), r.end());
  return pluss::make_series(flat, {static_cast<std::int64_t>(rows.size()), static_cast<std::int64_t>(rows[0].size())}).to(f64);
}
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
int main() {
  const auto p = mat({{1, 0}, {0, 1}});
  const auto a = mat({{1, 0}, {0, 1}});
  const auto lower = vec({-1.0, -1.0}), upper = vec({1.0, 1.0});
  const auto q = mat({{0.2, -0.5}, {2.0, 0.0}, {-3.0, 3.0}, {-0.9, 0.9}});

  const auto batched = qp::solve(p, q, a, lower, upper);
  std::cout << "batched solve: shape [" << batched.x.size(0) << ", " << batched.x.size(1) << "], converged " << batched.converged << "\n";

  const auto solver = qp::prepare(p, a);
  std::cout << "prepared solver: " << solver.variables() << " variables, " << solver.constraints() << " constraints\n";
  const auto reused = solver.solve(q, lower, upper);

  const auto xb = batched.x.to_vec<double>(), xr = reused.x.to_vec<double>(), qv = q.to_vec<double>();
  std::cout << std::left << std::setw(16) << "q" << std::setw(24) << "x (batched)" << std::setw(24) << "x (prepared)" << "exact\n";
  std::cout << std::fixed << std::setprecision(4);
  for (int i = 0; i < 4; ++i) {
    auto c = [](double v) { return std::max(-1.0, std::min(1.0, v)); };
    std::ostringstream a1, a2, a3, a4;
    a1 << std::fixed << std::setprecision(1) << qv[2 * i] << ", " << qv[2 * i + 1];
    a2 << std::fixed << std::setprecision(4) << xb[2 * i] << ", " << xb[2 * i + 1];
    a3 << std::fixed << std::setprecision(4) << xr[2 * i] << ", " << xr[2 * i + 1];
    a4 << std::fixed << std::setprecision(4) << c(-qv[2 * i]) + 0.0 << ", " << c(-qv[2 * i + 1]) + 0.0;
    std::cout << std::left << std::setw(16) << a1.str() << std::setw(24) << a2.str() << std::setw(24) << a3.str() << a4.str() << "\n";
  }
}
