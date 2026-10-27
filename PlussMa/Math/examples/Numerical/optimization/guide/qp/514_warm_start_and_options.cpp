// Warm starts and options. The solver is iterative (ADMM), so a good starting point saves iterations: `result.warm()` returns the
// {x, z, y} state, and passing it back as the last argument starts the next solve from there. This is what to do when a problem
// is re-solved with a slightly changed q or bounds (model predictive control, a sweep). `Options` sets the accuracy: the run stops
// when both residuals fall below abs_tol + rel_tol * scale, checked every `check_every` iterations (so the count is a multiple of
// it); `max_iters` caps the work; `rho` is the starting penalty parameter; by default it is adapted during the run (`adaptive_rho`), so the starting value matters little. `sigma` and `alpha` are the regularisation and over-relaxation.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

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
  const auto p = mat({{4.0, 1.0}, {1.0, 2.0}});
  const auto a = mat({{1.0, 1.0}, {1.0, 0.0}, {0.0, 1.0}});
  const auto lower = vec({1.0, 0.0, 0.0}), upper = vec({1.0, 0.7, 0.7});

  const auto first = qp::solve(p, vec({1.0, 1.0}), a, lower, upper);
  std::cout << "first solve: " << first.iterations << " iterations\n";
  const auto q2 = vec({1.05, 0.95});  // a slightly different problem
  const auto cold = qp::solve(p, q2, a, lower, upper);
  const auto warm = qp::solve(p, q2, a, lower, upper, qp::Options{}, first.warm());
  std::cout << "changed q, cold start: " << cold.iterations << " iterations\n";
  std::cout << "changed q, warm start: " << warm.iterations << " iterations\n\n";

  std::cout << std::left << std::setw(30) << "options" << std::setw(12) << "iterations" << std::setw(12) << "converged" << "x0 error\n";
  auto run = [&](const char* label, qp::Options o) {
    const auto r = qp::solve(p, vec({1.0, 1.0}), a, lower, upper, o);
    std::cout << std::left << std::setw(30) << label << std::setw(12) << r.iterations << std::setw(12) << (r.converged ? "yes" : "no")
              << std::scientific << std::setprecision(1) << std::abs(r.x.host<double>()[0] - 0.3) << "\n";
  };
  run("defaults (1e-5)", qp::Options{});
  qp::Options tight; tight.abs_tol = 1e-9; tight.rel_tol = 1e-9;
  run("tolerances 1e-9", tight);
  qp::Options loose; loose.abs_tol = 1e-3; loose.rel_tol = 1e-3;
  run("tolerances 1e-3", loose);
  qp::Options capped; capped.max_iters = 20;
  run("max_iters 20", capped);
  qp::Options bigrho; bigrho.rho = 5.0;
  run("rho 5", bigrho);
  qp::Options smallrho; smallrho.rho = 0.005;
  run("rho 0.005", smallrho);
  qp::Options fixedrho; fixedrho.adaptive_rho = false;
  run("adaptive_rho off", fixedrho);
  qp::Options fixedsmall; fixedsmall.adaptive_rho = false; fixedsmall.rho = 0.005;
  run("adaptive_rho off, rho 0.005", fixedsmall);
}
