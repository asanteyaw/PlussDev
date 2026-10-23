// The smallest derivative-free solve. The objective is an ordinary function of a Series that returns a number; no
// gradient is written or recorded. `solve_dfo` takes the objective and a starting point, picks a method on its own
// (`Auto`), and reports which one it chose and why.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  // f(x) = (x0 - 1)^2 + 2 (x1 + 2)^2 + 3 (x2 - 0.5)^2, minimum 0 at (1, -2, 0.5).
  auto objective = [](const pluss::Series& x) {
    const auto c = vec({1.0, -2.0, 0.5});
    const auto w = vec({1.0, 2.0, 3.0});
    return ((x - c).square() * w).sum();
  };
  const auto r = dfo::solve_dfo(objective, vec({2.5, -1.5, 3.0}));

  std::cout << "selected solver : " << dfo::to_string(r.selected_solver) << "\n";
  std::cout << "reason          : " << r.selection_reason << "\n";
  std::cout << "status          : " << dfo::to_string(r.status) << "\n";
  std::cout << std::fixed << std::setprecision(4) << "minimizer       : " << r.minimizer.select(0, 0).item<double>() << ", "
            << r.minimizer.select(0, 1).item<double>() << ", " << r.minimizer.select(0, 2).item<double>() << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum         : " << r.minimum << "\n";
  std::cout << "evaluations     : " << r.diagnostics.evaluations << "\n";
  std::cout << "iterations      : " << r.diagnostics.iterations << "\n";
}
