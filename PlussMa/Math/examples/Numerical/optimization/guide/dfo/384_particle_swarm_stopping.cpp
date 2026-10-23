// Stopping and modes for both swarms. The early-stop test (`check_every` > 0) declares convergence when the best value has
// stopped improving relatively and the swarm has collapsed; `max_generations` and `max_evaluations` are the budgets. The
// sequential execution mode updates particles one at a time and gives a different (equally valid) random path.
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
