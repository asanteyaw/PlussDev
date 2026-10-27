<!-- generated from particle_swarm_in_practice.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Particle swarm in practice

[Back to Derivative-Free Optimization 9: The Particle Swarm Minimisers](../dfo_09_particle_swarm.md)

Particle swarm imitates a flock: many particles move through the search region, each pulled toward the best place it has seen itself and the best place any particle has seen. It needs only function values, scales better with the number of variables than a simplex, and is the usual choice for rugged functions in a box. Two standalone minimisers exist: `particle_swarm_minimizer`, the classic scheme, and `particle_swarm_dv_minimizer`, a variant that mixes in differential evolution.

The programs show a direct call on the many-minimum Rastrigin function, a table of the swarm's parameters (each cell one seed, which makes the point that the defaults are forgiving), the differential-evolution variant compared with the classic swarm at three and six variables, and stopping, where the early-stop test ends the run once the swarm has collapsed.

## Program 1: A direct call

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

int main() {
  dfo::ParticleSwarmConfig c;
  c.lower_bounds = filled(3, -5.12);
  c.upper_bounds = filled(3, 5.12);
  c.max_evaluations = 20000;
  const auto o = dfo::particle_swarm_minimizer(rastrigin, filled(3, 3.0), c);
  const auto x = o.minimizer.host<double>();
  std::cout << std::fixed << std::setprecision(5) << "minimizer   : " << x[0] << ", " << x[1] << ", " << x[2] << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum     : " << o.minimum << "\n";
  std::cout << "status      : " << dfo::to_string(o.status) << "\n";
  std::cout << "generations : " << o.diagnostics.iterations << "\nevaluations : " << o.diagnostics.evaluations << "\nlast step   : " << o.diagnostics.last_step << "\n";
}
```

Output

```text
minimizer   : -0.00000, 0.00000, 0.00000
minimum     : 0.00e+00
status      : ReachedEvaluationBudget
generations : 99
evaluations : 20099
last step   : generation
```
The swarm covers the whole box, finds the global minimum of this Rastrigin function from a start in a distant local-minimum basin, and ends on the evaluation budget: with the early-stop test off, only budgets end a run (Program 4).

## Program 2: The parameters

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <cmath>
#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rastrigin(const pluss::Series& x) {
  const double pi = 3.14159265358979323846;
  return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();
}

int main() {
  std::vector<std::pair<const char*, dfo::ParticleSwarmConfig>> cases;
  dfo::ParticleSwarmConfig base;
  cases.emplace_back("defaults", base);
  auto small = base; small.population_size = 30;
  cases.emplace_back("population_size 30", small);
  auto cog = base; cog.cognitive = 3.0; cog.social = 1.0;
  cases.emplace_back("cognitive 3, social 1", cog);
  auto soc = base; soc.cognitive = 1.0; soc.social = 3.0;
  cases.emplace_back("cognitive 1, social 3", soc);
  auto constant = base; constant.linear_inertia_schedule = false; constant.inertia = 0.7;
  cases.emplace_back("constant inertia 0.7", constant);
  auto clamp = base; clamp.velocity_clamp_ratio = 0.1;
  cases.emplace_back("velocity_clamp_ratio 0.1", clamp);
  auto tv = base; tv.time_varying_acceleration = true;
  cases.emplace_back("time_varying_acceleration", tv);
  auto nocenter = base; nocenter.center_particle = false;
  cases.emplace_back("center_particle off", nocenter);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    c.lower_bounds = filled(3, -5.12);
    c.upper_bounds = filled(3, 5.12);
    c.max_evaluations = 20000;
    std::vector<double> minima;  // one per seed
    for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
      c.rng_seed = seed;
      minima.push_back(dfo::particle_swarm_minimizer(rastrigin, filled(3, 3.0), c).minimum);
    }
    out.writeln(label, minima);
  }
}
```

Output

```text
defaults [0.00e+00, 0.00e+00, 0.00e+00]
population_size 30 [0.00e+00, 9.95e-01, 0.00e+00]
cognitive 3, social 1 [0.00e+00, 0.00e+00, 0.00e+00]
cognitive 1, social 3 [0.00e+00, 0.00e+00, 0.00e+00]
constant inertia 0.7 [2.31e-14, 0.00e+00, 1.60e-14]
velocity_clamp_ratio 0.1 [0.00e+00, 0.00e+00, 0.00e+00]
time_varying_acceleration [0.00e+00, 0.00e+00, 0.00e+00]
center_particle off [0.00e+00, 0.00e+00, 0.00e+00]
```
Each cell is one seed. On this problem nearly every setting finds the global minimum with every seed, so the table mainly shows that the defaults are forgiving. Where there is a difference:

- A **small swarm** (30 particles) fails on one seed in three, with value 0.995: too few particles to cover a rugged box. Use at least 5 to 10 particles per variable, and more for rugged functions.
- **Constant inertia** 0.7 settles to about 1e-14 instead of exactly 0: without the shrinking schedule the particles never quite stop moving. Keep the schedule on when you need full precision.
- The pull weights, the speed clamp, the time-varying pulls and the centre particle made no difference here. They matter on harder functions; change one at a time, over several seeds, if you tune.

## Program 3: The differential-evolution variant

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

int main() {
  std::cout << std::left << std::setw(6) << "dim" << std::setw(14) << "method" << "minimum for seeds 1, 2, 3\n";
  for (std::int64_t n : {3, 6}) {
    for (int which : {0, 1}) {
      std::cout << std::left << std::setw(6) << n << std::setw(14) << (which == 0 ? "swarm" : "swarm-DV");
      for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
        double minimum = 0.0;
        if (which == 0) {
          dfo::ParticleSwarmConfig c;
          c.lower_bounds = filled(n, -5.12); c.upper_bounds = filled(n, 5.12); c.max_evaluations = 40000; c.rng_seed = seed;
          minimum = dfo::particle_swarm_minimizer(rastrigin, filled(n, 3.0), c).minimum;
        } else {
          dfo::ParticleSwarmDvConfig c;
          c.lower_bounds = filled(n, -5.12); c.upper_bounds = filled(n, 5.12); c.max_evaluations = 40000; c.rng_seed = seed;
          minimum = dfo::particle_swarm_dv_minimizer(rastrigin, filled(n, 3.0), c).minimum;
        }
        std::cout << std::scientific << std::setprecision(2) << minimum << "   ";
      }
      std::cout << "\n";
    }
  }
}
```

Output

```text
dim   method        minimum for seeds 1, 2, 3
3     swarm         0.00e+00   0.00e+00   0.00e+00   
3     swarm-DV      3.59e-01   3.69e-01   7.13e-02   
6     swarm         9.95e-01   0.00e+00   1.28e-09   
6     swarm-DV      1.39e+01   1.80e+01   1.25e+01   
```
At 3 variables the classic swarm finds the global minimum exactly on every seed and the variant ends within 0.4 of it. At 6 variables, with the same 40000 evaluations, the classic swarm reaches essentially zero on two seeds and about 1 on the third, while the variant stops between 12 and 18. The variant moves each particle only when the move improves it and has no memory of a particle's own best position, so it behaves like differential evolution (Chapter 8, where plain DE also stops near 15 on this problem) rather than like a swarm. The classic swarm is the better first choice on rugged functions; try the variant as a second opinion.

## Program 4: Stopping and execution modes

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <cmath>
#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rastrigin(const pluss::Series& x) {
  const double pi = 3.14159265358979323846;
  return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();
}
int main() {
  auto sphere = [](const pluss::Series& x) { return x.square().sum(); };
  dfo::ParticleSwarmConfig base;
  base.population_size = 40;
  base.max_generations = 400;
  base.lower_bounds = filled(2, -5.12);
  base.upper_bounds = filled(2, 5.12);
  std::vector<std::pair<const char*, dfo::PopulationSolverOutcome>> runs;
  runs.emplace_back("swarm, check_every 0 (test off)", dfo::particle_swarm_minimizer(sphere, filled(2, 3.0), base));
  auto every = base; every.check_every = 10;
  runs.emplace_back("swarm, check_every 10", dfo::particle_swarm_minimizer(sphere, filled(2, 3.0), every));
  auto gens = base; gens.max_generations = 15;
  runs.emplace_back("swarm, max_generations 15", dfo::particle_swarm_minimizer(sphere, filled(2, 3.0), gens));
  auto evs = base; evs.max_evaluations = 500;
  runs.emplace_back("swarm, max_evaluations 500", dfo::particle_swarm_minimizer(sphere, filled(2, 3.0), evs));
  auto seq = base; seq.execution_mode = dfo::PopulationSolverExecutionMode::OptimMasterCompat;
  runs.emplace_back("swarm, sequential mode", dfo::particle_swarm_minimizer(sphere, filled(2, 3.0), seq));

  dfo::ParticleSwarmDvConfig dv;
  dv.population_size = 40;
  dv.max_generations = 400;
  dv.lower_bounds = filled(2, -5.12);
  dv.upper_bounds = filled(2, 5.12);
  runs.emplace_back("swarm-DV, check_every 0 (test off)", dfo::particle_swarm_dv_minimizer(sphere, filled(2, 3.0), dv));
  auto dvevery = dv; dvevery.check_every = 10;
  runs.emplace_back("swarm-DV, check_every 10", dfo::particle_swarm_dv_minimizer(sphere, filled(2, 3.0), dvevery));

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (const auto& [label, o] : runs)
    out.writeln(label, dfo::to_string(o.status), "minimum", o.minimum, "evaluations", o.diagnostics.evaluations, "generations", o.diagnostics.iterations);
}
```

Output

```text
swarm, check_every 0 (test off) ReachedIterationBudget minimum 0.00e+00 evaluations 16440 generations 400
swarm, check_every 10 Converged minimum 7.02e-29 evaluations 2501 generations 60
swarm, max_generations 15 ReachedIterationBudget minimum 2.00e-04 evaluations 655 generations 15
swarm, max_evaluations 500 ReachedEvaluationBudget minimum 2.59e-09 evaluations 532 generations 12
swarm, sequential mode ReachedIterationBudget minimum 0.00e+00 evaluations 16440 generations 400
swarm-DV, check_every 0 (test off) ReachedEvaluationBudget minimum 0.00e+00 evaluations 16042 generations 399
swarm-DV, check_every 10 Converged minimum 6.41e-27 evaluations 7642 generations 190
```
With the test off the run uses its whole generation budget. With `check_every` 10 it stops once the best value has stopped improving and the swarm has collapsed, here after 60 generations on a unimodal function, with the answer already below 1e-28. For rugged functions leave the test off and rely on a budget, or the swarm may collapse on a local minimum and stop there. The sequential mode updates particles one at a time and follows a different random path; both give valid results. A generation limit reports `ReachedIterationBudget`, an evaluation limit `ReachedEvaluationBudget`.

## Decisions and pitfalls

- **Use at least 5 to 10 particles per variable.** A swarm of 30 failed on one seed in three, with too few particles to cover a rugged box; use more for rugged functions.
- **Keep the inertia schedule when you need full precision.** A constant inertia of 0.7 settles to about 1e-14 instead of exactly 0 because the particles never quite stop.
- **Tune one setting at a time and over several seeds.** The pull weights, the speed clamp and the centre particle made no difference here; they matter on harder functions.
- **The variant is not a drop-in improvement.** At six variables with the same budget the classic swarm reached essentially zero on two of three seeds and the variant stopped between 12 and 18, because it moves each particle only along one differential direction.
- **For rugged functions leave the stopping test off and rely on a budget.** Otherwise the swarm may collapse before it has found the global basin.

Where next: [Chapter 10](../dfo_10_mesh_adaptive_search.md) describes the mesh adaptive direct search family.
