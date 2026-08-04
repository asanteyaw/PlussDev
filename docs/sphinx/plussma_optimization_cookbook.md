# PlussMa Optimization Cookbook

This cookbook is task-first and covers the full PlussMa optimization surface:

- constrained smooth optimization (`pluss::optim`)
- functional Adam/Septima loops (`pluss::optim`)
- derivative-free global/local/blackbox optimization (`pluss::subsp::math::optimization`)

## Recipe 1: Minimal Unconstrained `optima` (Autodiff Objective)

### API Signature

- `optima(objective_autodiff, initial: Series, options: OptimaOptions = {}, constraints: ObjectiveConstraints = {}) -> OptimaState`

### Notes

Fastest way to solve smooth unconstrained problems with one vector parameter.
- Use a `Trace` objective callback so derivatives come from autodiff.

### Inputs

- scalar objective callback from `Trace -> Trace`
- initial point

### Output

- `OptimaState` with converged parameter estimate and diagnostics

### Options

- `OptimaOptions.algorithm` defaults to `SequentialQP`

### Examples

```cpp
#include <pluss/ma.h>
#include <iostream>

namespace pmo = pluss::optim;

int main() {
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series x0 = pluss::make_series(std::vector<double>{2.0, -1.0}, {2}, cfg);

  pmo::OptimaOptions options = pmo::presets::sequential_qp();
  options.sequential_qp.max_iterations = 120;
  options.sequential_qp.optimality_tolerance = 1e-8;

  const pmo::OptimaState out = pmo::optima(
      [](const pmo::Trace& x) -> pmo::Trace { return pmo::sum(pmo::square(x - 1.0)); },
      x0,
      options);

  std::cout << out.message << "\n";
}
```

### Expected output

- converged state with objective near zero and parameter near `[1, 1]`.

## Recipe 2: Bound + Linear Constraints

### API Signature

- `constraints().lbs(...).ubs(...).ln_eq(...).ln_ineq(...).build() -> ObjectiveConstraints`
- `optima(..., constraints: ObjectiveConstraints) -> OptimaState`

### Notes

Use `ObjectiveConstraintsBuilder` when objective is simple and constraints are the only extra setup.

### Inputs

- bounds and linear matrices

### Output

- constrained optimum satisfying linear/bound requirements

### Options

- use `constraint_tolerance` and `active_set_tolerance` for strict feasibility targets

### Examples

```cpp
pmo::ObjectiveConstraints c = pmo::constraints()
    .lbs(pluss::make_series(std::vector<double>{0.0, 0.0}, {2}))
    .ubs(pluss::make_series(std::vector<double>{3.0, 3.0}, {2}))
    .ln_ineq(pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2}),
             pluss::make_series(std::vector<double>{2.5}, {1}))
    .build();

const auto out = pmo::optima(
    [](const pmo::Trace& x) -> pmo::Trace {
      return pmo::square(x.select(0, 0) - 1.0) + pmo::square(x.select(0, 1) - 2.0);
    },
    pluss::make_series(std::vector<double>{0.3, 0.3}, {2}),
    pmo::presets::sequential_qp(),
    c);
```

### Expected output

- solution inside bounds and satisfying `x0 + x1 <= 2.5`.

## Recipe 3: Nonlinear Constraints With Autodiff Objective

### API Signature

- `problem().objective(...).nl_eq(...).nl_ineq(...).build() -> ConstrainedProblem`
- `optima(problem, initial, options) -> OptimaState`

### Notes

Best pattern for complex constrained NLP where you need explicit problem construction.

### Inputs

- `Trace` objective and nonlinear constraint callbacks

### Output

- constrained optimum with autodiff-derived derivatives

### Options

- use `InternalProtocol` for harder nonlinear constrained problems

### Examples

```cpp
using namespace pluss;
namespace pmo = pluss::optim;

auto pb = pmo::problem();
pb.objective([](const pmo::ParameterPack<pmo::Trace>& p) -> pmo::Trace {
  const pmo::Trace x = p.at("x");
  return pmo::sum(pmo::square(x - 1.0));
});
pb.nl_eq([](const pmo::ParameterPack<pmo::Trace>& p) -> pmo::Trace {
  return pmo::sum(p.at("x")) - 2.0;
});
pb.nl_ineq([](const pmo::ParameterPack<pmo::Trace>& p) -> pmo::Trace {
  return pmo::sum(pmo::square(p.at("x"))) - 5.0;
});

pmo::OptimaOptions options = pmo::presets::robust_internal_protocol();
const auto out = pmo::optima(pb.build(), pmo::pack_series(make_series(std::vector<double>{0.8, 0.8}, {2})), options);
```

### Expected output

- feasible constrained optimum with small stationarity and constraint violation.

## Recipe 4: Migrate `objective_fn(...)` To AD-Only `objective(...)`

### API Signature

- `problem().objective((Trace|Series|ParameterPack<Trace>) -> (Trace|Series)).build()`

### Notes

`objective_fn(...)` is removed from the `optima` problem builder.  
Use `objective(...)` and return `Trace` or `Series` (single-block `"x"` form).

If your old objective mutates model internals (for example `set_params` with `set_data`), gradients can become disconnected.
For AD-only behavior, write the objective as a pure function of traced params.

### Example (NGARCH-style scalar recursion)

```cpp
using namespace pluss;
namespace pmo = pluss::optim;

const Series scaler = /* user scaler */;
const Series returns = /* 1D returns */;

auto pb = pmo::problem();
pb.objective([&returns, &scaler](const pmo::Trace& x) -> pmo::Trace {
  const Series p = x.value() * scaler; // traced params (scaled)
  const Series omega  = p.pos(0);
  const Series alpha  = p.pos(1);
  const Series phi    = p.pos(2);
  const Series lambda = p.pos(3);
  const Series gamma  = p.pos(4);

  const double kLog2Pi = std::log(2.0 * 3.14159265358979323846);
  Series h_t = returns.var().clamp_min(1e-12);
  Series z_t = (returns.pos(0) - lambda * h_t) / h_t.sqrt();
  Series nll = 0.5 * (h_t.log() + z_t.square() + kLog2Pi);

  for (int64_t t = 1; t < returns.size(0); ++t) {
    h_t = omega + phi * (h_t - omega)
          + alpha * (z_t.square() - 1.0 - 2.0 * gamma * h_t.sqrt() * z_t);
    h_t = h_t.clamp_min(1e-12);
    z_t = (returns.pos(t) - lambda * h_t) / h_t.sqrt();
    nll = nll + 0.5 * (h_t.log() + z_t.square() + kLog2Pi);
  }
  return pmo::trace(nll);
});

// bounds + solve as usual
const pmo::ConstrainedProblem problem = pb.build();
```

### Expected output

- non-static objective iterations (assuming feasible scaling/bounds and valid data)
- gradients connected to optimization parameters through AD

## Recipe 5: Working-Set First Strategy

### API Signature

- `OptimaOptions.algorithm = OptimaAlgorithm::WorkingSet`
- `solve_working_set(problem, initial, options.working_set) -> WorkingSetState`
- `optima(problem, initial, options) -> OptimaState`

### Notes

Use when active-constraint handling dominates convergence behavior.

### Inputs

- constrained problem and active-set-sensitive options

### Output

- working-set state through the unified `optima` API

### Options

- `favor_working_set_updates`
- `max_working_set_violations`
- `add_constraint_tolerance`
- `drop_multiplier_tolerance`
- `multiplier_smoothing`

### Examples

```cpp
// Assume `problem` and `init_params` were built as in Recipe 3.
pmo::OptimaOptions options = pmo::presets::sequential_qp();
options.algorithm = pmo::OptimaAlgorithm::WorkingSet;
options.working_set.max_iterations = 150;
options.working_set.favor_working_set_updates = true;
options.working_set.max_working_set_violations = 10;
options.working_set.add_constraint_tolerance = 1e-3;

const auto out = pmo::optima(problem, init_params, options);
```

### Expected output

- robust active-set evolution with better behavior on binding constraints.

## Recipe 6: Merit-Method Baseline

### API Signature

- `OptimaOptions.algorithm = OptimaAlgorithm::MeritMethod`
- `solve_penalty_method(problem, initial, options.penalty) -> PenaltyMethodState`

### Notes

MeritMethod is a strong baseline for quick feasibility/objective tradeoff experiments.

### Inputs

- constrained problem
- penalty options

### Output

- penalty-iteration trajectory and final state

### Options

- `penalty`
- `constraint_tolerance`
- `optimality_tolerance`
- `backtracking`, `armijo`

### Examples

```cpp
// Assume `problem` and `init_params` were built as in Recipe 3.
pmo::OptimaOptions options{};
options.algorithm = pmo::OptimaAlgorithm::MeritMethod;
options.penalty.penalty = 20.0;
options.penalty.max_iterations = 200;
options.penalty.constraint_tolerance = 1e-7;

const auto out = pmo::optima(problem, init_params, options);
```

### Expected output

- objective decreases while feasibility improves according to penalty strength.

## Recipe 6: Progress Tracing + Fail-Loud Summary

### API Signature

- `reports::iter(std::ostream&, int precision = 6) -> progress_hook`
- `reports::detailed_summary(std::ostream&, const OptimaState&) -> std::ostream&`

### Notes

Attach tracing before tuning so failures are actionable immediately.
This is the MATLAB `OutputFcn` analogue: the callback runs at solver start/iteration/done.

### Inputs

- stream + solver options

### Output

- per-iteration telemetry and terminal summary

### Options

- precision and inclusion flags from `ProgressTracePreset`

### Examples

```cpp
// Assume `problem` and `init_params` were built as in Recipe 3.
pmo::OptimaOptions options = pmo::presets::internal_protocol();
options.internal_protocol.progress_hook = pmo::reports::iter(std::cout, 8);
const auto out = pmo::optima(problem, init_params, options);
pmo::reports::detailed_summary(std::cout, out) << "\n";
```

```cpp
// OutputFcn-style intercept with early stop.
pmo::OptimaOptions options = pmo::presets::robust_internal_protocol();
options.progress_hook = [](const pmo::OptimaProgressSnapshot& s) {
  if (s.stage == pmo::OptimaProgressStage::Iteration) {
    // custom side effects: logging/checkpointing/safety guards
    if (s.iteration >= 50 && s.stationarity < 1e-6) return true; // stop
  }
  return false;
};
const auto out = pmo::optima(problem, init_params, options);
```

### Expected output

- readable iteration lines and complete final diagnostics.

## Recipe 7: Derivative Audit Before Long Runs

### API Signature

- `audit_optima_derivatives(problem, params, options = {}) -> DerivativeAuditReport`

### Notes

Run once to catch analytic/autodiff mismatch before expensive solver runs.

### Inputs

- problem with both analytic and autodiff callbacks

### Output

- pass/fail report with absolute and relative diffs

### Options

- `DerivativeAuditOptions.absolute_tolerance`
- `DerivativeAuditOptions.relative_tolerance`

### Examples

```cpp
// Assume `problem` and `init_params` were built as in Recipe 3.
pmo::DerivativeAuditOptions aopt{};
aopt.absolute_tolerance = 1e-8;
aopt.relative_tolerance = 1e-6;
const auto audit = pmo::audit_optima_derivatives(problem, init_params, aopt);
```

### Expected output

- `audit.passed == true` for consistent derivatives.

## Recipe 8: Functional Adam (Explicit Gradient Loop)

### API Signature

- `adam_state_like(...)`
- `adam_step_inplace(...)`

### Notes

Use explicit gradients for custom objectives outside `optima`.

### Inputs

- params + gradients

### Output

- iterative parameter updates

### Options

- tune `learning_rate`, `beta1`, `beta2`, `eps`, `weight_decay`

### Examples

```cpp
std::vector<pluss::Series> params{pluss::make_series(std::vector<double>{1.0, -2.0}, {2})};
pmo::AdamState st = pmo::adam_state_like(params);
pmo::AdamOptions opt{};
opt.learning_rate = 0.1;

for (int i = 0; i < 60; ++i) {
  std::vector<pluss::Series> grad{params[0] - pluss::make_series(std::vector<double>{3.0, 3.0}, {2})};
  pmo::adam_step_inplace(params, grad, st, opt);
}
```

### Expected output

- parameters approach target vector.

## Recipe 9: Functional Septima (Primal-Dual Constrained Update)

### API Signature

- `septima(...) -> SeptimaResult` (one-shot solver API in `pluss::optim`)

### Notes

Use one-shot constrained minimization with internal primal-dual updates.

### Inputs

- objective callback
- initial params
- `SeptimaConstraints`

### Output

- solved params and final state/diagnostics

### Options

- `SeptimaSolveOptions` convergence knobs
- `SeptimaSolveOptions.step` primal-dual knobs

### Examples

Single-block Trace objective:

```cpp
pmo::SeptimaSolveOptions opt{};
opt.max_iterations = 100;
opt.step.adam.learning_rate = 0.04;
opt.step.dual_learning_rate = 0.08;
opt.step.enable_constraint_autodiff = true;

pmo::SeptimaConstraints c{};
c.equalities = [](const std::vector<pluss::Series>& p) { return std::vector<pluss::Series>{p[0].sum() - 3.0}; };
c.inequalities = [](const std::vector<pluss::Series>& p) { return std::vector<pluss::Series>{p[0].square().sum() - 10.0}; };

const pluss::Series target = pluss::make_series(std::vector<double>{2.0, 1.0}, {2});
const pluss::Series x0 = pluss::make_series(std::vector<double>{0.5, -0.5}, {2});

const auto out = pmo::septima(
  [&](const pmo::Trace& x) -> pmo::Trace {
    return pmo::sum(pmo::square(x - target)) * 0.5;
  },
  x0,
  opt,
  c);
```

Pre-defined function (no lambda):

```cpp
pmo::Trace obj_trace(const pmo::Trace& x) {
  return pmo::sum(pmo::square(x)) * 0.5;
}

const auto out = pmo::septima(obj_trace, x0, opt, c);
```

Series objective:

```cpp
pluss::Series obj_series(const pluss::Series& x) {
  return x.square().sum() * 0.5;
}

const auto out = pmo::septima(obj_series, x0, opt, c);
```

Multi-block vector objective:

```cpp
std::vector<pluss::Series> x0_blocks{
  pluss::make_series(std::vector<double>{0.2, -0.3}, {2}),
  pluss::make_series(std::vector<double>{0.1}, {1})
};

const auto out = pmo::septima(
  [](const std::vector<pluss::Series>& p) -> pluss::Series {
    return p[0].square().sum() + p[1].square().sum();
  },
  x0_blocks,
  opt,
  c);
```

Named multi-block `ParameterPack` objective:

```cpp
const auto init = pmo::pack_vector({
  {"weights", pluss::make_series(std::vector<double>{0.2, -0.3}, {2})},
  {"bias",    pluss::make_series(std::vector<double>{0.1}, {1})}
});

const auto out = pmo::septima(
  [](const pmo::ParameterPack<pmo::Trace>& p) -> pmo::Trace {
    const auto w = p.at("weights");
    const auto b = p.at("bias");
    return pmo::sum(pmo::square(w)) + pmo::sum(pmo::square(b));
  },
  init,
  opt,
  c);
```

Progress hook:

```cpp
pmo::SeptimaSolveOptions opt_hook = opt;
opt_hook.progress_hook = [](const pmo::SeptimaProgressSnapshot& s) {
  std::cout << "iter=" << s.iteration
            << " f=" << s.objective
            << " ginf=" << s.gradient_inf_norm
            << " cinf=" << s.constraint_violation << "\n";
  return true;
};
```

### Expected output

- objective decreases while equality/inequality residuals reduce.

### PlussMa Septima vs PlussAI Minimax

- Use `pluss::optim::septima(...)` when you want one-shot solve semantics and a `SeptimaResult` output.
- Use `pluss::nn::optim::Minimax` when you are in a model training loop and want `optimizer.step()` control per iteration.
- They share primal-dual ideas, but their integration model is different: solver call (PlussMa) vs optimizer object (PlussAI).

## Recipe 10: Unified DFO Auto Selection

### API Signature

- `DFO::solve(problem, options) -> DfoResult`

### Notes

Best first pass for blackbox objective when you do not want to manually pick a solver.

### Inputs

- `DfoProblem` with objective and optional bounds/constraints

### Output

- `DfoResult` with selected solver and diagnostics

### Options

- `DfoOptions.solver = Auto`
- budget controls (`max_iterations`, `max_evaluations`, `max_total_evaluations`)

### Examples

```cpp
namespace dfo = pluss::subsp::math::optimization;
dfo::DfoProblem p;
p.initial = pluss::series::full({12}, 2.5, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
p.objective = [](const pluss::Series& x) { return x.square().sum(); };

dfo::DfoOptions o{};
o.solver = dfo::DfoSolverKind::Auto;
o.max_iterations = 700;
o.max_evaluations = 20000;
o.rng_seed = 20260226ULL;

const dfo::DfoResult out = dfo::DFO::solve(p, o);
```

### Expected output

- auto-selected solver with non-empty `selection_reason` and improved objective.

## Recipe 11: DFO Builder Chain

### API Signature

- `DFO::build(initial) -> DfoBuilder`
- `DfoBuilder::with_* -> DfoBuilder`
- `DfoBuilder::solve() -> DfoResult`

### Notes

Builder form is best for readable composition in scripts/demos.

### Inputs

- initial series and chained options/callbacks

### Output

- `DfoResult`

### Options

- use `with_solver(...)` to force algorithm family

### Examples

```cpp
const auto out = dfo::DFO::build(pluss::series::full({6}, 2.0, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)))
                     .with_objective([](const pluss::Series& x) { return x.square().sum(); })
                     .with_bounds(pluss::series::full({6}, -2.0), pluss::series::full({6}, 2.0))
                     .with_solver(dfo::DfoSolverKind::PatternSearch)
                     .with_max_iterations(400)
                     .with_seed(7)
                     .solve();
```

### Expected output

- pattern-search run with deterministic seed and bounded domain.

## Recipe 12: Local DFO With Nelder-Mead

### API Signature

- `nelder_mead_minimizer(objective, initial, config = {}) -> NelderMeadOutcome`

### Notes

Use as a local unconstrained baseline.

### Inputs

- scalar objective callback
- initial point

### Output

- local minimizer estimate

### Options

- simplex geometry and tolerances in `NelderMeadConfig`

### Examples

```cpp
dfo::NelderMeadConfig cfg{};
cfg.max_iterations = 1200;
cfg.position_tolerance = 1e-8;
cfg.value_tolerance = 1e-8;

const auto out = dfo::nelder_mead_minimizer(
    [](const pluss::Series& x) { return x.square().sum(); },
    pluss::make_series(std::vector<double>{1.0, -1.5}, {2}),
    cfg);
```

### Expected output

- converged/stopped local solution with status and diagnostics.

## Recipe 13: Constrained Pattern Search

### API Signature

- `pattern_search_minimizer(objective, initial, config = {}) -> PSOutcome`

### Notes

Pattern search handles mixed bounds/linear/nonlinear constraints without derivatives.

### Inputs

- objective + `PSConfig`

### Output

- `PSOutcome`

### Options

- `constraint_policy`
- `poll_pattern`
- `polling_order`
- `search_strategy`
- `mesh_scaling`

### Examples

```cpp
dfo::PSConfig ps{};
ps.max_iterations = 800;
ps.max_evaluations = 6000;
ps.constraint_policy = dfo::PSConstraintPolicy::AugmentedLagrangian;
ps.lower_bounds = pluss::series::full({2}, -2.0);
ps.upper_bounds = pluss::series::full({2}, 2.0);
ps.linear_ineq_matrix = pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2});
ps.linear_ineq_rhs = pluss::make_series(std::vector<double>{1.5}, {1});
ps.nonlinear_constraints = [](const pluss::Series& x) {
  dfo::PSConstraintValues c;
  c.inequality = x.square().sum().sub(1.5).reshape({1});
  return c;
};
```

### Expected output

- constrained minimizer with violation diagnostics and mesh statistics.

## Recipe 14: Global DFO (DE / DE-PRMM / PSO / PSO-DV)

### API Signature

- `differential_evolution_minimizer(...) -> PopulationSolverOutcome`
- `differential_evolution_prmm_minimizer(...) -> PopulationSolverOutcome`
- `particle_swarm_minimizer(...) -> PopulationSolverOutcome`
- `particle_swarm_dv_minimizer(...) -> PopulationSolverOutcome`

### Notes

Use for global search in rugged landscapes.

### Inputs

- objective + population config

### Output

- best-found solution and population diagnostics

### Options

- population size, generations/evaluations, stochastic seed, bounds, execution mode

### Examples

```cpp
dfo::DifferentialEvolutionConfig de{};
de.population_size = 96;
de.max_generations = 600;
de.lower_bounds = pluss::series::full({12}, -5.0);
de.upper_bounds = pluss::series::full({12}, 5.0);
de.rng_seed = 20260225ULL;
const pluss::Series x0 = pluss::series::full({12}, 2.5, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));

const auto out = dfo::differential_evolution_minimizer(
    [](const pluss::Series& x) { return x.square().mean().add(x.cos().mean()); },
    x0,
    de);
```

### Expected output

- globally improved objective with deterministic reproducibility under fixed seed.

## Recipe 15: MADS Blackbox (Advanced, Multiobjective, Cooperative Variants)

### API Signature

- `mads_blackbox_minimizer(...) -> BlackboxAdvancedOutcome`
- `orthomads_blackbox_minimizer(...) -> BlackboxAdvancedOutcome`
- `ltmads_blackbox_minimizer(...) -> BlackboxAdvancedOutcome`
- `coopmads_blackbox_minimizer(...) -> BlackboxAdvancedOutcome`
- `psdmads_blackbox_minimizer(...) -> BlackboxAdvancedOutcome`
- `discomads_blackbox_minimizer(...) -> BlackboxAdvancedOutcome`
- `vnsmads_blackbox_minimizer(...) -> BlackboxAdvancedOutcome`

### Notes

Use these for expensive blackbox optimization with constraints, caching, and surrogate-assisted ordering.

### Inputs

- `BlackboxEvaluation` callback
- advanced MADS config

### Output

- scalarized optimum + pareto archive (when multiobjective)

### Options

- `constraint_handling`
- `multiobjective_scalarization`
- cache/surrogate options
- cooperative/disco/vns strategy options

### Examples

```cpp
dfo::MadsBlackboxAdvancedConfig cfg{};
cfg.constraint_handling = dfo::ConstraintHandling::ProgressiveBarrier;
cfg.multiobjective_scalarization = dfo::MultiObjectiveScalarization::WeightedSum;
cfg.enable_cache = true;
cfg.surrogate_mode = dfo::SurrogateMode::CandidateOrdering;
cfg.max_total_evaluations = 8000;
cfg.lower_bounds = pluss::series::full({8}, -4.0);
cfg.upper_bounds = pluss::series::full({8}, 4.0);

const auto out = dfo::mads_blackbox_minimizer(
    [](const pluss::Series& x) {
      dfo::BlackboxEvaluation ev;
      ev.objectives = x.square().sum().reshape({1});
      ev.constraints = (x.sum() - 2.0).reshape({1});
      return ev;
    },
    pluss::series::full({8}, 1.0),
    cfg);
```

### Expected output

- advanced diagnostics and feasible blackbox solution with scalarized objective improvement.

## Option Atlas: `OptimaOptions` And Sub-Options

### API Signature

- `OptimaOptions`
- `PenaltyMethodOptions`
- `SequentialQPOptions`
- `WorkingSetOptions`
- `InternalProtocolOptions`

### Notes

This section enumerates all user-facing option fields for constrained smooth solvers.

### Inputs

- typed option structs

### Output

- solver behavior control

### Options

`PenaltyMethodOptions` fields:

- `max_iterations`
- `max_line_search_iterations`
- `initial_step_size`
- `backtracking`
- `armijo`
- `penalty`
- `constraint_tolerance`
- `optimality_tolerance`
- `project_bounds`
- `progress_hook`

`SequentialQPOptions` fields:

- `max_iterations`
- `max_line_search_iterations`
- `max_second_order_corrections`
- `max_restoration_iterations`
- `max_restoration_line_search_iterations`
- `initial_step_size`
- `backtracking`
- `armijo`
- `initial_penalty`
- `constraint_tolerance`
- `optimality_tolerance`
- `step_tolerance`
- `value_progress_tolerance`
- `active_set_tolerance`
- `regularization`
- `bfgs_curvature_tolerance`
- `second_order_correction_tolerance`
- `fraction_to_boundary`
- `pivot_threshold`
- `max_krylov_iterations`
- `iterative_dimension_threshold`
- `krylov_tolerance`
- `krylov_absolute_tolerance`
- `objective_limit`
- `project_bounds`
- `enable_restoration`
- `system_structure`
- `linear_system_solver`
- `hessian_approximation`
- `problem_scaling`
- `progress_hook`

`WorkingSetOptions` additional fields:

- `favor_working_set_updates`
- `max_working_set_violations`
- `add_constraint_tolerance`
- `drop_multiplier_tolerance`
- `feasibility_priority_ratio`
- `max_stalled_iterations`
- `multiplier_smoothing`
- `multiplier_retention_tolerance`

`InternalProtocolOptions` fields:

- `max_iterations`
- `max_line_search_iterations`
- `max_restoration_iterations`
- `max_restoration_line_search_iterations`
- `initial_step_size`
- `backtracking`
- `armijo`
- `initial_barrier_parameter`
- `minimum_barrier_parameter`
- `barrier_decay`
- `equality_penalty`
- `centrality_fraction`
- `dual_recentering_fraction`
- `feasibility_activation_factor`
- `constraint_tolerance`
- `optimality_tolerance`
- `fraction_to_boundary`
- `pivot_threshold`
- `initialization_margin`
- `bfgs_curvature_tolerance`
- `lbfgs_memory`
- `max_krylov_iterations`
- `iterative_dimension_threshold`
- `krylov_tolerance`
- `krylov_absolute_tolerance`
- `hvp_options`
- `objective_limit`
- `project_bounds`
- `enable_restoration`
- `enable_feasibility_phase`
- `system_structure`
- `linear_system_solver`
- `hessian_approximation`
- `problem_scaling`
- `progress_hook`

### Examples

```cpp
pmo::OptimaOptions o = pmo::presets::robust_internal_protocol();
o.internal_protocol.hessian_approximation = pmo::OptimaHessianApproximation::Lbfgs;
o.internal_protocol.linear_system_solver = pmo::OptimaLinearSystemSolver::ConjugateGradient;
o.internal_protocol.problem_scaling = pmo::OptimaProblemScaling::ObjectiveAndConstraints;
```

### Expected output

- predictable algorithm behavior tuned to problem conditioning and scale.

## Option Atlas: DFO Solver Families

### API Signature

- `DfoOptions`
- `NelderMeadConfig`
- `PSConfig`
- `DifferentialEvolutionConfig`
- `DifferentialEvolutionPrmmConfig`
- `ParticleSwarmConfig`
- `ParticleSwarmDvConfig`
- `MadsBlackboxConfig`
- `MadsBlackboxAdvancedConfig`
- `CoopMadsConfig`
- `PsdMadsConfig`
- `DiscoMadsConfig`
- `VnsMadsConfig`

### Notes

This section lists all DFO family option dimensions that users can tune directly.

### Inputs

- DFO options structs

### Output

- deterministic and reproducible blackbox/global optimization control

### Options

`DfoOptions` key fields:

- `solver`
- `max_iterations`
- `max_evaluations`
- `max_total_evaluations`
- `check_every`
- `relative_improvement_tolerance`
- `rng_seed`
- `promote_cpu_to_float64`
- `display_iter`
- `auto_policy`
- family configs: `nelder_mead`, `differential_evolution`, `differential_evolution_prmm`, `particle_swarm`, `particle_swarm_dv`, `pattern_search`, `mads`

`PSConfig` key dimensions:

- stopping tolerances (`mesh_tolerance`, `step_tolerance`, `value_tolerance`, constraint tolerances)
- budgets (`max_iterations`, `max_evaluations`, `time_limit_seconds`)
- mesh geometry (`initial_mesh_size`, `max_mesh_size`, expansion/contraction)
- polling/search enums (`poll_pattern`, `polling_order`, `search_strategy`)
- constraint policy (`constraint_policy`)
- scaling (`mesh_scaling`, NUPS parameters)
- caching, bounds, linear constraints, nonlinear callback
- random seed and numeric behavior

`MadsBlackboxAdvancedConfig` key dimensions:

- constraint handling mode
- scalarization mode + weights/reference
- cache controls
- surrogate controls
- heavy kernel platform/dtype controls
- pareto archive controls

### Examples

```cpp
dfo::DfoOptions o{};
o.solver = dfo::DfoSolverKind::MadsAdvanced;
o.mads.constraint_handling = dfo::ConstraintHandling::ProgressiveBarrier;
o.mads.multiobjective_scalarization = dfo::MultiObjectiveScalarization::Chebyshev;
o.mads.enable_cache = true;
o.mads.surrogate_mode = dfo::SurrogateMode::CandidateOrdering;
```

### Expected output

- explicit control over global/local search dynamics, constraints, and expensive evaluation policy.

## Troubleshooting Patterns

### API Signature

- `OptimaState.message`
- `OptimaState.exit_flag`
- `DfoResult.status`
- `to_string(PSStatus|DfoStatus|DfoSolverKind)`

### Notes

These checks diagnose the most common optimization failures quickly.

### Inputs

- solver outputs

### Output

- actionable error/failure categories

### Options

- tighten/relax tolerances, increase budgets, switch solver family

### Examples

```cpp
if (!out.converged) {
  std::cerr << "optima failed: " << out.message << "\n";
  std::cerr << "stall category: " << static_cast<int>(out.output.stall_category) << "\n";
}
```

### Expected output

- clear cause for non-convergence and direct next tuning action.
