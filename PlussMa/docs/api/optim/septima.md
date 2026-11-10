<!-- generated from septima.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# First-order and quasi-Newton optimization (Septima)

Gradient-based minimisation of a smooth objective with Adam, limited-memory BFGS or limited-memory SR1 with a trust region. Objectives are ordinary Series arithmetic, differentiated automatically; Adam and L-BFGS also accept bounds, equalities and inequalities through an augmented Lagrangian.

## The Surface

| Symbol | What it is |
|---|---|
| [`AdamOptions`](septima/AdamOptions.md) | Settings of the Adam optimizer: learning rate, moment decay, regularisation and variants. |
| [`AdamState`](septima/AdamState.md) | The running state of Adam: the moment estimates and the step count. |
| [`LbfgsOptions`](septima/LbfgsOptions.md) | Settings of the L-BFGS optimizer: step scale, memory, curvature test and line search. |
| [`Lsr1ExitFlag`](septima/Lsr1ExitFlag.md) | Why a run of `l_sr1_trust_region` stopped. |
| [`Lsr1Options`](septima/Lsr1Options.md) | The L-SR1 method's own settings: memory, trust-region radius and its update rules, the SR1 skip test and the line search. |
| [`Lsr1ProgressSnapshot`](septima/Lsr1ProgressSnapshot.md) | What an L-SR1 progress hook receives each iteration. |
| [`Lsr1Result`](septima/Lsr1Result.md) | What `l_sr1_trust_region` returns: the flat solution, the objective, the gradient norm and the exit flag. |
| [`Lsr1SolveOptions`](septima/Lsr1SolveOptions.md) | Budgets, tolerances, the L-SR1 settings and the progress hook of an `l_sr1_trust_region` run. |
| [`Lsr1State`](septima/Lsr1State.md) | The memory of the L-SR1 method at the end of a run: stored pairs, the scalar gamma and the trust radius. |
| [`SeptimaConstraints`](septima/SeptimaConstraints.md) | The constraints of a `septima` run: residual functions for equalities and inequalities, and bounds. |
| [`SeptimaExitFlag`](septima/SeptimaExitFlag.md) | Why a run of `septima` stopped. |
| [`SeptimaLbfgsLineSearch`](septima/SeptimaLbfgsLineSearch.md) | Whether L-BFGS steps are shortened by a line search. |
| [`SeptimaOptimizer`](septima/SeptimaOptimizer.md) | The two optimizers of `septima`: Adam and limited-memory BFGS. |
| [`SeptimaOptions`](septima/SeptimaOptions.md) | Chooses the optimizer of `septima` and holds the settings of Adam, L-BFGS and the constraint penalty schedule. |
| [`SeptimaProgressSnapshot`](septima/SeptimaProgressSnapshot.md) | What a `septima` progress hook receives each iteration. |
| [`SeptimaResult`](septima/SeptimaResult.md) | What `septima` returns: the answer blocks, the objective, the stopping measures and the exit flag. |
| [`SeptimaSolveOptions`](septima/SeptimaSolveOptions.md) | Budgets, stopping tolerances, the optimizer settings and the progress hook of a `septima` run. |
| [`SeptimaState`](septima/SeptimaState.md) | The optimizer's memory at the end of a run: Adam moments, L-BFGS history, multipliers and penalty bookkeeping. |
| [`adam_state_like`](septima/adam_state_like.md) | Creates a zeroed `AdamState` for a list of parameters. |
| [`adam_step`](septima/adam_step.md) | Applies one Adam update and returns the new parameters, leaving the inputs unchanged. |
| [`adam_step_inplace`](septima/adam_step_inplace.md) | Applies one Adam update to a list of parameters, in place, given their gradients. |
| [`l_sr1_trust_region`](septima/l_sr1_trust_region.md) | Minimises a smooth objective by limited-memory SR1 with a trust region, which can follow negative curvature. |
| [`lsr1_exit_flag_name`](septima/lsr1_exit_flag_name.md) | The name of an `Lsr1ExitFlag` as text. |
| [`septima`](septima/septima.md) | Minimises a smooth objective with Adam or limited-memory BFGS, optionally under bounds, equalities and inequalities. |
| [`septima_exit_flag_name`](septima/septima_exit_flag_name.md) | The name of a `SeptimaExitFlag` as text. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

septima/AdamOptions
septima/AdamState
septima/LbfgsOptions
septima/Lsr1ExitFlag
septima/Lsr1Options
septima/Lsr1ProgressSnapshot
septima/Lsr1Result
septima/Lsr1SolveOptions
septima/Lsr1State
septima/SeptimaConstraints
septima/SeptimaExitFlag
septima/SeptimaLbfgsLineSearch
septima/SeptimaOptimizer
septima/SeptimaOptions
septima/SeptimaProgressSnapshot
septima/SeptimaResult
septima/SeptimaSolveOptions
septima/SeptimaState
septima/adam_state_like
septima/adam_step
septima/adam_step_inplace
septima/l_sr1_trust_region
septima/lsr1_exit_flag_name
septima/septima
septima/septima_exit_flag_name
```
<!-- apiref:toctree:end -->
