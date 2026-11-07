// Stopping and modes for both swarms. The early-stop test (`check_every` > 0) declares convergence when the best value has
// stopped improving relatively and the swarm has collapsed; `max_generations` and `max_evaluations` are the budgets. The
// sequential execution mode updates particles one at a time and gives a different (equally valid) random path.
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

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
