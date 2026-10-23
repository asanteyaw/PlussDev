// Seven methods, four problems, one budget. Each problem is boxed and every method gets the same start, the same
// bounds and at most 30000 evaluations. The table shows how each run ended, the objective value it reached and the
// evaluations it spent. Nothing here is a ranking of methods in general; it shows how the problem's shape decides.
//   sphere      smooth bowl, one minimum
//   rosenbrock  smooth but a curved narrow valley
//   rastrigin   smooth with a grid of local minima (global minimum 0 at the origin)
//   sum |x - c| non-smooth: a kink along every coordinate of the minimiser c
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

struct Case {
  const char* name;
  std::function<Series(const Series&)> f;
  std::vector<double> start;
  double lo, hi;
};

int main() {
  const double pi = 3.14159265358979323846;
  const std::vector<Case> cases = {
      {"sphere 5-D", [](const Series& x) { return x.square().sum(); }, {2, -1.5, 3, -2.5, 1}, -5, 5},
      {"rosenbrock 2-D",
       [](const Series& x) {
         auto a = x.select(0, 0), b = x.select(0, 1);
         return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
       },
       {-1.2, 1.0}, -3, 3},
      {"rastrigin 4-D", [pi](const Series& x) { return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum(); },
       {2.5, -3.5, 1.5, 4.0}, -5.12, 5.12},
      {"sum |x-c| 4-D", [](const Series& x) { return (x - vec({1.0, -2.0, 0.5, 3.0})).abs().sum(); }, {0, 0, 0, 0}, -6, 6},
  };
  const dfo::DfoSolverKind kinds[] = {
      dfo::DfoSolverKind::NelderMead,           dfo::DfoSolverKind::PatternSearch,  dfo::DfoSolverKind::OrthoMads,
      dfo::DfoSolverKind::DifferentialEvolution, dfo::DfoSolverKind::DifferentialEvolutionPrmm,
      dfo::DfoSolverKind::ParticleSwarm,        dfo::DfoSolverKind::ParticleSwarmDv};

  for (const auto& c : cases) {
    std::cout << c.name << "\n";
    for (auto k : kinds) {
      dfo::DfoProblem p;
      p.initial = vec(c.start);
      p.objective = c.f;
      const std::size_t n = c.start.size();
      p.lower_bounds = vec(std::vector<double>(n, c.lo));
      p.upper_bounds = vec(std::vector<double>(n, c.hi));
      dfo::DfoOptions o;
      o.solver = k;
      o.max_evaluations = 30000;
      o.max_iterations = 5000;
      const auto r = dfo::DFO::solve(p, o);
      std::cout << "  " << std::left << std::setw(27) << dfo::to_string(k) << std::setw(25) << dfo::to_string(r.status)
                << std::scientific << std::setprecision(2) << std::setw(10) << r.minimum << std::defaultfloat << "evals "
                << r.diagnostics.evaluations << "\n";
    }
  }
}
