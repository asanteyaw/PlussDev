<!-- generated from dfo.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Derivative-free search

Minimisation that uses only objective values: no gradient is written or recorded, so the objective may be non-smooth, discontinuous, noisy or a simulation. One front door, [`solve_dfo`](dfo/solve_dfo.md) / [`DFO`](dfo/DFO.md), chooses a method from the shape of the problem, or you name the method yourself and call it directly.

## Choosing a method

| The problem | Method | Start with |
|---|---|---|
| A few variables, no constraints | Nelder-Mead simplex | [`nelder_mead_minimizer`](dfo/nelder_mead_minimizer.md) |
| Bounds, linear rows or nonlinear constraints | Pattern search | [`pattern_search_minimizer`](dfo/pattern_search_minimizer.md) |
| Many local minima, a box to search | Differential evolution | [`differential_evolution_minimizer`](dfo/differential_evolution_minimizer.md) |
| Many variables | Particle swarm | [`particle_swarm_minimizer`](dfo/particle_swarm_minimizer.md) |
| A simulation that fails, several objectives, an expensive cache-able call | Mesh-adaptive search | [`mads_blackbox_minimizer`](dfo/mads_blackbox_minimizer.md) |
| Hundreds of variables or several basins | Cooperative, decomposition and neighbourhood MADS | [`coopmads_blackbox_minimizer`](dfo/coopmads_blackbox_minimizer.md), [`psdmads_blackbox_minimizer`](dfo/psdmads_blackbox_minimizer.md), [`vnsmads_blackbox_minimizer`](dfo/vnsmads_blackbox_minimizer.md) |

A derivative-free method has no gradient to certify optimality, so every status is a statement about the search; check the minimum, and for constrained problems the violation. Where a gradient is available and the objective is smooth, [Septima](septima.md) or [Optima](optima.md) need far fewer evaluations.

## Where things are

- **The front door:** [`solve_dfo`](dfo/solve_dfo.md), [`DFO`](dfo/DFO.md) and [`DfoBuilder`](dfo/DfoBuilder.md), with [`DfoProblem`](dfo/DfoProblem.md), [`DfoOptions`](dfo/DfoOptions.md), [`DfoResult`](dfo/DfoResult.md), [`DfoSolverKind`](dfo/DfoSolverKind.md) and [`DfoStatus`](dfo/DfoStatus.md).
- **Nelder-Mead:** [`nelder_mead_minimizer`](dfo/nelder_mead_minimizer.md) and [`NelderMeadConfig`](dfo/NelderMeadConfig.md).
- **Pattern search:** [`pattern_search_minimizer`](dfo/pattern_search_minimizer.md) and [`PSConfig`](dfo/PSConfig.md), with its enumerations (`PSPollPattern`, `PSSearchStrategy`, `PSConstraintPolicy` and the rest).
- **Evolutionary and swarm methods:** four minimizers and four configurations, with [`PopulationSolverOutcome`](dfo/PopulationSolverOutcome.md).
- **Mesh-adaptive search:** three scalar and seven black-box minimizers, [`BlackboxEvaluation`](dfo/BlackboxEvaluation.md), and the configurations [`MadsBlackboxConfig`](dfo/MadsBlackboxConfig.md) and [`MadsBlackboxAdvancedConfig`](dfo/MadsBlackboxAdvancedConfig.md).

## The Surface

| Symbol | What it is |
|---|---|
| [`BlackboxAdvancedDiagnostics`](dfo/BlackboxAdvancedDiagnostics.md) | The counters of a black-box mesh-adaptive run. |
| [`BlackboxAdvancedOutcome`](dfo/BlackboxAdvancedOutcome.md) | The result of a black-box mesh-adaptive run. |
| [`BlackboxEvaluation`](dfo/BlackboxEvaluation.md) | What a black-box callback returns: objectives, constraints and two flags. |
| [`BlackboxSolverDiagnostics`](dfo/BlackboxSolverDiagnostics.md) | The counters of a scalar mesh-adaptive run. |
| [`BlackboxSolverDisplay`](dfo/BlackboxSolverDisplay.md) | Whether a mesh-adaptive run prints its progress. |
| [`BlackboxSolverOutcome`](dfo/BlackboxSolverOutcome.md) | The result of a scalar mesh-adaptive run. |
| [`BlackboxSolverStatus`](dfo/BlackboxSolverStatus.md) | Why a mesh-adaptive run stopped. |
| [`ConstraintHandling`](dfo/ConstraintHandling.md) | What a black-box mesh-adaptive search does with constraint values. |
| [`CoopMadsConfig`](dfo/CoopMadsConfig.md) | Settings of cooperative MADS: the base MADS configuration plus the block structure. |
| [`DFO`](dfo/DFO.md) | The derivative-free front door: holds a set of options and solves problems with them. |
| [`DfoAutoPolicy`](dfo/DfoAutoPolicy.md) | The dimension thresholds `Auto` uses to choose among the unconstrained and boxed methods. |
| [`DfoBuilder`](dfo/DfoBuilder.md) | A chainable description of a derivative-free problem and its options, solved with `solve()`. |
| [`DfoConstraintValues`](dfo/DfoConstraintValues.md) | The two kinds of nonlinear constraint values a constraint callback returns. |
| [`DfoDiagnostics`](dfo/DfoDiagnostics.md) | Counters and last-state values of a derivative-free run. |
| [`DfoOptions`](dfo/DfoOptions.md) | Method choice, budgets, tolerance, seed and the per-method settings of a derivative-free run. |
| [`DfoProblem`](dfo/DfoProblem.md) | The data of a derivative-free problem: start, objective or black box, constraints and bounds. |
| [`DfoResult`](dfo/DfoResult.md) | The outcome of a derivative-free run: the minimizer, the minimum, the status and which method ran. |
| [`DfoSolverKind`](dfo/DfoSolverKind.md) | Which derivative-free method the front door runs. |
| [`DfoStatus`](dfo/DfoStatus.md) | Why a derivative-free run stopped. |
| [`DifferentialEvolutionConfig`](dfo/DifferentialEvolutionConfig.md) | Population, mutation, crossover, stopping and sampling settings of differential evolution. |
| [`DifferentialEvolutionPrmmConfig`](dfo/DifferentialEvolutionPrmmConfig.md) | Population, self-adaptation, reduction and budget settings of adaptive differential evolution. |
| [`DiscoMadsConfig`](dfo/DiscoMadsConfig.md) | Settings of Disco MADS: the base configuration plus the rescue around failed evaluations. |
| [`MadsBlackboxAdvancedConfig`](dfo/MadsBlackboxAdvancedConfig.md) | The black-box mesh-adaptive settings: everything in the basic configuration plus constraints, several objectives, cache, surrogate and Pareto archive. |
| [`MadsBlackboxConfig`](dfo/MadsBlackboxConfig.md) | Mesh, frame, poll, search, budget and bound settings of the mesh-adaptive methods. |
| [`MadsPollScheme`](dfo/MadsPollScheme.md) | How the poll directions of a mesh-adaptive search are built. |
| [`MultiObjectiveScalarization`](dfo/MultiObjectiveScalarization.md) | How several objectives are reduced to one value to minimise. |
| [`NelderMeadConfig`](dfo/NelderMeadConfig.md) | Tolerances, budgets and step coefficients of the Nelder-Mead method. |
| [`NelderMeadDiagnostics`](dfo/NelderMeadDiagnostics.md) | The counters of a Nelder-Mead run. |
| [`NelderMeadOutcome`](dfo/NelderMeadOutcome.md) | The result of a Nelder-Mead run: the minimizer, the minimum, the status and the counters. |
| [`PSBasisFamily`](dfo/PSBasisFamily.md) | The set of directions the poll is built on. |
| [`PSConfig`](dfo/PSConfig.md) | Mesh, poll, search, constraint, cache, bound and budget settings of pattern search. |
| [`PSConstraintPolicy`](dfo/PSConstraintPolicy.md) | How nonlinear constraint violations are handled. |
| [`PSConstraintValues`](dfo/PSConstraintValues.md) | The nonlinear constraint values a pattern-search constraint callback returns. |
| [`PSDiagnostics`](dfo/PSDiagnostics.md) | The counters and last-state values of a pattern-search run. |
| [`PSDirectionFamily`](dfo/PSDirectionFamily.md) | How poll directions are adapted to active constraints. |
| [`PSDisplay`](dfo/PSDisplay.md) | Whether a pattern-search run prints its progress. |
| [`PSMeshScaling`](dfo/PSMeshScaling.md) | How the mesh is scaled per variable. |
| [`PSOutcome`](dfo/PSOutcome.md) | The result of a pattern-search run. |
| [`PSPollPattern`](dfo/PSPollPattern.md) | Which set of directions is polled. |
| [`PSPollingOrder`](dfo/PSPollingOrder.md) | The order the poll points are tried in. |
| [`PSSearchStrategy`](dfo/PSSearchStrategy.md) | The extra search step tried before each poll. |
| [`PSStatus`](dfo/PSStatus.md) | Why a pattern-search run stopped. |
| [`ParticleSwarmConfig`](dfo/ParticleSwarmConfig.md) | Swarm size, inertia and acceleration schedules, velocity clamp and stopping settings of the particle swarm. |
| [`ParticleSwarmDvConfig`](dfo/ParticleSwarmDvConfig.md) | Swarm size, differential and social scales, crossover and stagnation settings of the differential-velocity swarm. |
| [`PopulationSolverDiagnostics`](dfo/PopulationSolverDiagnostics.md) | The counters of a differential-evolution or swarm run. |
| [`PopulationSolverDisplay`](dfo/PopulationSolverDisplay.md) | Whether a differential-evolution or swarm run prints its progress. |
| [`PopulationSolverExecutionMode`](dfo/PopulationSolverExecutionMode.md) | How a generation of a population method is computed. |
| [`PopulationSolverOutcome`](dfo/PopulationSolverOutcome.md) | The result of a differential-evolution or swarm run. |
| [`PopulationSolverStatus`](dfo/PopulationSolverStatus.md) | Why a differential-evolution or swarm run stopped. |
| [`PsdMadsConfig`](dfo/PsdMadsConfig.md) | Settings of partial-space decomposition MADS: the base configuration plus the random subsets. |
| [`SolverDisplay`](dfo/SolverDisplay.md) | Whether a Nelder-Mead run prints its progress. |
| [`SolverStatus`](dfo/SolverStatus.md) | Why a Nelder-Mead run stopped. |
| [`SurrogateMode`](dfo/SurrogateMode.md) | Whether a cheap model orders the poll candidates of a mesh-adaptive search. |
| [`VnsMadsConfig`](dfo/VnsMadsConfig.md) | Settings of variable-neighbourhood MADS: the base configuration plus the kicks. |
| [`coopmads_blackbox_minimizer`](dfo/coopmads_blackbox_minimizer.md) | Minimises a black box in many variables with cooperative MADS, which optimises blocks of variables in turn. |
| [`differential_evolution_minimizer`](dfo/differential_evolution_minimizer.md) | Minimises an objective with differential evolution, a population method that needs no gradient. |
| [`differential_evolution_prmm_minimizer`](dfo/differential_evolution_prmm_minimizer.md) | Minimises an objective with differential evolution that adapts its own parameters and shrinks its population. |
| [`discomads_blackbox_minimizer`](dfo/discomads_blackbox_minimizer.md) | Minimises a black box that fails at many points with Disco MADS, which looks for valid points around each failure. |
| [`ltmads_blackbox_minimizer`](dfo/ltmads_blackbox_minimizer.md) | Minimises a black box with LT-MADS, mesh adaptive direct search with random lower-triangular poll directions. |
| [`ltmads_minimizer`](dfo/ltmads_minimizer.md) | Minimises an objective with LT-MADS, mesh adaptive direct search with random lower-triangular poll directions. |
| [`mads_blackbox_minimizer`](dfo/mads_blackbox_minimizer.md) | Minimises a black box with mesh adaptive direct search, with constraints, several objectives, failed evaluations, a cache and a surrogate. |
| [`mesh_adaptive_direct_search_minimizer`](dfo/mesh_adaptive_direct_search_minimizer.md) | Minimises an objective with mesh adaptive direct search. |
| [`nelder_mead_minimizer`](dfo/nelder_mead_minimizer.md) | Minimises an objective with the Nelder-Mead simplex method. |
| [`orthomads_blackbox_minimizer`](dfo/orthomads_blackbox_minimizer.md) | Minimises a black box with OrthoMADS, mesh adaptive direct search with orthogonal poll directions. |
| [`orthomads_minimizer`](dfo/orthomads_minimizer.md) | Minimises an objective with OrthoMADS, mesh adaptive direct search with orthogonal poll directions. |
| [`particle_swarm_dv_minimizer`](dfo/particle_swarm_dv_minimizer.md) | Minimises an objective with a swarm whose velocity is built from differences between particles. |
| [`particle_swarm_minimizer`](dfo/particle_swarm_minimizer.md) | Minimises an objective with a particle swarm: points that move with inertia toward their own best and the swarm's best. |
| [`pattern_search_minimizer`](dfo/pattern_search_minimizer.md) | Minimises an objective by polling a pattern of directions on an adaptive mesh, with bounds, linear rows and nonlinear constraints. |
| [`psdmads_blackbox_minimizer`](dfo/psdmads_blackbox_minimizer.md) | Minimises a black box in many variables with partial-space decomposition MADS, which optimises random subsets of the variables. |
| [`solve_dfo`](dfo/solve_dfo.md) | Minimises an objective from a starting point, choosing the method automatically. |
| [`to_string`](dfo/to_string.md) | Returns the name of a solver kind or a status as text. |
| [`to_string_BlackboxSolverStatus`](dfo/to_string_BlackboxSolverStatus.md) | Returns the name of a mesh-adaptive status. |
| [`to_string_PSStatus`](dfo/to_string_PSStatus.md) | Returns the name of a pattern-search status. |
| [`to_string_PopulationSolverStatus`](dfo/to_string_PopulationSolverStatus.md) | Returns the name of a population-method status. |
| [`to_string_SolverStatus`](dfo/to_string_SolverStatus.md) | Returns the name of a Nelder-Mead status. |
| [`vnsmads_blackbox_minimizer`](dfo/vnsmads_blackbox_minimizer.md) | Minimises a black box with several basins with variable-neighbourhood MADS, which kicks the best point and searches again. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

dfo/BlackboxAdvancedDiagnostics
dfo/BlackboxAdvancedOutcome
dfo/BlackboxEvaluation
dfo/BlackboxSolverDiagnostics
dfo/BlackboxSolverDisplay
dfo/BlackboxSolverOutcome
dfo/BlackboxSolverStatus
dfo/ConstraintHandling
dfo/CoopMadsConfig
dfo/DFO
dfo/DfoAutoPolicy
dfo/DfoBuilder
dfo/DfoConstraintValues
dfo/DfoDiagnostics
dfo/DfoOptions
dfo/DfoProblem
dfo/DfoResult
dfo/DfoSolverKind
dfo/DfoStatus
dfo/DifferentialEvolutionConfig
dfo/DifferentialEvolutionPrmmConfig
dfo/DiscoMadsConfig
dfo/MadsBlackboxAdvancedConfig
dfo/MadsBlackboxConfig
dfo/MadsPollScheme
dfo/MultiObjectiveScalarization
dfo/NelderMeadConfig
dfo/NelderMeadDiagnostics
dfo/NelderMeadOutcome
dfo/PSBasisFamily
dfo/PSConfig
dfo/PSConstraintPolicy
dfo/PSConstraintValues
dfo/PSDiagnostics
dfo/PSDirectionFamily
dfo/PSDisplay
dfo/PSMeshScaling
dfo/PSOutcome
dfo/PSPollPattern
dfo/PSPollingOrder
dfo/PSSearchStrategy
dfo/PSStatus
dfo/ParticleSwarmConfig
dfo/ParticleSwarmDvConfig
dfo/PopulationSolverDiagnostics
dfo/PopulationSolverDisplay
dfo/PopulationSolverExecutionMode
dfo/PopulationSolverOutcome
dfo/PopulationSolverStatus
dfo/PsdMadsConfig
dfo/SolverDisplay
dfo/SolverStatus
dfo/SurrogateMode
dfo/VnsMadsConfig
dfo/coopmads_blackbox_minimizer
dfo/differential_evolution_minimizer
dfo/differential_evolution_prmm_minimizer
dfo/discomads_blackbox_minimizer
dfo/ltmads_blackbox_minimizer
dfo/ltmads_minimizer
dfo/mads_blackbox_minimizer
dfo/mesh_adaptive_direct_search_minimizer
dfo/nelder_mead_minimizer
dfo/orthomads_blackbox_minimizer
dfo/orthomads_minimizer
dfo/particle_swarm_dv_minimizer
dfo/particle_swarm_minimizer
dfo/pattern_search_minimizer
dfo/psdmads_blackbox_minimizer
dfo/solve_dfo
dfo/to_string
dfo/to_string_BlackboxSolverStatus
dfo/to_string_PSStatus
dfo/to_string_PopulationSolverStatus
dfo/to_string_SolverStatus
dfo/vnsmads_blackbox_minimizer
```
<!-- apiref:toctree:end -->
