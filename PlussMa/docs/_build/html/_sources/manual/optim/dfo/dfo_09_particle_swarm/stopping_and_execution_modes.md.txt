<!-- generated from stopping_and_execution_modes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Stopping and execution modes

[Back to The Particle Swarm Minimisers](../dfo_09_particle_swarm.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rastrigin(const pluss::Series& x) {
  const double pi = 3.14159265358979323846;
  return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();
}
const char* name(dfo::PopulationSolverStatus s) {
  return s == dfo::PopulationSolverStatus::Converged ? "Converged"
         : s == dfo::PopulationSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}

void show(const char* label, const dfo::PopulationSolverOutcome& o) {
  std::cout << std::left << std::setw(38) << label << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(11)
            << o.minimum << std::defaultfloat << std::setw(8) << o.diagnostics.evaluations << "generations " << o.diagnostics.iterations << "\n";
}

int main() {
  auto sphere = [](const pluss::Series& x) { return x.square().sum(); };
  dfo::ParticleSwarmConfig base;
  base.population_size = 40;
  base.max_generations = 400;
  base.lower_bounds = filled(2, -5.12);
  base.upper_bounds = filled(2, 5.12);
  show("swarm, check_every 0 (test off)", dfo::particle_swarm_minimizer(sphere, filled(2, 3.0), base));
  auto every = base; every.check_every = 10;
  show("swarm, check_every 10", dfo::particle_swarm_minimizer(sphere, filled(2, 3.0), every));
  auto gens = base; gens.max_generations = 15;
  show("swarm, max_generations 15", dfo::particle_swarm_minimizer(sphere, filled(2, 3.0), gens));
  auto evs = base; evs.max_evaluations = 500;
  show("swarm, max_evaluations 500", dfo::particle_swarm_minimizer(sphere, filled(2, 3.0), evs));
  auto seq = base; seq.execution_mode = dfo::PopulationSolverExecutionMode::OptimMasterCompat;
  show("swarm, sequential mode", dfo::particle_swarm_minimizer(sphere, filled(2, 3.0), seq));

  dfo::ParticleSwarmDvConfig dv;
  dv.population_size = 40;
  dv.max_generations = 400;
  dv.lower_bounds = filled(2, -5.12);
  dv.upper_bounds = filled(2, 5.12);
  show("swarm-DV, check_every 0 (test off)", dfo::particle_swarm_dv_minimizer(sphere, filled(2, 3.0), dv));
  auto dvevery = dv; dvevery.check_every = 10;
  show("swarm-DV, check_every 10", dfo::particle_swarm_dv_minimizer(sphere, filled(2, 3.0), dvevery));
}
```

Output

```text
swarm, check_every 0 (test off)       ReachedIterationBudget    0.00e+00   16440   generations 400
swarm, check_every 10                 Converged                 7.02e-29   2501    generations 60
swarm, max_generations 15             ReachedIterationBudget    2.00e-04   655     generations 15
swarm, max_evaluations 500            ReachedEvaluationBudget   2.59e-09   532     generations 12
swarm, sequential mode                ReachedIterationBudget    0.00e+00   16440   generations 400
swarm-DV, check_every 0 (test off)    ReachedEvaluationBudget   0.00e+00   16042   generations 399
swarm-DV, check_every 10              Converged                 6.41e-27   7642    generations 190
```
With the test off the run uses its whole generation budget. With `check_every` 10 it stops once the best value has stopped improving and the swarm has collapsed, here after 60 generations on a unimodal function, with the answer already below 1e-28. For rugged functions leave the test off and rely on a budget, or the swarm may collapse on a local minimum and stop there. The sequential mode updates particles one at a time and follows a different random path; both give valid results. A generation limit reports `ReachedIterationBudget`, an evaluation limit `ReachedEvaluationBudget`.
