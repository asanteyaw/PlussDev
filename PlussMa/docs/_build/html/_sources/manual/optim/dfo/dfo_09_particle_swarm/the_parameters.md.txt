<!-- generated from the_parameters.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The parameters

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

void run(const char* label, dfo::ParticleSwarmConfig c) {
  c.lower_bounds = filled(3, -5.12);
  c.upper_bounds = filled(3, 5.12);
  c.max_evaluations = 20000;
  std::cout << std::left << std::setw(30) << label;
  for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
    c.rng_seed = seed;
    const auto o = dfo::particle_swarm_minimizer(rastrigin, filled(3, 3.0), c);
    std::cout << std::scientific << std::setprecision(2) << o.minimum << "   ";
  }
  std::cout << "\n";
}

int main() {
  dfo::ParticleSwarmConfig base;
  run("defaults", base);
  auto small = base; small.population_size = 30;
  run("population_size 30", small);
  auto cog = base; cog.cognitive = 3.0; cog.social = 1.0;
  run("cognitive 3, social 1", cog);
  auto soc = base; soc.cognitive = 1.0; soc.social = 3.0;
  run("cognitive 1, social 3", soc);
  auto constant = base; constant.linear_inertia_schedule = false; constant.inertia = 0.7;
  run("constant inertia 0.7", constant);
  auto clamp = base; clamp.velocity_clamp_ratio = 0.1;
  run("velocity_clamp_ratio 0.1", clamp);
  auto tv = base; tv.time_varying_acceleration = true;
  run("time_varying_acceleration", tv);
  auto nocenter = base; nocenter.center_particle = false;
  run("center_particle off", nocenter);
}
```

Output

```text
defaults                      0.00e+00   0.00e+00   0.00e+00   
population_size 30            0.00e+00   9.95e-01   0.00e+00   
cognitive 3, social 1         0.00e+00   0.00e+00   0.00e+00   
cognitive 1, social 3         0.00e+00   0.00e+00   0.00e+00   
constant inertia 0.7          2.31e-14   0.00e+00   1.60e-14   
velocity_clamp_ratio 0.1      0.00e+00   0.00e+00   0.00e+00   
time_varying_acceleration     0.00e+00   0.00e+00   0.00e+00   
center_particle off           0.00e+00   0.00e+00   0.00e+00   
```
Each cell is one seed. On this problem nearly every setting finds the global minimum with every seed, so the table mainly shows that the defaults are forgiving. Where there is a difference:

- A **small swarm** (30 particles) fails on one seed in three, with value 0.995: too few particles to cover a rugged box. Use at least 5 to 10 particles per variable, and more for rugged functions.
- **Constant inertia** 0.7 settles to about 1e-14 instead of exactly 0: without the shrinking schedule the particles never quite stop moving. Keep the schedule on when you need full precision.
- The pull weights, the speed clamp, the time-varying pulls and the centre particle made no difference here. They matter on harder functions; change one at a time, over several seeds, if you tune.
