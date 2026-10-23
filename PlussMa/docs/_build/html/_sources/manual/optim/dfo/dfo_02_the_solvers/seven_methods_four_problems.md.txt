<!-- generated from seven_methods_four_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Seven methods, four problems

[Back to Choosing a Solver](../dfo_02_the_solvers.md)


The same start, bounds and budget (30000 evaluations) for every method. `sum |x-c|` is non-smooth: it has a kink at its minimum.

```cpp
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
```

Output

```text
sphere 5-D
  NelderMead                 Converged                2.62e-09  evals 377
  PatternSearch              Converged                2.36e-10  evals 804
  OrthoMads                  Converged                9.51e-26  evals 2224
  DifferentialEvolution      Converged                4.63e-06  evals 28200
  DifferentialEvolutionPrmm  ReachedEvaluationBudget  7.38e-13  evals 30000
  ParticleSwarm              Converged                1.58e-30  evals 14271
  ParticleSwarmDv            ReachedEvaluationBudget  6.45e-07  evals 30111
rosenbrock 2-D
  NelderMead                 Converged                8.18e-10  evals 159
  PatternSearch              Converged                1.14e-08  evals 935
  OrthoMads                  ReachedIterationBudget   1.09e-19  evals 15940
  DifferentialEvolution      ReachedEvaluationBudget  2.61e-15  evals 30000
  DifferentialEvolutionPrmm  ReachedEvaluationBudget  3.98e-15  evals 30000
  ParticleSwarm              ReachedEvaluationBudget  2.09e-09  evals 30149
  ParticleSwarmDv            ReachedEvaluationBudget  1.69e-03  evals 30040
rastrigin 4-D
  NelderMead                 Converged                4.18e+01  evals 180
  PatternSearch              Converged                1.39e+01  evals 688
  OrthoMads                  Converged                5.97e+00  evals 772
  DifferentialEvolution      ReachedEvaluationBudget  3.18e+00  evals 30000
  DifferentialEvolutionPrmm  ReachedEvaluationBudget  4.61e-06  evals 30000
  ParticleSwarm              ReachedEvaluationBudget  0.00e+00  evals 30149
  ParticleSwarmDv            ReachedEvaluationBudget  5.92e+00  evals 30155
sum |x-c| 4-D
  NelderMead                 Converged                2.38e+00  evals 378
  PatternSearch              Converged                2.09e-05  evals 663
  OrthoMads                  Converged                4.63e-13  evals 1423
  DifferentialEvolution      ReachedEvaluationBudget  5.32e-05  evals 30000
  DifferentialEvolutionPrmm  ReachedEvaluationBudget  2.40e-08  evals 30000
  ParticleSwarm              Converged                0.00e+00  evals 16281
  ParticleSwarmDv            ReachedEvaluationBudget  1.29e-03  evals 30142
```
What to take from the table, and what not to:

- **Smooth, one minimum (sphere, Rosenbrock).** Nelder–Mead and pattern search converge with the fewest evaluations (hundreds). The population methods reach similar accuracy at ten to a hundred times the evaluations, or stop on the budget instead of converging.
- **Many local minima (Rastrigin).** The local methods (Nelder–Mead, pattern search, OrthoMADS) converge, to a local minimum, and say `Converged`: the status describes the method's stopping test, not the global quality. The population methods keep searching; DE‑PRMM and particle swarm reach the global minimum (to about 5e‑6 and 0 respectively) at the cost of the whole budget.
- **Non-smooth (`sum |x-c|`).** Nelder–Mead stalls at a value of about 2.4 although it reports `Converged`: its simplex collapses against the kink. The direct-search methods (pattern search, OrthoMADS) get through the kink, because they only compare values.
- **Budget statuses are not failures.** A population method that ends on `ReachedEvaluationBudget` with a value of 1e‑12 has done well; it simply has no cheap stopping test.

One table on four problems is evidence, not a ranking. Different starts, dimensions and budgets change the numbers; the shapes of the results (local methods cheap and local, population methods expensive and global) are what carry over.
