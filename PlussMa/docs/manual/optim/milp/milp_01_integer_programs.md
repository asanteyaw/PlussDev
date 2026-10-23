<!-- generated from milp_01_integer_programs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Integer Programming 1: Choosing Whole Things

Some decisions come in whole units: a project is taken or not, a machine is bought or not, a truck carries a whole number of pallets. An integer program is a linear program in which some of the variables must take whole-number values. The extra requirement is what makes these problems hard, and what makes them useful: nearly every real plan with an on/off choice is one. After this chapter you can state an integer program, solve it by branch and bound, check a small one by brute force, read the status and the gap, and use the node limit when a search is too long to finish.

Every program is complete and its output is the output of that program.

## The mental model

An integer program is a linear program with a list of variables that must be integer:

```text
minimize    c . x
subject to  a_eq x  = b_eq,   a_ub x <= b_ub,   lower <= x <= upper
            x_j integer for every j marked in `integer`
```

`milp::Problem` holds a `linprog::Problem` in its member `lp` and a `Series` called `integer`, with one entry per variable, nonzero where the variable must be integer. Every integer variable needs **finite** lower and upper bounds. A 0/1 decision is an integer variable with bounds 0 and 1.

The method is **branch and bound**. Solve the linear program with the integrality dropped, the *relaxation*; its optimum is a bound that no integer solution can beat. If the relaxation's answer happens to be integer, it is the answer. Otherwise pick an integer variable with a fractional value, say `x = 2.4`, and split into two smaller problems, one with `x <= 2` and one with `x >= 3`. Repeat on the pieces, discarding any piece whose bound is no better than the best integer solution already found. Each piece is a *node*. The search stops when the best solution found and the best bound still open are within the gap tolerance.

| Result field | Meaning |
|---|---|
| `x` | the best integer solution found (integer variables are rounded to exact integers) |
| `objective` | its objective value |
| `best_bound` | the best bound no integer solution can beat |
| `status` | `Optimal`, `Infeasible`, `Unbounded` or `NodeLimit` |
| `nodes`, `lp_rounds` | the work done: nodes explored, batched relaxations solved |

| Option | Meaning | Default |
|---|---|---|
| `max_nodes` | stop after this many nodes | 200000 |
| `batch` | open nodes solved together in each round | 64 |
| `integer_tolerance` | how close to a whole number counts as whole | 1e-6 |
| `gap_tolerance` | stop when the bound and the best solution agree to this (absolute and relative) | 1e-7 |
| `lp` | the options of the linear programs at the nodes | |

## Programs

- [Choosing projects under a budget](milp_01_integer_programs/choosing_projects_under_a_budget.md)
- [Integers that are not 0 or 1, and when the relaxation is enough](milp_01_integer_programs/integers_that_are_not_0_or_1_and_when_the_relaxa.md)
- [How a search ends](milp_01_integer_programs/how_a_search_ends.md)

## What to remember

- An integer program is a linear program plus a list of integer variables, which need finite bounds. A yes/no decision is an integer in [0, 1].
- Branch and bound solves the relaxation, splits on a fractional variable, and prunes by the best solution found. It reports the best solution and the best bound.
- The relaxation is a bound. If its answer is whole, you are done; if not, the gap shows how much integrality costs.
- Test a formulation on a small instance by brute force before trusting it on a large one.
- `NodeLimit` still returns the best solution and a proven bound. The difference is the gap, and it is the measure of how good the answer is.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

milp_01_integer_programs/choosing_projects_under_a_budget
milp_01_integer_programs/integers_that_are_not_0_or_1_and_when_the_relaxa
milp_01_integer_programs/how_a_search_ends
```
<!-- apiref:toctree:end -->
