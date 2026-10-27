<!-- generated from dfo_06_nelder_mead.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Derivative-Free Optimization 6: The Nelder–Mead Minimiser

The earlier chapters used Nelder–Mead through the `DFO` facade. This chapter calls it directly with `nelder_mead_minimizer`, which exposes every setting of the method. After it you can set the starting simplex, the step coefficients and the stopping tests, mark regions as forbidden, and search over matrices and other shapes.

Every program is complete and its output is the output of that program. Counts are evaluations of the objective.

## The method

A simplex is a set of n+1 points in n variables: a segment for one variable, a triangle for two, a tetrahedron for three. Each iteration compares the objective at the points and replaces the worst one:

1. **Reflect** the worst point through the centre of the others.
2. If the reflected point is the new best, try to **expand** further in that direction and keep the better of the two.
3. If it beats the second-worst point, keep it.
4. Otherwise **contract**, towards the centre, from outside (if the reflected point was better than the worst) or from inside (if not).
5. If the contraction fails, **shrink** every point towards the best.

The method stops when all vertices lie within `position_tolerance` of the best **and** their values are within `value_tolerance` of its value.

## The call

```cpp
NelderMeadOutcome nelder_mead_minimizer(objective, initial, NelderMeadConfig config = {});
```

| `NelderMeadConfig` field | Meaning | Default |
|---|---|---|
| `position_tolerance`, `value_tolerance` | stopping tolerances (both must hold) | 1e-4, 1e-4 |
| `max_iterations`, `max_evaluations` | budgets; 0 means 200 times the number of variables | 0, 0 |
| `reflection`, `expansion`, `contraction`, `shrink` | step coefficients | 1, 2, 0.5, 0.5 |
| `simplex_delta` | each coordinate of the start is scaled by 1 + this to form the other vertices | 0.05 |
| `simplex_delta_zero` | the step used for a coordinate that is exactly zero | 0.00025 |
| `promote_cpu_to_float64` | on the CPU, search in Float64 whatever the dtype of `initial` | true |
| `display` | `SolverDisplay::Iter` prints one line per iteration | `None` |

| `NelderMeadOutcome` field | Meaning |
|---|---|
| `minimizer` | best point, with the shape and dtype of `initial` |
| `minimum` | its objective value |
| `status` | `Converged`, `ReachedIterationBudget`, `ReachedEvaluationBudget` |
| `diagnostics` | `iterations`, `evaluations`, `last_step` (`expand`, `reflect`, `adjust_outer`, `adjust_inner`, `recenter`) |

The facade's `DfoOptions::nelder_mead` holds the same `NelderMeadConfig`, so every setting here is also available through `DFO`.

## Programs

- [Nelder–Mead in practice](dfo_06_nelder_mead/nelder_mead_in_practice.md)

## Checklist

1. Scale the starting simplex to the problem (`simplex_delta`); the default assumes variables of order one.
2. Raise or lower the tolerances to match the accuracy you need; both must hold.
3. Keep the default coefficients unless you have measured a reason to change them.
4. Use `+infinity`, not a large number, for forbidden points.
5. Beyond about ten variables, or on noisy objectives, prefer a population or mesh method (Chapter 2).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

dfo_06_nelder_mead/nelder_mead_in_practice
```
<!-- apiref:toctree:end -->
