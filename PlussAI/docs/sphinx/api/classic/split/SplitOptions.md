# `pluss::ai::classic::SplitOptions`

Every field every split-related call in `classic` might need — shared
across the direct functions in this family and [`metrics::cross_assess`](../metrics.md) /
[`tune`](../tune.md), which use it to choose and drive a split internally.
Which fields matter depends entirely on which function you call: a field a
given function does not read is simply ignored by it. See each function's
own `## Parameters` section for exactly which subset applies.

## Fields

### `plan`

`CVPlanKind plan = CVPlanKind::Holdout`

Only consulted by [`metrics::cross_assess`](../metrics.md) (and, through
it, [`tune`](../tune.md)) — it picks which function in this family to call
internally: `Holdout` -> [`holdout_split`](holdout.md) (or
[`strat_holdout_split`](holdout.md) — see `stratified`), `CrossVal` ->
[`crossval_plan`](crossval_plan.md) (or
[`stratified_crossval_plan`](crossval_plan.md)), `StratifiedCrossVal` ->
[`stratified_crossval_plan`](crossval_plan.md) unconditionally,
`GroupCrossVal` -> [`group_crossval_plan`](crossval_plan.md) (using
`groups`), `TSCrossVal` -> [`ts_crossval_plan`](ts_crossval_plan.md). Calling
one of this family's functions directly bypasses `plan` entirely — you have
already picked the function that matches it by name.

### `holdout_fraction`

`double holdout_fraction = 0.2`

Read by [`holdout_split`](holdout.md) / [`strat_holdout_split`](holdout.md).
The fraction of rows reserved for testing. Must be finite and in `(0, 1)`.

### `folds`

`std::int64_t folds = 5`

Read by every cross-validation-plan function in
[`crossval_plan`](crossval_plan.md) and
[`ts_crossval_plan`](ts_crossval_plan.md). Must be `>= 2`; for the
non-time-series variants, also `<= sample_count` (and, for
[`group_crossval_plan`](crossval_plan.md), `<=` the number of distinct
groups).

### `repeats`

`std::int64_t repeats = 1`

Read only by [`stratified_crossval_plan`](crossval_plan.md) — runs the
whole stratified k-fold split `repeats` times, concatenating every repeat's
folds into one [`SplitPlan`](SplitPlan.md). Must be `>= 1`. `repeats > 1`
forces every repeat (including the first) to shuffle with an
independently-derived seed, even if `shuffle` was `false` — repeating an
unshuffled split would just produce the same partition every time.

### `time_gap`

`std::int64_t time_gap = 0`

Read only by [`ts_crossval_plan`](ts_crossval_plan.md) — the number of
samples excluded between a fold's train window and its test window. `0`
(the default) means train ends exactly where test begins.

### `time_test_size`

`std::int64_t time_test_size = 0`

Read only by [`ts_crossval_plan`](ts_crossval_plan.md) — the fixed size of
every fold's test window. `<= 0` (the default) infers it as
`sample_count / (folds + 1)`.

### `time_max_train_size`

`std::int64_t time_max_train_size = 0`

Read only by [`ts_crossval_plan`](ts_crossval_plan.md) — caps each fold's
training window to at most this many of the most recent samples. `<= 0`
(the default) leaves the training window unbounded (expanding — every fold
trains on everything before its test window, however much that is).

### `stratified`

`bool stratified = false`

Only consulted by [`metrics::cross_assess`](../metrics.md), and only when
`plan` is `Holdout` or `CrossVal` on a classification task — `true` routes
to the class-stratified variant ([`strat_holdout_split`](holdout.md) /
[`stratified_crossval_plan`](crossval_plan.md)) instead of the plain one.
Has no effect for `plan = StratifiedCrossVal` (always stratified by
definition) or any other `plan` value, and no effect at all when calling a
`split` function directly — call the stratified function by name instead.

### `groups`

`pluss::Series groups = pluss::series::undefined()`

Only consulted by [`metrics::cross_assess`](../metrics.md) when `plan =
GroupCrossVal` — forwarded as the `groups` argument to
[`group_crossval_plan`](crossval_plan.md). Must be defined with length
matching the sample count in that case; irrelevant otherwise, including
when calling `group_crossval_plan` directly (it takes groups as its own
parameter, not through this field).

### `reliability_defaults`

`bool reliability_defaults = true`

Only consulted by [`metrics::cross_assess`](../metrics.md). `true` (the
default): for a classification task with `plan = Holdout` or `CrossVal` and
`stratified` not already `true`, silently upgrades `stratified` to `true` —
stratifying a classification split is almost always what you want, and this
makes it the practical default without changing `SplitOptions`'s own
field default. Set `false` to take `stratified` exactly as given.

### `shuffle`

`bool shuffle = true`

Read by every function except [`ts_crossval_plan`](ts_crossval_plan.md),
which **requires** `false` and fails otherwise — a time-ordered split's
whole point is chronological order. For every other function, `true` (the
default) randomizes row order (or group order, for
[`group_crossval_plan`](crossval_plan.md)) before partitioning; `false`
partitions rows in their original order.

### `max_parallel`

`std::int64_t max_parallel = 0`

Only consulted by [`metrics::cross_assess`](../metrics.md) — the number of
folds evaluated concurrently. `<= 0` (the default) auto-sizes from hardware
concurrency. Irrelevant to every function in this family, which only
compute indices and never evaluate a model.

### `random`

`RandomState random = {}`

The seed/determinism pair behind every shuffle in this family — see
[`RandomState`](../core/RandomState.md).

## Guidance

Calling a `split` function directly, the fields that matter are just the
ones that function's own page names — `plan`/`stratified`/`groups`/
`reliability_defaults`/`max_parallel` only matter once you hand the same
`SplitOptions` to [`metrics::cross_assess`](../metrics.md) or
[`tune`](../tune.md) instead. Leave `reliability_defaults` at its default
`true` for ordinary classification work; set it `false` only if you have a
specific reason an unstratified holdout/cross-val split is what you want
for a classification task.

## See Also

- [`RandomState`](../core/RandomState.md) — the seed/determinism pair.
- [`holdout`](holdout.md) / [`crossval_plan`](crossval_plan.md) /
  [`ts_crossval_plan`](ts_crossval_plan.md) — the functions that read a
  subset of these fields directly.
- [`metrics`](../metrics.md) — `cross_assess`, the primary consumer of the
  fields the direct functions never read.
