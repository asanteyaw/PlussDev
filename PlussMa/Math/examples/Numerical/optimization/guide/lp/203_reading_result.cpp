// Everything a solve returns, for a small program with an equality row, an inequality row and a bound: minimize
// 2 x0 + 3 x1 + x2 subject to x0 + x1 + x2 = 10, x0 - x1 <= 2 and 0 <= x <= (6, 6, 6). Each field is printed with a one-line
// reading. Multipliers (the dual values) are the prices of the rows: how much the optimal objective changes per unit of the
// right-hand side. The status is stored as a number so that a whole batch of solves can return one status per lane.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  lp::Problem p;
  p.c = vec({2, 3, 1});
  p.a_eq = mat({1, 1, 1}, 1, 3);
  p.b_eq = vec({10});
  p.a_ub = mat({1, -1, 0}, 1, 3);
  p.b_ub = vec({2});
  p.lower = vec({0, 0, 0});
  p.upper = vec({6, 6, 6});
  const auto r = lp::solve(p);

  std::cout << std::fixed << std::setprecision(4);
  const auto x = r.x.to_vec<double>();
  std::cout << "status          " << static_cast<int>(r.status.item<double>()) << "   (0 = Optimal)\n";
  std::cout << "x               " << x[0] << ", " << x[1] << ", " << x[2] << "\n";
  std::cout << "objective       " << r.objective.item<double>() << "   (the optimum of c . x)\n";
  std::cout << "dual_objective  " << r.dual_objective.item<double>() << "   (equals the objective at an optimum: the certificate)\n";
  std::cout << "dual_eq         " << r.dual_eq.to_vec<double>()[0] << "   (change of the optimum per unit of the equality's right-hand side)\n";
  std::cout << "dual_ub         " << r.dual_ub.to_vec<double>()[0] << "   (the same for the inequality; zero or negative for a minimization)\n";
  std::cout << "iterations      " << r.iterations << "\n";
  std::cout << "the ray and the Farkas multipliers are meaningful only for an unbounded or an infeasible program (next program)\n";
  return 0;
}
