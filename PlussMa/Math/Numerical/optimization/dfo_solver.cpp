#include "dfo_solver.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace pluss::subsp::math::optimization {
namespace {

struct ProblemTraits {
  std::int64_t dimension{0};
  bool has_objective{false};
  bool has_constraints{false};
  bool has_blackbox{false};
  bool has_bounds{false};
};

struct Selection {
  DfoSolverKind solver{DfoSolverKind::Auto};
  std::string reason{};
};

double scalar_as_f64(const Series& x, const char* op) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error(std::string(op) + ": expected scalar Series");
  }
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

void require_valid_initial(const Series& initial, const char* op) {
  if (!initial.defined()) {
    throw std::runtime_error(std::string(op) + ": initial point is undefined");
  }
  if (initial.numel() < 1) {
    throw std::runtime_error(std::string(op) + ": initial point must be non-empty");
  }
  if (!initial.dtype().is_floating() || initial.dtype().is_complex()) {
    throw std::runtime_error(std::string(op) + ": initial point must be real floating-point");
  }
}

ProblemTraits inspect_problem(const DfoProblem& problem) {
  require_valid_initial(problem.initial, "DFO::solve");

  const bool lower_defined = problem.lower_bounds.defined();
  const bool upper_defined = problem.upper_bounds.defined();
  if (lower_defined != upper_defined) {
    throw std::runtime_error("DFO::solve: both lower_bounds and upper_bounds must be defined together");
  }
  if (lower_defined && upper_defined) {
    if (problem.lower_bounds.numel() != problem.initial.numel() ||
        problem.upper_bounds.numel() != problem.initial.numel()) {
      throw std::runtime_error("DFO::solve: bounds must have same number of elements as initial point");
    }
  }

  ProblemTraits out;
  out.dimension = problem.initial.numel();
  out.has_objective = static_cast<bool>(problem.objective);
  out.has_constraints = static_cast<bool>(problem.constraints);
  out.has_blackbox = static_cast<bool>(problem.blackbox);
  out.has_bounds = lower_defined && upper_defined;

  if (!out.has_objective && !out.has_blackbox) {
    throw std::runtime_error("DFO::solve: provide either objective or blackbox callback");
  }
  if (out.has_constraints && !out.has_objective && !out.has_blackbox) {
    throw std::runtime_error("DFO::solve: constraints require objective or blackbox callback");
  }
  return out;
}

Selection choose_solver(const ProblemTraits& traits, const DfoOptions& options) {
  if (options.solver != DfoSolverKind::Auto) {
    return Selection{options.solver, "explicit solver override"};
  }

  if (traits.has_blackbox) {
    return Selection{DfoSolverKind::MadsAdvanced, "blackbox callback provided"};
  }
  if (traits.has_constraints) {
    return Selection{DfoSolverKind::OrthoMads, "explicit nonlinear constraints detected"};
  }

  const auto& policy = options.auto_policy;
  if (!traits.has_bounds) {
    if (traits.dimension <= policy.low_dimension_threshold) {
      return Selection{DfoSolverKind::NelderMead,
                       "small unconstrained problem; simplex local search is preferred"};
    }
    if (policy.prefer_pso_for_high_dimension &&
        traits.dimension >= policy.high_dimension_threshold) {
      return Selection{DfoSolverKind::ParticleSwarm,
                       "high-dimensional unconstrained problem; swarm search is preferred"};
    }
    return Selection{DfoSolverKind::DifferentialEvolutionPrmm,
                     "unconstrained medium-dimensional problem; DE-PRMM balances exploration/exploitation"};
  }

  if (policy.prefer_pso_for_high_dimension &&
      traits.dimension >= policy.high_dimension_threshold) {
    return Selection{DfoSolverKind::ParticleSwarm,
                     "high-dimensional boxed problem; swarm search with bounds is preferred"};
  }
  return Selection{DfoSolverKind::DifferentialEvolution,
                   "boxed problem without explicit constraints; differential evolution is preferred"};
}

std::function<Series(const Series&)> make_scalar_objective(
    const DfoProblem& problem,
    const DfoSolverKind solver) {
  if (problem.objective) {
    auto objective = problem.objective;
    return [objective = std::move(objective), solver](const Series& x) -> Series {
      const Series out = objective(x);
      if (!out.defined() || out.numel() != 1 || out.dtype().is_complex()) {
        throw std::runtime_error("DFO::solve(" + to_string(solver) +
                                 "): objective must return real scalar Series");
      }
      return out;
    };
  }

  if (!problem.blackbox) {
    throw std::runtime_error("DFO::solve(" + to_string(solver) + "): objective callback is required");
  }

  auto blackbox = problem.blackbox;
  return [blackbox = std::move(blackbox), solver](const Series& x) -> Series {
    const BlackboxEvaluation eval = blackbox(x);
    if (!eval.objectives.defined() || eval.objectives.numel() != 1 ||
        eval.objectives.dtype().is_complex()) {
      throw std::runtime_error("DFO::solve(" + to_string(solver) +
                               "): blackbox objective must be real scalar for this solver");
    }
    if (eval.constraints.defined() && eval.constraints.numel() > 0) {
      throw std::runtime_error("DFO::solve(" + to_string(solver) +
                               "): constraints require a MADS/blackbox solver");
    }
    return eval.objectives;
  };
}

std::function<BlackboxEvaluation(const Series&)> make_blackbox_objective(const DfoProblem& problem) {
  if (problem.blackbox) {
    return problem.blackbox;
  }

  if (!problem.objective) {
    throw std::runtime_error("DFO::solve: blackbox adaptation requires objective callback");
  }

  auto objective = problem.objective;
  auto constraints = problem.constraints;
  return [objective = std::move(objective), constraints = std::move(constraints)](
             const Series& x) -> BlackboxEvaluation {
    BlackboxEvaluation out;
    out.objectives = objective(x);
    if (constraints) {
      out.constraints = constraints(x);
    }
    return out;
  };
}

void apply_bounds_if_defined(
    const DfoProblem& problem,
    Series* lower,
    Series* upper) {
  if (!problem.lower_bounds.defined() || !problem.upper_bounds.defined()) {
    return;
  }
  *lower = problem.lower_bounds;
  *upper = problem.upper_bounds;
}

NelderMeadConfig make_nelder_config(const DfoOptions& options) {
  NelderMeadConfig cfg = options.nelder_mead;
  if (options.max_iterations > 0) {
    cfg.max_iterations = options.max_iterations;
  }
  if (options.max_evaluations > 0) {
    cfg.max_evaluations = options.max_evaluations;
  }
  if (options.relative_improvement_tolerance > 0.0) {
    cfg.position_tolerance = options.relative_improvement_tolerance;
    cfg.value_tolerance = options.relative_improvement_tolerance;
  }
  cfg.promote_cpu_to_float64 = options.promote_cpu_to_float64;
  if (options.display_iter) {
    cfg.display = SolverDisplay::Iter;
  }
  return cfg;
}

DifferentialEvolutionConfig make_de_config(
    const DfoProblem& problem,
    const DfoOptions& options) {
  DifferentialEvolutionConfig cfg = options.differential_evolution;
  if (options.max_iterations > 0) {
    cfg.max_generations = options.max_iterations;
  }
  if (options.max_evaluations > 0) {
    cfg.max_evaluations = options.max_evaluations;
  }
  cfg.check_every = options.check_every;
  if (options.relative_improvement_tolerance > 0.0) {
    cfg.relative_improvement_tolerance = options.relative_improvement_tolerance;
  }
  cfg.rng_seed = options.rng_seed;
  cfg.promote_cpu_to_float64 = options.promote_cpu_to_float64;
  if (options.display_iter) {
    cfg.display = PopulationSolverDisplay::Iter;
  }
  apply_bounds_if_defined(problem, &cfg.lower_bounds, &cfg.upper_bounds);
  return cfg;
}

DifferentialEvolutionPrmmConfig make_de_prmm_config(
    const DfoProblem& problem,
    const DfoOptions& options) {
  DifferentialEvolutionPrmmConfig cfg = options.differential_evolution_prmm;
  if (options.max_iterations > 0) {
    cfg.max_generations = options.max_iterations;
  }
  if (options.max_evaluations > 0) {
    cfg.max_evaluations = options.max_evaluations;
  }
  cfg.check_every = options.check_every;
  if (options.relative_improvement_tolerance > 0.0) {
    cfg.relative_improvement_tolerance = options.relative_improvement_tolerance;
  }
  cfg.rng_seed = options.rng_seed;
  cfg.promote_cpu_to_float64 = options.promote_cpu_to_float64;
  if (options.display_iter) {
    cfg.display = PopulationSolverDisplay::Iter;
  }
  apply_bounds_if_defined(problem, &cfg.lower_bounds, &cfg.upper_bounds);
  return cfg;
}

ParticleSwarmConfig make_pso_config(
    const DfoProblem& problem,
    const DfoOptions& options) {
  ParticleSwarmConfig cfg = options.particle_swarm;
  if (options.max_iterations > 0) {
    cfg.max_generations = options.max_iterations;
  }
  if (options.max_evaluations > 0) {
    cfg.max_evaluations = options.max_evaluations;
  }
  cfg.check_every = options.check_every;
  if (options.relative_improvement_tolerance > 0.0) {
    cfg.relative_improvement_tolerance = options.relative_improvement_tolerance;
  }
  cfg.rng_seed = options.rng_seed;
  cfg.promote_cpu_to_float64 = options.promote_cpu_to_float64;
  if (options.display_iter) {
    cfg.display = PopulationSolverDisplay::Iter;
  }
  apply_bounds_if_defined(problem, &cfg.lower_bounds, &cfg.upper_bounds);
  return cfg;
}

ParticleSwarmDvConfig make_pso_dv_config(
    const DfoProblem& problem,
    const DfoOptions& options) {
  ParticleSwarmDvConfig cfg = options.particle_swarm_dv;
  if (options.max_iterations > 0) {
    cfg.max_generations = options.max_iterations;
  }
  if (options.max_evaluations > 0) {
    cfg.max_evaluations = options.max_evaluations;
  }
  cfg.check_every = options.check_every;
  if (options.relative_improvement_tolerance > 0.0) {
    cfg.relative_improvement_tolerance = options.relative_improvement_tolerance;
  }
  cfg.rng_seed = options.rng_seed;
  cfg.promote_cpu_to_float64 = options.promote_cpu_to_float64;
  if (options.display_iter) {
    cfg.display = PopulationSolverDisplay::Iter;
  }
  apply_bounds_if_defined(problem, &cfg.lower_bounds, &cfg.upper_bounds);
  return cfg;
}

MadsBlackboxAdvancedConfig make_mads_config(
    const DfoProblem& problem,
    const DfoOptions& options,
    const DfoSolverKind solver) {
  MadsBlackboxAdvancedConfig cfg = options.mads;
  if (options.max_iterations > 0) {
    cfg.max_iterations = options.max_iterations;
  }
  if (options.max_evaluations > 0) {
    cfg.max_evaluations = options.max_evaluations;
  }
  if (options.max_total_evaluations > 0) {
    cfg.max_total_evaluations = options.max_total_evaluations;
  }
  cfg.check_every = options.check_every;
  if (options.relative_improvement_tolerance > 0.0) {
    cfg.relative_improvement_tolerance = options.relative_improvement_tolerance;
  }
  cfg.rng_seed = options.rng_seed;
  cfg.promote_cpu_to_float64 = options.promote_cpu_to_float64;
  if (options.display_iter) {
    cfg.display = BlackboxSolverDisplay::Iter;
  }
  if (solver == DfoSolverKind::OrthoMads) {
    cfg.poll_scheme = MadsPollScheme::Ortho2N;
  } else if (solver == DfoSolverKind::LtMads) {
    cfg.poll_scheme = MadsPollScheme::LtMadsRandom2N;
  }
  apply_bounds_if_defined(problem, &cfg.lower_bounds, &cfg.upper_bounds);
  return cfg;
}

DfoStatus from_status(SolverStatus s) {
  switch (s) {
    case SolverStatus::Converged:
      return DfoStatus::Converged;
    case SolverStatus::ReachedIterationBudget:
      return DfoStatus::ReachedIterationBudget;
    case SolverStatus::ReachedEvaluationBudget:
      return DfoStatus::ReachedEvaluationBudget;
  }
  return DfoStatus::ReachedIterationBudget;
}

DfoStatus from_status(PopulationSolverStatus s) {
  switch (s) {
    case PopulationSolverStatus::Converged:
      return DfoStatus::Converged;
    case PopulationSolverStatus::ReachedIterationBudget:
      return DfoStatus::ReachedIterationBudget;
    case PopulationSolverStatus::ReachedEvaluationBudget:
      return DfoStatus::ReachedEvaluationBudget;
  }
  return DfoStatus::ReachedIterationBudget;
}

DfoStatus from_status(BlackboxSolverStatus s) {
  switch (s) {
    case BlackboxSolverStatus::Converged:
      return DfoStatus::Converged;
    case BlackboxSolverStatus::ReachedIterationBudget:
      return DfoStatus::ReachedIterationBudget;
    case BlackboxSolverStatus::ReachedEvaluationBudget:
      return DfoStatus::ReachedEvaluationBudget;
  }
  return DfoStatus::ReachedIterationBudget;
}

DfoResult run_nelder_mead(
    const DfoProblem& problem,
    const DfoOptions& options) {
  const auto objective = make_scalar_objective(problem, DfoSolverKind::NelderMead);
  const NelderMeadConfig cfg = make_nelder_config(options);
  const NelderMeadOutcome out = nelder_mead_minimizer(objective, problem.initial, cfg);

  DfoResult result;
  result.minimizer = out.minimizer;
  result.objective_vector = pluss::value(out.minimum);
  result.minimum = out.minimum;
  result.status = from_status(out.status);
  result.diagnostics.iterations = out.diagnostics.iterations;
  result.diagnostics.evaluations = out.diagnostics.evaluations;
  result.diagnostics.total_evaluations = out.diagnostics.evaluations;
  result.diagnostics.last_phase = out.diagnostics.last_step;
  return result;
}

DfoResult run_de(
    const DfoProblem& problem,
    const DfoOptions& options) {
  const auto objective = make_scalar_objective(problem, DfoSolverKind::DifferentialEvolution);
  const DifferentialEvolutionConfig cfg = make_de_config(problem, options);
  const PopulationSolverOutcome out = differential_evolution_minimizer(objective, problem.initial, cfg);

  DfoResult result;
  result.minimizer = out.minimizer;
  result.objective_vector = pluss::value(out.minimum);
  result.minimum = out.minimum;
  result.status = from_status(out.status);
  result.diagnostics.iterations = out.diagnostics.iterations;
  result.diagnostics.evaluations = out.diagnostics.evaluations;
  result.diagnostics.total_evaluations = out.diagnostics.evaluations;
  result.diagnostics.last_phase = out.diagnostics.last_step;
  return result;
}

DfoResult run_de_prmm(
    const DfoProblem& problem,
    const DfoOptions& options) {
  const auto objective = make_scalar_objective(problem, DfoSolverKind::DifferentialEvolutionPrmm);
  const DifferentialEvolutionPrmmConfig cfg = make_de_prmm_config(problem, options);
  const PopulationSolverOutcome out = differential_evolution_prmm_minimizer(objective, problem.initial, cfg);

  DfoResult result;
  result.minimizer = out.minimizer;
  result.objective_vector = pluss::value(out.minimum);
  result.minimum = out.minimum;
  result.status = from_status(out.status);
  result.diagnostics.iterations = out.diagnostics.iterations;
  result.diagnostics.evaluations = out.diagnostics.evaluations;
  result.diagnostics.total_evaluations = out.diagnostics.evaluations;
  result.diagnostics.last_phase = out.diagnostics.last_step;
  return result;
}

DfoResult run_pso(
    const DfoProblem& problem,
    const DfoOptions& options) {
  const auto objective = make_scalar_objective(problem, DfoSolverKind::ParticleSwarm);
  const ParticleSwarmConfig cfg = make_pso_config(problem, options);
  const PopulationSolverOutcome out = particle_swarm_minimizer(objective, problem.initial, cfg);

  DfoResult result;
  result.minimizer = out.minimizer;
  result.objective_vector = pluss::value(out.minimum);
  result.minimum = out.minimum;
  result.status = from_status(out.status);
  result.diagnostics.iterations = out.diagnostics.iterations;
  result.diagnostics.evaluations = out.diagnostics.evaluations;
  result.diagnostics.total_evaluations = out.diagnostics.evaluations;
  result.diagnostics.last_phase = out.diagnostics.last_step;
  return result;
}

DfoResult run_pso_dv(
    const DfoProblem& problem,
    const DfoOptions& options) {
  const auto objective = make_scalar_objective(problem, DfoSolverKind::ParticleSwarmDv);
  const ParticleSwarmDvConfig cfg = make_pso_dv_config(problem, options);
  const PopulationSolverOutcome out = particle_swarm_dv_minimizer(objective, problem.initial, cfg);

  DfoResult result;
  result.minimizer = out.minimizer;
  result.objective_vector = pluss::value(out.minimum);
  result.minimum = out.minimum;
  result.status = from_status(out.status);
  result.diagnostics.iterations = out.diagnostics.iterations;
  result.diagnostics.evaluations = out.diagnostics.evaluations;
  result.diagnostics.total_evaluations = out.diagnostics.evaluations;
  result.diagnostics.last_phase = out.diagnostics.last_step;
  return result;
}

DfoResult run_mads(
    const DfoProblem& problem,
    const DfoOptions& options,
    const DfoSolverKind solver) {
  const auto blackbox = make_blackbox_objective(problem);
  const MadsBlackboxAdvancedConfig cfg = make_mads_config(problem, options, solver);

  BlackboxAdvancedOutcome out;
  if (solver == DfoSolverKind::LtMads) {
    out = ltmads_blackbox_minimizer(blackbox, problem.initial, cfg);
  } else if (solver == DfoSolverKind::MadsAdvanced) {
    out = mads_blackbox_minimizer(blackbox, problem.initial, cfg);
  } else {
    out = orthomads_blackbox_minimizer(blackbox, problem.initial, cfg);
  }

  DfoResult result;
  result.minimizer = out.minimizer;
  result.objective_vector = out.objective_vector;
  result.minimum = out.scalarized_minimum;
  result.status = from_status(out.status);
  result.diagnostics.iterations = out.diagnostics.iterations;
  result.diagnostics.evaluations = out.diagnostics.evaluations;
  result.diagnostics.total_evaluations = out.diagnostics.total_evaluations;
  result.diagnostics.last_phase = out.diagnostics.last_phase;

  if (result.objective_vector.defined() && result.objective_vector.numel() == 1) {
    result.minimum = scalar_as_f64(result.objective_vector, "DFO::solve");
  }
  return result;
}

} // namespace

std::string to_string(DfoSolverKind solver) {
  switch (solver) {
    case DfoSolverKind::Auto:
      return "Auto";
    case DfoSolverKind::NelderMead:
      return "NelderMead";
    case DfoSolverKind::DifferentialEvolution:
      return "DifferentialEvolution";
    case DfoSolverKind::DifferentialEvolutionPrmm:
      return "DifferentialEvolutionPrmm";
    case DfoSolverKind::ParticleSwarm:
      return "ParticleSwarm";
    case DfoSolverKind::ParticleSwarmDv:
      return "ParticleSwarmDv";
    case DfoSolverKind::OrthoMads:
      return "OrthoMads";
    case DfoSolverKind::LtMads:
      return "LtMads";
    case DfoSolverKind::MadsAdvanced:
      return "MadsAdvanced";
  }
  return "Auto";
}

std::string to_string(DfoStatus status) {
  switch (status) {
    case DfoStatus::Converged:
      return "Converged";
    case DfoStatus::ReachedIterationBudget:
      return "ReachedIterationBudget";
    case DfoStatus::ReachedEvaluationBudget:
      return "ReachedEvaluationBudget";
  }
  return "ReachedIterationBudget";
}

DfoResult DFO::run(const DfoProblem& problem) const {
  const ProblemTraits traits = inspect_problem(problem);
  const Selection choice = choose_solver(traits, options_);

  DfoResult result;
  switch (choice.solver) {
    case DfoSolverKind::NelderMead:
      result = run_nelder_mead(problem, options_);
      break;
    case DfoSolverKind::DifferentialEvolution:
      result = run_de(problem, options_);
      break;
    case DfoSolverKind::DifferentialEvolutionPrmm:
      result = run_de_prmm(problem, options_);
      break;
    case DfoSolverKind::ParticleSwarm:
      result = run_pso(problem, options_);
      break;
    case DfoSolverKind::ParticleSwarmDv:
      result = run_pso_dv(problem, options_);
      break;
    case DfoSolverKind::OrthoMads:
    case DfoSolverKind::LtMads:
    case DfoSolverKind::MadsAdvanced:
      result = run_mads(problem, options_, choice.solver);
      break;
    case DfoSolverKind::Auto:
      throw std::runtime_error("DFO::solve: internal error, unresolved Auto solver selection");
  }

  result.selected_solver = choice.solver;
  result.selection_reason = choice.reason;
  return result;
}

DfoBuilder DFO::build() {
  return DfoBuilder{};
}

DfoBuilder DFO::build(const Series& initial) {
  DfoProblem p;
  p.initial = initial;
  return DfoBuilder(std::move(p));
}

DfoBuilder DfoBuilder::with_problem(const DfoProblem& problem) const {
  DfoBuilder next = *this;
  next.problem_ = problem;
  return next;
}

DfoBuilder DfoBuilder::with_options(const DfoOptions& options) const {
  DfoBuilder next = *this;
  next.options_ = options;
  return next;
}

DfoBuilder DfoBuilder::with_initial(const Series& initial) const {
  DfoBuilder next = *this;
  next.problem_.initial = initial;
  return next;
}

DfoBuilder DfoBuilder::with_bounds(const Series& lower, const Series& upper) const {
  DfoBuilder next = *this;
  next.problem_.lower_bounds = lower;
  next.problem_.upper_bounds = upper;
  return next;
}

DfoBuilder DfoBuilder::with_solver(const DfoSolverKind solver) const {
  DfoBuilder next = *this;
  next.options_.solver = solver;
  return next;
}

DfoBuilder DfoBuilder::with_seed(const std::uint64_t seed) const {
  DfoBuilder next = *this;
  next.options_.rng_seed = seed;
  return next;
}

DfoBuilder DfoBuilder::with_max_iterations(const std::int64_t max_iterations) const {
  DfoBuilder next = *this;
  next.options_.max_iterations = max_iterations;
  return next;
}

DfoBuilder DfoBuilder::with_max_evaluations(const std::int64_t max_evaluations) const {
  DfoBuilder next = *this;
  next.options_.max_evaluations = max_evaluations;
  return next;
}

DfoBuilder DfoBuilder::with_max_total_evaluations(const std::int64_t max_total_evaluations) const {
  DfoBuilder next = *this;
  next.options_.max_total_evaluations = max_total_evaluations;
  return next;
}

DfoBuilder DfoBuilder::with_check_every(const std::int64_t check_every) const {
  DfoBuilder next = *this;
  next.options_.check_every = check_every;
  return next;
}

DfoBuilder DfoBuilder::with_tolerance(const double tolerance) const {
  DfoBuilder next = *this;
  next.options_.relative_improvement_tolerance = tolerance;
  return next;
}

DfoBuilder DfoBuilder::with_display_iter(const bool enabled) const {
  DfoBuilder next = *this;
  next.options_.display_iter = enabled;
  return next;
}

DfoBuilder DfoBuilder::with_promote_cpu_to_float64(const bool enabled) const {
  DfoBuilder next = *this;
  next.options_.promote_cpu_to_float64 = enabled;
  return next;
}

DfoBuilder DfoBuilder::with_auto_policy(const DfoAutoPolicy& policy) const {
  DfoBuilder next = *this;
  next.options_.auto_policy = policy;
  return next;
}

DfoBuilder DfoBuilder::with_nelder_mead_config(const NelderMeadConfig& cfg) const {
  DfoBuilder next = *this;
  next.options_.nelder_mead = cfg;
  return next;
}

DfoBuilder DfoBuilder::with_de_config(const DifferentialEvolutionConfig& cfg) const {
  DfoBuilder next = *this;
  next.options_.differential_evolution = cfg;
  return next;
}

DfoBuilder DfoBuilder::with_de_prmm_config(const DifferentialEvolutionPrmmConfig& cfg) const {
  DfoBuilder next = *this;
  next.options_.differential_evolution_prmm = cfg;
  return next;
}

DfoBuilder DfoBuilder::with_pso_config(const ParticleSwarmConfig& cfg) const {
  DfoBuilder next = *this;
  next.options_.particle_swarm = cfg;
  return next;
}

DfoBuilder DfoBuilder::with_pso_dv_config(const ParticleSwarmDvConfig& cfg) const {
  DfoBuilder next = *this;
  next.options_.particle_swarm_dv = cfg;
  return next;
}

DfoBuilder DfoBuilder::with_mads_config(const MadsBlackboxAdvancedConfig& cfg) const {
  DfoBuilder next = *this;
  next.options_.mads = cfg;
  return next;
}

DfoBuilder DfoBuilder::with_objective(std::function<Series(const Series&)> objective) const {
  DfoBuilder next = *this;
  next.problem_.objective = std::move(objective);
  return next;
}

DfoBuilder DfoBuilder::with_constraints(std::function<Series(const Series&)> constraints) const {
  DfoBuilder next = *this;
  next.problem_.constraints = std::move(constraints);
  return next;
}

DfoBuilder DfoBuilder::with_blackbox(std::function<BlackboxEvaluation(const Series&)> blackbox) const {
  DfoBuilder next = *this;
  next.problem_.blackbox = std::move(blackbox);
  return next;
}

DfoResult DfoBuilder::solve() const {
  return DFO(options_).run(problem_);
}

} // namespace pluss::subsp::math::optimization
