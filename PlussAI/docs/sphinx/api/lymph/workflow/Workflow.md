# `lymph::workflow::Workflow`

`Workflow` is a [`Model`](../models/Model.md) plus a [`Policy`](Policy.md) plus
an optional branch/route DSL for building that `Model`'s prep pipeline
column-by-column — the object you reach for the moment raw data needs an
admission contract or per-column routing before it can reach a model.

> See the [workflow family](../workflow.md) for what this concern owns, and
> [Introduction](Workflow/Introduction.md) for how the pieces fit together.

## Scope And Alias

`Workflow` is `lymph::workflow::Workflow`. There is no shorter family-root
alias the way `models::Model` also has a bare `lymph::` re-export — write the
full `workflow::` qualification, or bring in `namespace workflow =
pluss::ai::lymph::workflow;` the way the examples on this page do.

## Construction

```cpp
lymph::workflow::Workflow lymph::workflow::make(lymph::workflow::Policy policy);
lymph::workflow::Workflow lymph::workflow::make(lymph::models::Model model,
                                                lymph::workflow::Policy policy = {});
// plus the branch-spec DSL overloads and restore()/load() -- see make.md
```

A `Workflow` always holds a `Model` — `make(policy)` alone starts from an
empty, untrained `Model` template; `make(model, policy)` starts from a `Model`
you already built (a factory call, optionally with `prepare()`/`append(...)`
stages already attached). Either way, `Workflow::on(...)` /
`Workflow::branch(...)` can still attach more prep stages afterward, routed
through the branch DSL instead of `Model::append`. See [`make`](make.md) for
every construction path, including reloading a saved `Workflow` from disk.

## State

`bool trained() const` · `stage_count()` · `task_kind()` · `model_spec()` ·
`transform_specs()` · `graph_info()` · `nested_param_keys()` /
`nested_param(path)` · `flow_summary()` · `save_blueprint()`
: Mirror the identically-named [`Model`](../models/Model.md) accessors —
  `Workflow` reads these straight off the `Model` it holds.

`const std::vector<WorkflowBranchInfo>& branches() const` · `branch_count()`
: One entry per branch opened through `on(...)` / `branch(...)`, each holding
  the branch's name (if named), whether it was named, and how many stages have
  been routed into it. Empty on a `Workflow` that never used the branch DSL.

`bool joined() const`
: Set by [`join()`](#configuration) — informational only; nothing in
  `Workflow` currently branches its own behavior on it.

`const WorkflowOptions& options() const` · `std::int64_t workers() const`
: `WorkflowOptions{workers, schedule, cache}` — `workers()` is a shortcut for
  `options().workers`.

`bool time_aware() const` · `const std::optional<TimeOptions>& time_options() const`
: Whether [`time(...)`](#configuration) has been called — read by
  [`selection::validate::time`](../selection/validate/time.md) to fill in
  unset chronological-split window fields.

`const Model& model() const` / `Model& model()`
: The held estimator — reach in when you need a `Model`-only method
  `Workflow` does not forward (`prepare()`, `append(...)`).

`const Policy& policy() const`
: The admission contract every raw-frame verb applies. See
  [`Policy`](Policy.md).

## Configuration

These are small, chainable setters — folded in here rather than given their
own method pages:

`Workflow& workers(std::int64_t value)`
: Sets `options().workers`. Throws if `value < 1`.

`Workflow& join()`
: Marks the `Workflow` joined (see [`joined()`](#state) above).

`Workflow& time(TimeOptions options)`
: Opts the `Workflow` into the time-aware contract. Throws via
  `validate_time_options` on a negative `horizon` / `gap` / `lookback`, or an
  empty `index` when `reject_missing_index` is left at its default `true`. See
  [`TimeOptions`](TimeOptions.md).

## Methods

| Method | Summary |
|--------|---------|
| [`on`](Workflow/on.md) | Open a branch routed at named columns / a `Route` / a predicate, then `.add(stage)` to it — the `ColumnTransformer`-shaped DSL. |
| [`learn`](Workflow/learn.md) | Apply `Policy` to any raw frame, then fit the held `Model` (`learn_` in place). |
| [`map`](Workflow/map.md) | Run preparation only, no model inference (`learn_map` fits and maps in one call). |
| [`infer`](Workflow/infer.md) | Predict for new data (`infer_scores` for raw scores). |
| [`assess`](Workflow/assess.md) | Score the held `Model` on labelled data; returns a `MetricReport`. |
| [`save`](Workflow/save.md) | Write the trained `Model` to a bundle file, plus a `Policy` sidecar. |
| [`with`](Workflow/with.md) | Rebuild on a different base `Model`, replaying every branch/route step recorded so far. |

## Changelog

- **0.1** — `Workflow` = `Model` + `Policy` + the branch/route DSL; the six
  `Model`-shaped data verbs, `save`/`load` with a `Policy` sidecar, and
  `with(model)` to rebuild on a different base model.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Workflow/Introduction
Workflow/assess
Workflow/infer
Workflow/learn
Workflow/map
Workflow/on
Workflow/save
Workflow/with
```
<!-- apiref:toctree:end -->
