<!-- generated from dfo_10_mesh_adaptive_search.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Derivative-Free Optimization 10: Mesh Adaptive Direct Search

Mesh adaptive direct search (MADS) is a pattern search whose poll directions keep changing, so that over a run they point in every direction rather than only along the axes. It handles rough, non-smooth and discontinuous functions, and its stopping rule is tied to a quantity you can reason about: the size of the poll. This chapter covers the three scalar entry points. Chapter 11 covers the black-box interface with failures, constraints and several objectives, and Chapter 12 the specialised variants.

Every program is complete and its output is the output of that program. Counts are evaluations of the objective.

## The method

MADS keeps a **frame size** (the poll radius) and a finer **mesh size** (the grid the poll points lie on, never larger than the frame). Each iteration:

1. **Search (optional).** Try `search_points` extra points over the frame; if one improves, accept it.
2. **Poll.** Evaluate 2n points around the best point: a set of directions that span all of space, scaled to the frame and snapped to the mesh.
3. On success, enlarge the frame (× `frame_expand_factor`) and mesh (× `mesh_expand_factor`). On failure, shrink them (× `frame_refine_factor`, × `mesh_refine_factor`). The mesh shrinks faster than the frame, which is what makes the set of poll directions dense as the run converges.

The run converges when the frame falls below `mesh_tolerance` and, if the window test is on, the best value has stopped improving.

The directions differ between the two entry points. **OrthoMADS** (`orthomads_minimizer`) takes an orthogonal basis from a Halton-sequence reflection, a different one each iteration. **LT-MADS** (`ltmads_minimizer`) draws a random lower-triangular basis. `mesh_adaptive_direct_search_minimizer` reads the choice from `poll_scheme` (`Ortho2N` by default, or `LtMadsRandom2N`).

## The calls

```cpp
BlackboxSolverOutcome mesh_adaptive_direct_search_minimizer(objective, initial, MadsBlackboxConfig config = {});
BlackboxSolverOutcome orthomads_minimizer(objective, initial, MadsBlackboxConfig config = {});
BlackboxSolverOutcome ltmads_minimizer(objective, initial, MadsBlackboxConfig config = {});
```

| `MadsBlackboxConfig` | Meaning | Default |
|---|---|---|
| `max_iterations`, `max_evaluations` | budgets; 0 means 250 and 500 times the number of variables | 0, 0 |
| `initial_frame_size`, `initial_mesh_size` | starting scales; 0 means chosen from the start point | 0, 0 |
| `frame_refine_factor`, `mesh_refine_factor` | applied after a failed iteration | 0.5, 0.25 |
| `frame_expand_factor`, `mesh_expand_factor` | applied after a success | 2, 4 |
| `mesh_tolerance` | converged when the frame is below this | 1e-8 |
| `check_every`, `relative_improvement_tolerance` | improvement-window test; `check_every` 0 turns it off | 10, 1e-8 |
| `min_improvement` | a trial must beat the best by this to count (and by a few rounding units whatever this is) | 0 |
| `opportunistic` | stop a poll at the first improvement | true |
| `search_points` | extra search points per iteration | 0 |
| `poll_scheme` | `Ortho2N` or `LtMadsRandom2N` | `Ortho2N` |
| `lower_bounds`, `upper_bounds` | optional box | none |
| `rng_seed`, `promote_cpu_to_float64`, `display` | as before | 5489, true, `None` |

`BlackboxSolverOutcome` has `minimizer`, `minimum`, `status` and `diagnostics`: `iterations`, `evaluations`, `last_phase`, `mesh_size`, `frame_size`.

## Programs

- [Mesh adaptive direct search in practice](dfo_10_mesh_adaptive_search/mads_in_practice.md)

## Checklist

1. Start with `orthomads_minimizer`; it needs no tuning.
2. Set `max_evaluations` for the accuracy you want; the default is small.
3. Read the final `frame_size` in the diagnostics to see whether a run finished or was cut off.
4. Use `search_points` when the function may have several basins.
5. Give bounds when you know them.
6. For failures, constraints or several objectives use the black-box interface (next chapter).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

dfo_10_mesh_adaptive_search/mads_in_practice
```
<!-- apiref:toctree:end -->
