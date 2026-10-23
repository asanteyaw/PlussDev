// A local method on a multimodal function. Nelder-Mead from one start falls into whichever local minimum is nearest.
// Restarting it from several different starts and keeping the best is the simplest global strategy, and its cost is
// easy to count: the evaluations add up. The 2-D Rastrigin function has its global minimum 0 at the origin and a grid
// of local minima around it.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const double pi = 3.14159265358979323846;
  auto rastrigin = [pi](const pluss::Series& x) { return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum(); };
  const std::vector<std::vector<double>> starts = {{3.2, 2.7}, {-2.1, 3.4}, {1.4, -1.6}, {-3.6, -2.4}, {0.6, 0.7}, {2.2, -2.9}};

  double best = 1e300;
  std::int64_t total = 0;
  std::cout << std::left << std::setw(16) << "start" << std::setw(14) << "minimum" << std::setw(10) << "evals"
            << std::setw(14) << "best so far" << "total evals\n";
  for (const auto& s : starts) {
    dfo::DfoProblem p;
    p.initial = vec(s);
    p.objective = rastrigin;
    dfo::DfoOptions o;
    o.solver = dfo::DfoSolverKind::NelderMead;
    const auto r = dfo::DFO::solve(p, o);
    total += r.diagnostics.evaluations;
    best = std::min(best, r.minimum);
    std::cout << std::left << "(" << std::fixed << std::setprecision(1) << std::setw(5) << s[0] << "," << std::setw(5) << s[1] << ") "
              << std::scientific << std::setprecision(2) << std::setw(14) << r.minimum << std::defaultfloat << std::setw(10)
              << r.diagnostics.evaluations << std::scientific << std::setw(14) << best << std::defaultfloat << total << "\n";
  }
}
