<!-- generated from dfo_12_mads_variants.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Derivative-Free Optimization 12: The MADS Variants

Four specialised solvers are built on the black-box MADS engine of Chapter 11. They are available as standalone functions only; the `DFO` facade does not select them. Each addresses one situation: many variables (`coopmads_blackbox_minimizer`, `psdmads_blackbox_minimizer`), a black box that fails often (`discomads_blackbox_minimizer`), and several basins (`vnsmads_blackbox_minimizer`). After this chapter you can tell which one a problem calls for, and when plain MADS is the better choice.

Every program is complete and its output is the output of that program. Counts are evaluations of the objective, and each configuration holds a `base` of type `MadsBlackboxAdvancedConfig` that carries the budgets, bounds, constraint handling and everything else from Chapter 11.

## At a glance

| Solver | Configuration | Idea | Use when |
|---|---|---|---|
| `coopmads_blackbox_minimizer` | `CoopMadsConfig` | optimise blocks of variables one after another | many variables, weakly coupled |
| `psdmads_blackbox_minimizer` | `PsdMadsConfig` | optimise random subsets of random size | many variables, some coupling |
| `discomads_blackbox_minimizer` | `DiscoMadsConfig` | search around failures for valid points | a large fraction of evaluations fail |
| `vnsmads_blackbox_minimizer` | `VnsMadsConfig` | restart from perturbed copies of the best point | several basins |

All four return a `BlackboxAdvancedOutcome`, and divide the evaluation budget of `base` among their internal runs. The two decomposition solvers keep a share (`polish_fraction`, 25% by default) for a last full-space run from the best point: blocks cannot see the coupling between variables in different blocks, and the last run removes the error that coupling leaves.

## Programs

- [Variants for many variables: cooperative and partial-space MADS](dfo_12_mads_variants/decomposition_variants.md)
- [Variants for failing evaluations and several basins](dfo_12_mads_variants/failures_and_several_basins.md)

## Checklist

1. Weakly coupled, many variables: cooperative MADS. Coupled variables: plain MADS or a population method.
2. Frequent failures: Disco, accepting the extra calls.
3. Several basins: variable-neighbourhood MADS, or the search step, or a population method.
4. The budget in `base` is divided among the internal runs; set it for the whole solve.
5. Run a few seeds and keep the best when the function is rugged.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

dfo_12_mads_variants/decomposition_variants
dfo_12_mads_variants/failures_and_several_basins
```
<!-- apiref:toctree:end -->
