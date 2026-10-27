<!-- generated from comparing_the_methods.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Comparing the methods

[Back to Derivative-Free Optimization 2: Choosing a Solver](../dfo_02_the_solvers.md)

There are seven derivative-free methods in the family: Nelder–Mead, pattern search, differential evolution (in two variants), particle swarm, and the mesh adaptive direct search methods. They differ in what they assume and what they cost, and no ranking of them is meaningful without a problem. The most useful single exercise is to run all of them on the same set of problems with the same start, bounds and evaluation budget, and read the shapes of the results.

The first program does exactly that, on four problems chosen to stress different things: a smooth sphere, the curved valley of Rosenbrock, the many-minimum Rastrigin function, and a non-smooth function with a kink at its minimum. The second shows the standard remedy for many local minima when the budget allows: restart a cheap local method from several starts and keep the best.

## Program 1: Seven methods, four problems

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

## Program 2: Restarting a local method

When the problem has many local minima and the evaluation budget allows, restart a cheap local method from several starts and keep the best.

```cpp
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
```

Output

```text
start           minimum       evals     best so far   total evals
(3.2  ,2.7  ) 1.79e+01      60        1.79e+01      60
(-2.1 ,3.4  ) 1.99e+01      105       1.79e+01      165
(1.4  ,-1.6 ) 4.97e+00      56        4.97e+00      221
(-3.6 ,-2.4 ) 1.99e+01      61        4.97e+00      282
(0.6  ,0.7  ) 1.99e+00      67        1.99e+00      349
(2.2  ,-2.9 ) 1.29e+01      56        1.99e+00      405
```
Six restarts cost 405 evaluations in total and improved the best value from 17.9 to 1.99. None reached the global minimum 0, because none started in its basin. That is the honest limit of restarts: they help in proportion to how well the starts cover the region. A population method covers the region by construction, which is why it found the global minimum above, at far more evaluations. Pick the strategy by the evaluations you can afford.

## How reliable is each method on a rugged function?

One run of a stochastic method is one draw from a distribution of outcomes, and a table of single runs can mislead. The more honest comparison repeats each method over many seeds and reports how *often* it finds the global minimum and what it costs. The program runs five methods on the three-variable Rastrigin function in the box [−5.12, 5.12] from a start in a local-minimum basin, 30 seeds each with the same budget, and counts the runs that end below 10⁻³ (the global minimum is 0; the nearest local minima have value 1).

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }

int main() {
  const double pi = 3.14159265358979323846;
  auto rastrigin = [pi](const pluss::Series& x) { return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum(); };
  struct Entry { const char* name; dfo::DfoSolverKind kind; };
  const Entry methods[] = {{"NelderMead", dfo::DfoSolverKind::NelderMead},
                           {"PatternSearch", dfo::DfoSolverKind::PatternSearch},
                           {"OrthoMads", dfo::DfoSolverKind::OrthoMads},
                           {"DifferentialEvolution", dfo::DfoSolverKind::DifferentialEvolution},
                           {"ParticleSwarm", dfo::DfoSolverKind::ParticleSwarm}};
  const int seeds = 30;
  std::cout << std::left << std::setw(24) << "method" << std::right << std::setw(14) << "global found" << std::setw(20) << "mean evaluations" << std::setw(18) << "mean minimum" << "\n";
  for (const auto& m : methods) {
    int found = 0;
    double evals = 0.0, mean_min = 0.0;
    for (int s = 1; s <= seeds; ++s) {
      dfo::DfoProblem p;
      p.initial = filled(3, 3.0);
      p.lower_bounds = filled(3, -5.12);
      p.upper_bounds = filled(3, 5.12);
      p.objective = rastrigin;
      dfo::DfoOptions o;
      o.solver = m.kind;
      o.max_evaluations = 6000;
      o.rng_seed = static_cast<std::uint64_t>(s);
      const auto r = dfo::DFO::solve(p, o);
      found += r.minimum < 1e-3;
      evals += static_cast<double>(r.diagnostics.evaluations);
      mean_min += r.minimum;
    }
    std::cout << std::left << std::setw(24) << m.name << std::right << std::setw(10) << found << " / " << seeds << std::fixed << std::setprecision(0) << std::setw(14) << evals / seeds
              << std::setprecision(3) << std::setw(18) << mean_min / seeds << "\n";
  }
}
```

Output

```text
method                    global found    mean evaluations      mean minimum
NelderMead                       0 / 30            84            26.864
PatternSearch                    1 / 30           334            18.838
OrthoMads                        0 / 30           519             5.970
DifferentialEvolution            0 / 30          6000             2.841
ParticleSwarm                   20 / 30          6029             0.200
```

Read the second column as a probability, and the surprise in it. Particle swarm finds the global minimum on 20 of the 30 seeds (mean final value 0.2). The three local methods almost never do (Nelder–Mead 0, OrthoMADS 0, pattern search 1 of 30): started in a local basin, they converge to a local minimum and stop, in a few hundred evaluations, which is exactly what they are built for. The differential-evolution method, which the earlier table suggests is a global method, found it on **none** of the 30 seeds within this budget, ending at a mean value of 2.8 after spending all 6000 evaluations. With its default population size it spends the budget on a few generations of a large population, and has not yet collapsed onto a basin; the parameters page of the differential-evolution chapter shows that a smaller population is the better use of a tight budget. So a method's global reputation depends on the budget as well as on the method: compare them at the budget you have, and over several seeds, before choosing. The evaluations column is the price: the population methods spend the whole budget where the local methods stop early.

## Decisions and pitfalls

- **Local methods are cheap and local; population methods are expensive and global.** On smooth, single-minimum problems Nelder–Mead and pattern search need hundreds of evaluations; the population methods need ten to a hundred times as many.
- **`Converged` describes the stopping test, not the global quality.** A local method on Rastrigin reports `Converged` at a local minimum.
- **Nelder–Mead stalls on a kink.** Its simplex collapses against a non-smooth minimum while reporting `Converged`; the direct-search methods, which only compare values, get through.
- **One table is evidence, not a ranking.** Different starts, dimensions and budgets change the numbers; the shapes of the results are what carry over.
- **Restarts help in proportion to how well the starts cover the region.** Six restarts improved the best value from 17.9 to 1.99 but none started in the global minimum's basin; a population method covers the region by construction.

Where next: [Chapter 3](../dfo_03_bounds_and_constraints.md) adds bounds and constraints.
