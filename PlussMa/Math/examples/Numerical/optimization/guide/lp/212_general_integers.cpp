// Integer variables are not only 0 or 1. The first program maximizes 5 x + 4 y + 3 z subject to 2 x + 3 y + z <= 5,
// 4 x + y + 2 z <= 11 and 3 x + 4 y + 2 z <= 8, with x, y, z non-negative integers up to 10; its linear relaxation happens to be
// integral already, so the search finishes at the root. The second maximizes x + y subject to x + y <= 3.5 with both integer: the
// relaxation reaches 3.5, no integer point does, and the integer optimum is 3. The program prints, for each, the relaxation value,
// the integer optimum, the number of nodes the search needed and the solution.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace lp = pluss::subsp::math::linprog;
namespace milp = pluss::subsp::math::milp;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series mat(std::vector<double> v, std::int64_t rows, std::int64_t cols) { return pluss::make_series(v, {rows, cols}).to(f64); }

int main() {
  std::cout << std::fixed << std::setprecision(4);
  const auto run = [](const char* label, milp::Problem p) {
    const auto relaxed = lp::solve(p.lp);
    const auto r = milp::solve(p);
    const auto x = r.x.host<double>();
    std::cout << std::left << std::setw(32) << label << "relaxation " << std::setw(9) << -relaxed.objective.item<double>() << "integer optimum " << std::setw(9)
              << -r.objective.item<double>() << "nodes " << std::setw(4) << r.nodes << "x =";
    for (const double v : x) std::cout << " " << v;
    std::cout << "\n";
  };

  {
    milp::Problem p;
    p.lp.c = vec({-5, -4, -3});
    p.lp.a_ub = mat({2, 3, 1, 4, 1, 2, 3, 4, 2}, 3, 3);
    p.lp.b_ub = vec({5, 11, 8});
    p.lp.lower = vec({0, 0, 0});
    p.lp.upper = vec({10, 10, 10});
    p.integer = vec({1, 1, 1});
    run("a small production plan", p);
  }
  {
    milp::Problem p;
    p.lp.c = vec({-1, -1});
    p.lp.a_ub = mat({1, 1}, 1, 2);
    p.lp.b_ub = vec({3.5});
    p.lp.lower = vec({0, 0});
    p.lp.upper = vec({10, 10});
    p.integer = vec({1, 1});
    run("x + y <= 3.5, both integer", p);
  }
  return 0;
}
