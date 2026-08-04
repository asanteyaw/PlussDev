#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace pluss::subsp::math::optimization;

std::string status_string(BlackboxSolverStatus s) {
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

BlackboxEvaluation constrained_multiobjective_blackbox(const pluss::Series& x) {
  BlackboxEvaluation out;

  const pluss::Series f1 = x.sub(1.0).square().sum();
  const pluss::Series f2 = x.add(1.0).square().sum().add(x.sum().mul(5.0).sin().abs().mul(0.05));

  const pluss::Series c1 = x.sum().sub(1.2);
  const pluss::Series c2 = x.square().sum().sub(3.0);

  const double f1v = f1.to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
  const double f2v = f2.to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
  const double c1v = c1.to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();
  const double c2v = c2.to(pluss::pCPU, pluss::ScalarType::Float64).item<double>();

  out.objectives = pluss::make_series({f1v, f2v}, pluss::Config{}.with_dtype(x.scalar_type()).with_platform(x.platform()));
  out.constraints = pluss::make_series({c1v, c2v}, pluss::Config{}.with_dtype(x.scalar_type()).with_platform(x.platform()));

  if (x.abs().amax().item<double>() > 4.5) {
    out.evaluation_ok = false;
  }
  return out;
}

} // namespace

int main() {
  using namespace pluss::subsp::math::optimization;

  const pluss::Config cfg = pluss::Config{}
                                .with_dtype(pluss::ScalarType::Float64)
                                .with_platform(pluss::pCPU);

  const std::int64_t dim = 3;
  const pluss::Series x0 = pluss::series::full({dim}, 1.8, cfg);
  const pluss::Series lb = pluss::series::full({dim}, -5.0, cfg);
  const pluss::Series ub = pluss::series::full({dim}, 5.0, cfg);

  MadsBlackboxAdvancedConfig config;
  config.max_iterations = 220;
  config.max_evaluations = 800;
  config.max_total_evaluations = 1200;
  config.check_every = 10;
  config.search_points = 10;
  config.relative_improvement_tolerance = 1e-9;
  config.mesh_tolerance = 1e-7;
  config.lower_bounds = lb;
  config.upper_bounds = ub;
  config.poll_scheme = MadsPollScheme::Ortho2N;
  config.constraint_handling = ConstraintHandling::ProgressiveBarrier;
  config.progressive_barrier_penalty = 80.0;
  config.multiobjective_scalarization = MultiObjectiveScalarization::Chebyshev;
  config.objective_weights = pluss::make_series({0.7, 0.3}, cfg);
  config.objective_reference = pluss::make_series({0.0, 0.0}, cfg);
  config.enable_cache = true;
  config.cache_max_entries = 100000;
  config.surrogate_mode = SurrogateMode::CandidateOrdering;
  config.surrogate_min_points = 14;
  config.surrogate_max_points = 300;
  config.surrogate_bandwidth = 0.0;
  config.pareto_archive_limit = 128;
  config.rng_seed = 20260226ULL;

  const auto out = orthomads_blackbox_minimizer(constrained_multiobjective_blackbox, x0, config);

  CoopMadsConfig coop_cfg;
  coop_cfg.base = config;
  coop_cfg.cooperative_rounds = 3;
  coop_cfg.subproblem_size = 2;
  coop_cfg.subproblems_per_round = 2;
  coop_cfg.rng_seed = 20260227ULL;
  const auto coop_out = coopmads_blackbox_minimizer(constrained_multiobjective_blackbox, x0, coop_cfg);

  PsdMadsConfig psd_cfg;
  psd_cfg.base = config;
  psd_cfg.rounds = 3;
  psd_cfg.min_subproblem_size = 1;
  psd_cfg.max_subproblem_size = 2;
  psd_cfg.subproblems_per_round = 3;
  psd_cfg.rng_seed = 20260228ULL;
  const auto psd_out = psdmads_blackbox_minimizer(constrained_multiobjective_blackbox, x0, psd_cfg);

  DiscoMadsConfig disco_cfg;
  disco_cfg.base = config;
  disco_cfg.rescue_trials = 6;
  disco_cfg.rescue_radius = 0.35;
  disco_cfg.rng_seed = 20260229ULL;
  const auto disco_out = discomads_blackbox_minimizer(constrained_multiobjective_blackbox, x0, disco_cfg);

  VnsMadsConfig vns_cfg;
  vns_cfg.base = config;
  vns_cfg.neighborhoods = 4;
  vns_cfg.initial_kick_scale = 0.15;
  vns_cfg.kick_growth = 1.6;
  vns_cfg.rng_seed = 20260230ULL;
  const auto vns_out = vnsmads_blackbox_minimizer(constrained_multiobjective_blackbox, x0, vns_cfg);

  std::cout << std::fixed << std::setprecision(8);
  std::cout << "Native Pluss Blackbox MADS Demo\n";
  std::cout << "status=" << status_string(out.status)
            << " iter=" << out.diagnostics.iterations
            << " bb_evals=" << out.diagnostics.evaluations
            << " total_evals=" << out.diagnostics.total_evaluations
            << " scalarized=" << out.scalarized_minimum << "\n";

  std::cout << "cache_hits=" << out.diagnostics.cache_hits
            << " surrogate_orderings=" << out.diagnostics.surrogate_orderings
            << " pareto_size=" << out.diagnostics.pareto_size
            << " obj_dim=" << out.diagnostics.objective_dimension
            << " constr_dim=" << out.diagnostics.constraint_dimension
            << " phase=" << out.diagnostics.last_phase << "\n";

  std::cout << "best x=" << out.minimizer.to(pluss::pCPU, pluss::ScalarType::Float64) << "\n";
  std::cout << "best objectives=" << out.objective_vector.to(pluss::pCPU, pluss::ScalarType::Float64) << "\n";

  if (out.pareto_objectives.defined()) {
    std::cout << "pareto objectives shape=" << out.pareto_objectives.sizes()[0]
              << "x" << out.pareto_objectives.sizes()[1] << "\n";
  }

  auto print_brief = [&](const char* name, const BlackboxAdvancedOutcome& r) {
    std::cout << name
              << " status=" << status_string(r.status)
              << " scalarized=" << r.scalarized_minimum
              << " evals=" << r.diagnostics.evaluations
              << " total=" << r.diagnostics.total_evaluations
              << " cache_hits=" << r.diagnostics.cache_hits
              << " surrogate_orderings=" << r.diagnostics.surrogate_orderings
              << " pareto=" << r.diagnostics.pareto_size
              << " phase=" << r.diagnostics.last_phase
              << "\n";
  };

  print_brief("COOP", coop_out);
  print_brief("PSD", psd_out);
  print_brief("DISCO", disco_out);
  print_brief("VNS", vns_out);

  return 0;
}
