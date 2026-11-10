<!-- generated from Introduction.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::DfoBuilder` Introduction

`DfoBuilder` collects a [`DfoProblem`](../DfoProblem.md) and a [`DfoOptions`](../DfoOptions.md) through chained calls. Every `with_*` method returns a **new** builder with that one setting changed and leaves the original untouched, so a builder is a value: keep a base and branch it. The problem parts are the starting point ([`with_initial`](with_initial.md)), the objective ([`with_objective`](with_objective.md)) or a black box ([`with_blackbox`](with_blackbox.md)), constraints ([`with_constraints`](with_constraints.md), [`with_constraints_values`](with_constraints_values.md)) and bounds ([`with_bounds`](with_bounds.md)). The option parts are the method ([`with_solver`](with_solver.md)), the budgets ([`with_max_iterations`](with_max_iterations.md), [`with_max_evaluations`](with_max_evaluations.md), [`with_max_total_evaluations`](with_max_total_evaluations.md)), the stopping controls ([`with_tolerance`](with_tolerance.md), [`with_check_every`](with_check_every.md)), the seed ([`with_seed`](with_seed.md)) and the per-method configurations (`with_nelder_mead_config`, `with_de_config`, `with_de_prmm_config`, `with_pso_config`, `with_pso_dv_config`, `with_mads_config`). [`solve`](solve.md) runs it. [`problem`](problem.md) and [`options`](options.md) read back what has been collected.

Callables passed to the objective, constraint and black-box setters may return a `Series` or an ordinary number; a number is wrapped as a scalar `Series`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="options.html">
    <span class="label">Next</span>
    <span class="title">options</span>
  </a>
</div>
<!-- apiref:nav:end -->
