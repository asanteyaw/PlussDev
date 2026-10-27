<!-- generated from linprog_03_methods_warm_starts_and_sweeps.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Linear Programming 3: Methods, Warm Starts and Sweeps

Chapter 1 introduced the two methods and chapter 2 the prices and certificates. This chapter is about using them well: what each method returns when many plans are equally good, how to restart the simplex method from a previous answer when the data change a little, and how to solve many related programs in one call. After it you can choose a method for the answer you want, reuse a solution when you re-solve, and run a parameter sweep as a single batched solve.

Every program is complete and its output is the output of that program.

## The mental model

Real work with linear programs is rarely one solve. You change a price and ask again, or you trace a trade-off by sweeping a weight, or you check a hundred scenarios. The library supports that in two ways.

- **A warm start** reuses the optimal *basis* of one solve (the set of constraints that bind at the corner) as the starting point of the next. The simplex method then needs only the pivots that the change makes necessary, which for a small change is few or none. A `Result` from the simplex method carries `basis` and `at_upper` for every lane, and a `Problem` accepts them back as `warm_basis` and `warm_at_upper`.
- **A batch** puts many programs in one call. If `c` has a leading axis, `[B, n]`, each row is a separate objective, and the constraint matrix is shared. The right-hand sides and the bound values may carry the leading axis too. The call returns a `Series` of statuses, objectives and points, one per lane.

The two methods also differ in which optimal point they return when the optimum is not unique, which matters when the point itself is the answer.

| Setting | Meaning | Default |
|---|---|---|
| `method` | `InteriorPoint` or `Simplex` | `InteriorPoint` |
| `max_iterations` | the budget per solve | 150 |
| `tolerance` | relative primal, dual and gap tolerance (interior point) | 1e-9 |
| `accept_tolerance` | a stalled solve with residuals below this is `NearOptimal` | 1e-6 |
| `max_lanes` | solve at most this many lanes per batched call (bounds the workspace); any answer is unchanged | 0 (all at once) |

## Programs

- [Methods, warm starts and sweeps](linprog_03_methods_warm_starts_and_sweeps/methods_warm_starts_and_sweeps.md)

## What to remember

- When the optimum is not unique, the simplex method returns a corner and the interior point method a central point. Choose the one that suits how the answer will be used.
- A warm start passes the optimal `basis` and `at_upper` of one simplex solve to the next. A moved right-hand side often needs no pivots; a moved price needs a few. Check `warm_started`.
- A leading axis on `c` (and optionally on the right-hand sides and bounds) turns one call into a batch. `max_lanes` bounds the memory and never changes the answers.
- A sweep of a weight is a way to find the breakpoints at which the best plan changes.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

linprog_03_methods_warm_starts_and_sweeps/methods_warm_starts_and_sweeps
```
<!-- apiref:toctree:end -->
