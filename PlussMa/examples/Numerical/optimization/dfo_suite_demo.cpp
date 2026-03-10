#include <pluss/ma.h>

#include <iomanip>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

pluss::Config config_like(const pluss::Series& x) {
  return pluss::Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
}

pluss::Series scalar_like(const pluss::Series& x, double v) {
  return pluss::make_series(v, config_like(x));
}

double as_f64(const pluss::Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
}

pluss::Series ackley(const pluss::Series& x) {
  const pluss::Series sq_mean = x.square().mean();
  const pluss::Series cos_mean = x.mul(2.0 * 3.14159265358979323846).cos().mean();

  const pluss::Series twenty = scalar_like(x, 20.0);
  const pluss::Series e = scalar_like(x, 2.71828182845904523536);
  const pluss::Series t1 = sq_mean.sqrt().mul(-0.2).exp().mul(20.0);
  const pluss::Series t2 = cos_mean.exp();

  return twenty.add(e).sub(t1).sub(t2);
}

std::string to_string(pluss::subsp::math::optimization::PopulationSolverStatus s) {
  using pluss::subsp::math::optimization::PopulationSolverStatus;
  switch (s) {
    case PopulationSolverStatus::Converged:
      return "Converged";
    case PopulationSolverStatus::ReachedIterationBudget:
      return "ReachedIterationBudget";
    case PopulationSolverStatus::ReachedEvaluationBudget:
      return "ReachedEvaluationBudget";
  }
  return "Unknown";
}

std::string to_string(pluss::subsp::math::optimization::BlackboxSolverStatus s) {
  using pluss::subsp::math::optimization::BlackboxSolverStatus;
  switch (s) {
    case BlackboxSolverStatus::Converged:
      return "Converged";
    case BlackboxSolverStatus::ReachedIterationBudget:
      return "ReachedIterationBudget";
    case BlackboxSolverStatus::ReachedEvaluationBudget:
      return "ReachedEvaluationBudget";
  }
  return "Unknown";
}

void print_outcome(
    const std::string& name,
    const pluss::subsp::math::optimization::PopulationSolverOutcome& out) {
  const pluss::Series x = out.minimizer.to(pluss::pCPU, pluss::ScalarType::Float64);
  const pluss::Series norm = x.square().sum().sqrt();

  std::cout << name
            << " status=" << to_string(out.status)
            << " iter=" << out.diagnostics.iterations
            << " evals=" << out.diagnostics.evaluations
            << " f=" << out.minimum
            << " ||x||=" << as_f64(norm)
            << " step=" << out.diagnostics.last_step
            << '\n';
}

void print_outcome(
    const std::string& name,
    const pluss::subsp::math::optimization::BlackboxSolverOutcome& out) {
  const pluss::Series x = out.minimizer.to(pluss::pCPU, pluss::ScalarType::Float64);
  const pluss::Series norm = x.square().sum().sqrt();

  std::cout << name
            << " status=" << to_string(out.status)
            << " iter=" << out.diagnostics.iterations
            << " evals=" << out.diagnostics.evaluations
            << " f=" << out.minimum
            << " ||x||=" << as_f64(norm)
            << " phase=" << out.diagnostics.last_phase
            << '\n';
}

} // namespace

int main() {
  using namespace pluss::subsp::math::optimization;

  const std::int64_t dim = 12;
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x0 = pluss::full({dim}, 2.5, cfg);
  const pluss::Series lb = pluss::full({dim}, -5.0, cfg);
  const pluss::Series ub = pluss::full({dim}, 5.0, cfg);

  DifferentialEvolutionConfig de_cfg;
  de_cfg.population_size = 96;
  de_cfg.max_generations = 600;
  de_cfg.max_evaluations = 0;
  de_cfg.check_every = 10;
  de_cfg.relative_improvement_tolerance = 1e-10;
  de_cfg.mutation_factor = 0.85;
  de_cfg.crossover_rate = 0.9;
  de_cfg.best_mix = 0.15;
  de_cfg.lower_bounds = lb;
  de_cfg.upper_bounds = ub;
  de_cfg.rng_seed = 20260225ULL;

  DifferentialEvolutionPrmmConfig de_prmm_cfg;
  de_prmm_cfg.population_size = 112;
  de_prmm_cfg.elite_count = 24;
  de_prmm_cfg.max_generations = 700;
  de_prmm_cfg.max_evaluations = 0;
  de_prmm_cfg.check_every = 10;
  de_prmm_cfg.reduction_passes = 2;
  de_prmm_cfg.relative_improvement_tolerance = 1e-10;
  de_prmm_cfg.lower_bounds = lb;
  de_prmm_cfg.upper_bounds = ub;
  de_prmm_cfg.rng_seed = 20260226ULL;

  ParticleSwarmConfig pso_cfg;
  pso_cfg.population_size = 90;
  pso_cfg.max_generations = 700;
  pso_cfg.max_evaluations = 0;
  pso_cfg.check_every = 10;
  pso_cfg.relative_improvement_tolerance = 1e-10;
  pso_cfg.center_particle = true;
  pso_cfg.linear_inertia_schedule = true;
  pso_cfg.time_varying_acceleration = true;
  pso_cfg.lower_bounds = lb;
  pso_cfg.upper_bounds = ub;
  pso_cfg.rng_seed = 20260227ULL;

  ParticleSwarmDvConfig pso_dv_cfg;
  pso_dv_cfg.population_size = 90;
  pso_dv_cfg.max_generations = 700;
  pso_dv_cfg.max_evaluations = 0;
  pso_dv_cfg.check_every = 10;
  pso_dv_cfg.relative_improvement_tolerance = 1e-10;
  pso_dv_cfg.stagnation_limit = 40;
  pso_dv_cfg.lower_bounds = lb;
  pso_dv_cfg.upper_bounds = ub;
  pso_dv_cfg.rng_seed = 20260228ULL;

  MadsBlackboxConfig ortho_cfg;
  ortho_cfg.max_iterations = 800;
  ortho_cfg.max_evaluations = 4000;
  ortho_cfg.check_every = 10;
  ortho_cfg.search_points = 8;
  ortho_cfg.relative_improvement_tolerance = 1e-10;
  ortho_cfg.mesh_tolerance = 1e-7;
  ortho_cfg.lower_bounds = lb;
  ortho_cfg.upper_bounds = ub;
  ortho_cfg.poll_scheme = MadsPollScheme::Ortho2N;
  ortho_cfg.rng_seed = 20260229ULL;

  MadsBlackboxConfig lt_cfg = ortho_cfg;
  lt_cfg.poll_scheme = MadsPollScheme::LtMadsRandom2N;
  lt_cfg.rng_seed = 20260230ULL;

  std::cout << std::fixed << std::setprecision(8);
  std::cout << "Ackley minimization in " << dim << "D using Series-based DFO solvers\n";

  const auto de_out = differential_evolution_minimizer(ackley, x0, de_cfg);
  print_outcome("de", de_out);

  const auto de_prmm_out = differential_evolution_prmm_minimizer(ackley, x0, de_prmm_cfg);
  print_outcome("de_prmm", de_prmm_out);

  const auto pso_out = particle_swarm_minimizer(ackley, x0, pso_cfg);
  print_outcome("pso", pso_out);

  const auto pso_dv_out = particle_swarm_dv_minimizer(ackley, x0, pso_dv_cfg);
  print_outcome("pso_dv", pso_dv_out);

  const auto ortho_out = orthomads_minimizer(ackley, x0, ortho_cfg);
  print_outcome("orthomads", ortho_out);

  const auto lt_out = ltmads_minimizer(ackley, x0, lt_cfg);
  print_outcome("ltmads", lt_out);

  return 0;
}
