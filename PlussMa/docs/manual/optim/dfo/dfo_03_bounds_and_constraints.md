<!-- generated from dfo_03_bounds_and_constraints.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Derivative-Free Optimization 3: Bounds and Constraints

Real problems have limits: a dose cannot be negative, a design must carry a load, a budget must balance. This chapter covers the three ways to state limits: bounds, inequality constraints, and equality constraints. After it you can state each, know which solvers accept which, tell a feasible answer from an infeasible one, and see how the mesh methods approach a boundary.

Every program is complete and its output is the output of that program.

## Three kinds of limit

| Kind | Written as | How you state it |
|---|---|---|
| bounds | `lower <= x <= upper` | `lower_bounds`, `upper_bounds` (Series, same length as `initial`) |
| inequality | `g(x) <= 0`, every entry | `constraints` returning a vector, or `constraints_values().inequality` |
| equality | `h(x) = 0`, every entry | `constraints_values().equality` |

The sign convention matters: a constraint is satisfied when its value is **zero or negative**. To require `x0 + x1 >= 1`, return `1 - x0 - x1`.

| Solver | Bounds | Inequalities | Equalities |
|---|---|---|---|
| `NelderMead` | yes (by projection) | no (rejected with an error) | no |
| `PatternSearch` | yes | yes | yes |
| `OrthoMads`, `LtMads`, `MadsAdvanced` | yes | yes | no (rejected with an error) |
| `DifferentialEvolution*`, `ParticleSwarm*` | yes | no | no |

`Auto` routes any problem with constraints to `PatternSearch`, which is the one method that handles all three kinds.

## Programs

- [Bounds only](dfo_03_bounds_and_constraints/bounds_only.md)
- [Inequality constraints](dfo_03_bounds_and_constraints/inequality_constraints.md)
- [Equality constraints](dfo_03_bounds_and_constraints/equality_constraints.md)
- [Mesh methods with constraints](dfo_03_bounds_and_constraints/mesh_methods_with_constraints.md)
- [No feasible point](dfo_03_bounds_and_constraints/no_feasible_point.md)

## Checklist

1. Write every constraint so that feasible means `<= 0` (inequality) or `= 0` (equality).
2. Use bounds for simple limits on single variables; they are cheaper and tighter than the same limit written as a constraint.
3. Equalities: `PatternSearch` only. If you can eliminate the equality by substitution, do it.
4. After the solve, read `max_constraint_violation`. Treat a violation above your tolerance as a failed solve whatever the status.
5. Expect the last digits on a boundary to be slow; budget for them or accept a looser answer.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

dfo_03_bounds_and_constraints/bounds_only
dfo_03_bounds_and_constraints/inequality_constraints
dfo_03_bounds_and_constraints/equality_constraints
dfo_03_bounds_and_constraints/mesh_methods_with_constraints
dfo_03_bounds_and_constraints/no_feasible_point
```
<!-- apiref:toctree:end -->
