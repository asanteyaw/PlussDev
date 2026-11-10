<!-- generated from milp.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Mixed-integer linear programming

Branch-and-bound on the batched linear-programming solver: the open nodes of the search tree are lanes of one batched call, so each round solves many relaxations together.

## The Surface

| Symbol | What it is |
|---|---|
| [`Options`](milp/Options.md) | Node limit, batch size and tolerances of the branch-and-bound search. |
| [`Problem`](milp/Problem.md) | A mixed-integer linear program: a linear program plus the flags of which variables are integer. |
| [`Result`](milp/Result.md) | What `milp::solve` returns: the best integer solution, its objective, the proven bound and search counters. |
| [`Status`](milp/Status.md) | The verdict of a mixed-integer solve. |
| [`solve`](milp/solve.md) | Solves a mixed-integer linear program by branch-and-bound and returns the best solution found with a proven bound. |
| [`to_string`](milp/to_string.md) | The name of a mixed-integer `Status` as text. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

milp/Options
milp/Problem
milp/Result
milp/Status
milp/solve
milp/to_string
```
<!-- apiref:toctree:end -->
