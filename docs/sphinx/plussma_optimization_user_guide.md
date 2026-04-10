# PlussMa Optimization User Guide

This guide covers the full public optimization surface in PlussMa:

- constrained gradient-based optimization (`pluss::optim`)
- functional Adam and Minimax (`pluss::optim`)
- derivative-free optimization (`pluss::subsp::math::optimization`)

## Include And Namespace

### API Signature

- `#include <pluss/ma.h>`
- `namespace pmo = pluss::optim`
- `namespace dfo = pluss::subsp::math::optimization`

### Notes

`pluss::optim` is the core constrained optimization and functional optimizer surface.
DFO solvers live under `pluss::subsp::math::optimization`.

### Inputs

- PlussMa headers
- `Series` inputs (recommended `Float64` for numerical stability)

### Output

- optimization APIs available in one translation unit

### Options

- prefer `Config{}.with_dtype(ScalarType::Float64)` for most optimization workloads

### Examples

```cpp
#include <pluss/ma.h>

namespace pmo = pluss::optim;
namespace dfo = pluss::subsp::math::optimization;
```

### Expected output

- code compiles with both constrained and DFO optimization symbols available.

## Mental Model

### API Signature

- `ConstrainedProblem(objective, nonlinear, linear, bounds) -> ConstrainedProblem`
- `optima(problem: ConstrainedProblem, initial: ParameterPack<Series>|Series, options: OptimaOptions = {}) -> OptimaState`
- `DFO::solve(problem: DfoProblem, options: DfoOptions = {}) -> DfoResult`

### Notes

Use this split:

- smooth objective + gradients/AD: `pmo::optima(...)`
- blackbox/noisy/non-smooth objective: `dfo::*` solvers

### Inputs

- objective callback(s)
- initial parameters
- optional constraints and solver options

### Output

- solver state/result with final parameters, objective, diagnostics, and exit status

### Options

- algorithm selection (`OptimaAlgorithm`, `DfoSolverKind`)
- deterministic seeds (`rng_seed`)

### Examples

```cpp
// Smooth constrained: pmo::optima(...)
// Blackbox global: dfo::DFO::solve(...)
```

### Expected output

- you can route each task family to the correct solver stack without API mismatch.

## Core Problem Definition (Constrained)

### API Signature

- `problem() -> ProblemBuilder`
- `constraints() -> ObjectiveConstraintsBuilder`
- `pack_series(values: Series, name: string = "x") -> ParameterPack<Series>`
- `pack_vector(blocks: {name, Series}...) -> ParameterPack<Series>`
- `pack_scalars(blocks: {name, Series}...) -> ParameterPack<Series>`

### Notes

`ProblemBuilder` is the main ergonomic entrypoint for constrained optimization setup.

### Inputs

- objective callback (`objective`, `objective_dual`, `objective_analytic`)
- optional nonlinear/linear/bounds constraints

### Output

- `ConstrainedProblem`

### Options

- objective callback modes:
  - autodiff callback (`Trace`)
  - autodiff callback (`Series`) for single-block `"x"` problems
  - dual callback (`DualTrace`)
  - analytic callback (value + gradient)

### Examples

```cpp
using namespace pluss;
namespace pmo = pluss::optim;

const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
const Series x0 = make_series(std::vector<double>{0.2, 0.2}, {2}, cfg);
const Series target = make_series(std::vector<double>{1.0, 2.0}, {2}, cfg);

auto pb = pmo::problem();
pb.objective([target](const pmo::Trace& x) -> pmo::Trace {
  return pmo::sum(pmo::square(x - pmo::trace(target, false)));
});

auto cb = pmo::constraints();
cb.ubs(make_series(std::vector<double>{3.0, 3.0}, {2}, cfg));
cb.lbs(make_series(std::vector<double>{0.0, 0.0}, {2}, cfg));
cb.ln_ineq(make_series(std::vector<double>{1.0, 1.0}, {1, 2}, cfg),
           make_series(std::vector<double>{2.8}, {1}, cfg));

pmo::ConstrainedProblem problem = pb.build();
pmo::ObjectiveConstraints objective_constraints = cb.build();
```

Single-block `Series` objective form (equivalent AD path):

```cpp
auto pb2 = pmo::problem();
pb2.objective([target](const Series& x) -> Series {
  return (x - target).square().sum();
});
```

### Expected output

- fully defined constrained problem ready for `optima(...)`.

## Solving With `optima`

### API Signature

- `optima(problem: ConstrainedProblem, initial: ParameterPack<Series>|Series, options: OptimaOptions = {}) -> OptimaState`
- `optima(objective_autodiff: function, initial: Series|ParameterPack<Series>, options: OptimaOptions = {}, constraints: ObjectiveConstraints = {}) -> OptimaState`
- `optima(objective_analytic: function, initial: Series|ParameterPack<Series>, options: OptimaOptions = {}, constraints: ObjectiveConstraints = {}) -> OptimaState`

### Notes

`optima(...)` overloads are designed to avoid boilerplate while preserving full control.
- `objective_autodiff`/`objective_analytic` provide direct derivative paths and are preferred for performance.

### Inputs

- objective callback in one of the supported modes
- initial parameter(s)
- `OptimaOptions`

### Output

- `OptimaState`:
  - `params` (final)
  - `evaluation` (objective + constraints)
  - `output` (diagnostics summary)
  - `message`, `exit_flag`, `converged`

### Options

- `OptimaOptions.algorithm`:
  - `MeritMethod`
  - `SequentialQP`
  - `WorkingSet`
  - `InternalProtocol`

`MeritMethod` is a real backend in `optima` (baseline penalty-method backend).  
For MATLAB-style constrained NLP behavior, prefer `SequentialQP` or `InternalProtocol`.

### Examples

```cpp
using namespace pluss;
namespace pmo = pluss::optim;

const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
const Series x0 = make_series(std::vector<double>{1.2, 0.9}, {2}, cfg);
const Series target = make_series(std::vector<double>{1.0, 1.0}, {2}, cfg);

pmo::OptimaOptions opts = pmo::presets::sequential_qp();
opts.sequential_qp.max_iterations = 120;
opts.sequential_qp.constraint_tolerance = 1e-8;
opts.sequential_qp.optimality_tolerance = 1e-8;

const pmo::OptimaState out = pmo::optima(
    [target](const pmo::Trace& x) -> pmo::Trace {
      return pmo::sum(pmo::square(x - pmo::trace(target, false)));
    },
    x0,
    opts);
```

### Expected output

- `out.converged` true for well-conditioned setup, with lower objective than initialization.

## Presets And Algorithm Selection

### API Signature

- `presets::sequential_qp() -> OptimaOptions`
- `presets::internal_protocol() -> OptimaOptions`
- `presets::robust_internal_protocol() -> OptimaOptions`

### Notes

Presets provide safe defaults and reduce option-surface setup time.

### Inputs

- none

### Output

- tuned `OptimaOptions` baseline

### Options

- edit preset fields after creation for task-specific tuning

### Examples

```cpp
pmo::OptimaOptions sqp = pmo::presets::sequential_qp();
pmo::OptimaOptions ipm = pmo::presets::internal_protocol();
pmo::OptimaOptions robust_ipm = pmo::presets::robust_internal_protocol();
```

### Expected output

- solver-ready option sets with deterministic defaults.

### Internal vs Robust Internal Protocol

Both presets select `OptimaAlgorithm::InternalProtocol`; `robust_internal_protocol()` is a higher-budget, safer default for hard constrained problems.

| Field | `internal_protocol()` | `robust_internal_protocol()` |
| --- | --- | --- |
| `max_iterations` | `100` | `160` |
| `max_restoration_iterations` | default | `10` |
| `max_restoration_line_search_iterations` | default | `20` |
| `enable_restoration` | default | `true` |
| `enable_feasibility_phase` | default | `false` |
| `hessian_approximation` | default | `Bfgs` |
| `linear_system_solver` | default | `Auto` (explicit) |
| `system_structure` | default | `Auto` (explicit) |

Rule of thumb:

- start with `internal_protocol()` when the model is clean and well-scaled
- use `robust_internal_protocol()` when you expect feasibility/recovery difficulty

## Progress, Summaries, And Diagnostics

### API Signature

- `reports::trace(os, preset = {}) -> function<bool(const OptimaProgressSnapshot&)>`
- `reports::iteration_only_trace(os, precision = 6) -> function<bool(const OptimaProgressSnapshot&)>`
- `reports::iter(os, precision = 6) -> function<bool(const OptimaProgressSnapshot&)>`
- `reports::compact_summary(os, state) -> ostream&`
- `reports::detailed_summary(os, state) -> ostream&`
- `print_summary(os, state) -> ostream&`

### Notes

Use progress hooks for transparent optimization traces and fail-loud diagnosis.
This is the MATLAB `OutputFcn` equivalent.

### Inputs

- output stream
- optional `ProgressTracePreset`
- `OptimaState`

### Output

- streamed progress rows and summary reports

### Options

`ProgressTracePreset`:

- `include_start`
- `include_done`
- `include_message`
- `include_krylov`
- `precision`

### Examples

```cpp
pmo::OptimaOptions opts = pmo::presets::sequential_qp();
opts.sequential_qp.progress_hook = pmo::reports::iter(std::cout, 8);
const auto out = pmo::optima(objective, x0, opts);
pmo::reports::detailed_summary(std::cout, out) << '\n';
```

```cpp
// Backend-agnostic hook (recommended): propagated to the selected solver
// when that solver-specific hook is not explicitly set.
pmo::OptimaOptions opts = pmo::presets::robust_internal_protocol();
opts.progress_hook = [](const pmo::OptimaProgressSnapshot& s) {
  if (s.stage == pmo::OptimaProgressStage::Iteration) {
    // inspect params, objective, violations, and decide what to do next
    if (s.evaluation.max_constraint_violation < 1e-8 && s.stationarity < 1e-6) {
      return true; // early-stop request (OutputFcn-style stop flag)
    }
  }
  return false;
};
const auto out = pmo::optima(problem, init_params, opts);
```

### Expected output

- per-iteration lines plus an end summary with stationarity/violation/exit diagnostics.

## Derivative Audit

### API Signature

- `audit_optima_derivatives(problem: ConstrainedProblem, params: ParameterPack<Series>, options: DerivativeAuditOptions = {}) -> DerivativeAuditReport`

### Notes

Run derivative audit when you provide both analytic and autodiff definitions.

### Inputs

- constrained problem with matching callback families
- representative parameter point

### Output

- `DerivativeAuditReport` for objective, nonlinear equalities, nonlinear inequalities

### Options

`DerivativeAuditOptions`:

- `absolute_tolerance`
- `relative_tolerance`

### Examples

```cpp
const auto report = pmo::audit_optima_derivatives(problem, pmo::pack_series(x0));
if (!report.passed) {
  std::cerr << "Derivative audit failed\n";
}
```

### Expected output

- pass/fail diagnostic with absolute and relative mismatch statistics.

## Functional Adam

### API Signature

- `adam_state_like(params: vector<Series>) -> AdamState`
- `adam_step_inplace(params: vector<Series>&, sensitivities: vector<Series>, state: AdamState&, options: AdamOptions = {}) -> void`
- `adam_step(params: vector<Series>, sensitivities: vector<Series>, state: AdamState&, options: AdamOptions = {}) -> vector<Series>`

### Notes

Use functional Adam outside DL-module optimizers for custom numerical training loops.

### Inputs

- parameter vectors and gradients/sensitivities

### Output

- updated parameters and optimizer state

### Options

`AdamOptions`:

- `learning_rate`
- `beta1`
- `beta2`
- `eps`
- `weight_decay`

### Examples

```cpp
std::vector<Series> params{make_series(std::vector<double>{1.0, -2.0}, {2})};
std::vector<Series> grads{params[0] - make_series(std::vector<double>{3.0, 3.0}, {2})};

pmo::AdamOptions opt{};
opt.learning_rate = 0.1;

pmo::AdamState state = pmo::adam_state_like(params);
pmo::adam_step_inplace(params, grads, state, opt);
```

### Expected output

- parameter vector moves toward lower objective in one update step.

## Functional Minimax

### API Signature

- `minimax_state_like(params: vector<Series>, constraints: MinimaxConstraints = {}) -> MinimaxState`
- `minimax_step_inplace(params: vector<Series>&, sensitivities: vector<Series>, state: MinimaxState&, options: MinimaxOptions = {}, constraints: MinimaxConstraints = {}) -> void`
- `minimax_step_inplace(params: vector<Series>&, state: MinimaxState&, options: MinimaxOptions = {}, constraints: MinimaxConstraints = {}) -> void`
- `minimax_step(params: vector<Series>, sensitivities: vector<Series>, state: MinimaxState&, options: MinimaxOptions = {}, constraints: MinimaxConstraints = {}) -> vector<Series>`
- `minimax_step(params: vector<Series>, state: MinimaxState&, options: MinimaxOptions = {}, constraints: MinimaxConstraints = {}) -> vector<Series>`

### Notes

Minimax supports constraint-aware updates with primal and dual components.

### Inputs

- params
- sensitivities (or `.sensitivity()` with autodiff-traced params)
- optional equality/inequality/bounds constraints

### Output

- updated primal params and dual multipliers in state

### Options

`MinimaxOptions`:

- `adam`
- `dual_learning_rate`
- `equality_penalty`
- `inequality_penalty`
- `project_bounds`
- `enable_constraint_autodiff`
- `nan_recovery_attempts`
- `nan_recovery_shrink`

### Examples

```cpp
pmo::MinimaxOptions opt{};
opt.adam.learning_rate = 0.04;
opt.dual_learning_rate = 0.08;
opt.enable_constraint_autodiff = true;

pmo::MinimaxConstraints c{};
c.equalities = [](const std::vector<Series>& p) { return std::vector<Series>{p[0].sum() - 3.0}; };
c.inequalities = [](const std::vector<Series>& p) { return std::vector<Series>{p[0].square().sum() - 10.0}; };

auto state = pmo::minimax_state_like(params, c);
pmo::minimax_step_inplace(params, grads, state, opt, c);
```

### Expected output

- primal parameters update while multipliers track equality/inequality residuals.

## Derivative-Free Optimization (Unified)

### API Signature

- `DFO::solve(problem: DfoProblem, options: DfoOptions = {}) -> DfoResult`
- `solve_dfo(objective: function, initial: Series, options: DfoOptions = {}) -> DfoResult`
- `DFO::build() -> DfoBuilder`
- `DFO::build(initial: Series) -> DfoBuilder`
- `DfoBuilder::with_* (...) -> DfoBuilder`
- `DfoBuilder::solve() -> DfoResult`

### Notes

Unified DFO facade selects or forces solvers and unifies diagnostics.

### Inputs

- objective and optional constraints/blackbox callbacks
- initial guess and optional bounds

### Output

- `DfoResult` with selected solver, status, minimum, minimizer, diagnostics

### Options

`DfoSolverKind`:

- `Auto`
- `NelderMead`
- `PatternSearch`
- `DifferentialEvolution`
- `DifferentialEvolutionPrmm`
- `ParticleSwarm`
- `ParticleSwarmDv`
- `OrthoMads`
- `LtMads`
- `MadsAdvanced`

### Examples

```cpp
namespace dfo = pluss::subsp::math::optimization;

dfo::DfoProblem p;
p.initial = pluss::full({8}, 2.0, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
p.objective = [](const pluss::Series& x) { return x.square().sum(); };

dfo::DfoOptions opt{};
opt.solver = dfo::DfoSolverKind::Auto;
opt.max_iterations = 500;
opt.max_evaluations = 10000;
opt.rng_seed = 42;

const dfo::DfoResult out = dfo::DFO::solve(p, opt);
```

### Expected output

- returns converged/improved solution with populated diagnostics and solver selection reason.

## Derivative-Free Optimization (Direct Solver Calls)

### API Signature

- `nelder_mead_minimizer(objective, initial, config = {}) -> NelderMeadOutcome`
- `pattern_search_minimizer(objective, initial, config = {}) -> PSOutcome`
- `differential_evolution_minimizer(objective, initial, config = {}) -> PopulationSolverOutcome`
- `differential_evolution_prmm_minimizer(objective, initial, config = {}) -> PopulationSolverOutcome`
- `particle_swarm_minimizer(objective, initial, config = {}) -> PopulationSolverOutcome`
- `particle_swarm_dv_minimizer(objective, initial, config = {}) -> PopulationSolverOutcome`
- `orthomads_minimizer(objective, initial, config = {}) -> BlackboxSolverOutcome`
- `ltmads_minimizer(objective, initial, config = {}) -> BlackboxSolverOutcome`
- `mads_blackbox_minimizer(blackbox, initial, config = {}) -> BlackboxAdvancedOutcome`
- `orthomads_blackbox_minimizer(blackbox, initial, config = {}) -> BlackboxAdvancedOutcome`
- `ltmads_blackbox_minimizer(blackbox, initial, config = {}) -> BlackboxAdvancedOutcome`
- `coopmads_blackbox_minimizer(blackbox, initial, config = {}) -> BlackboxAdvancedOutcome`
- `psdmads_blackbox_minimizer(blackbox, initial, config = {}) -> BlackboxAdvancedOutcome`
- `discomads_blackbox_minimizer(blackbox, initial, config = {}) -> BlackboxAdvancedOutcome`
- `vnsmads_blackbox_minimizer(blackbox, initial, config = {}) -> BlackboxAdvancedOutcome`

### Notes

Use direct calls when you already know the solver family you want.

### Inputs

- solver-specific config structs and callbacks

### Output

- solver-specific outcomes with diagnostics

### Options

- all solver configs are strongly typed structs, no string configs

### Examples

```cpp
dfo::NelderMeadConfig nm{};
nm.max_iterations = 1500;
nm.position_tolerance = 1e-8;
nm.value_tolerance = 1e-8;

const auto out = dfo::nelder_mead_minimizer(
    [](const pluss::Series& x) { return x.square().sum(); },
    x0,
    nm);
```

### Expected output

- solver returns minimizer, scalar minimum, status enum, and iteration/evaluation diagnostics.

## Choosing The Right Solver Quickly

### API Signature

- `presets::*` + `optima(...)`
- `DFO::solve(...)`

### Notes

- smooth constrained NLP: `optima` with `SequentialQP` or `InternalProtocol`
- active-set-heavy constrained problems: `WorkingSet`
- blackbox noisy/non-smooth: DFO (`PatternSearch`, `MADS`, `DE`, `PSO`)
- simple unconstrained local baseline: `NelderMead`

### Inputs

- objective characteristics and constraint type

### Output

- solver family selection

### Options

- start deterministic (`seed`, fixed options), then loosen for global exploration

### Examples

```cpp
// 1) local constrained smooth -> pmo::optima(..., presets::sequential_qp())
// 2) blackbox global -> dfo::DFO::solve(...)
```

### Expected output

- faster solver convergence and more stable tuning workflow.
