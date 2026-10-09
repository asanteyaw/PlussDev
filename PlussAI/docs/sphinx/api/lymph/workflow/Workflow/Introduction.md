# `lymph::workflow::Workflow` Introduction

## What This Type Represents

`Workflow` is the object that carries a raw-data admission contract, a
column-routing DSL, and a trainable [`Model`](../../models/Model.md) through
one shared lifecycle. Concretely, it can:

- reject or trim a raw `DataFrame` before any prep stage sees it (`Policy`)
- route different named columns through different chains of prep stages,
  joined back together, the way a `ColumnTransformer` would (`on(...)`)
- accept whichever data shape a caller already holds — `DataFrame`,
  `FeatureBatch`, `LabeledBatch` — as one `Payload` argument, instead of a
  parameter list that grows with every new role
- fit, predict, and score with the same verb names `Model` has, because it
  delegates to the `Model` it holds once `Policy` has run
- opt into a leakage-aware time contract that chronological validation reads
  back out
- persist and reload as one bundle, `Policy` included

## Compared To A Pipeline And ColumnTransformer

If the closest thing you already know is a `Pipeline` (an ordered list of
named steps ending in an estimator) composed with a `ColumnTransformer`
(different column groups routed through different sub-pipelines), here is
exactly how `Workflow` maps onto that, and where it genuinely does not.

**A branch is a sub-pipeline; several branches are the column split.** Each
`on(name, columns)` call opens one branch, and each `.add(stage)` on it
appends one more step to *that branch's own* ordered chain — the direct
analog of a `numeric_pipeline`/`categorical_pipeline` list. Two branches on
one `Workflow` are the `ColumnTransformer` step: each routes its own columns
through its own chain, and the routed output feeds the one `Model` the
`Workflow` holds.

```cpp
// final_pipe's ("model", …) — the estimator the whole thing feeds.
workflow::Workflow final_pipe = workflow::make(models::logistic_regression());

// ("num", numeric_pipeline, numeric_features)
final_pipe.on("num", numeric_features)
    .add(prep::stage::simple_imputer({.strategy = prep::SimpleImputerStrategy::Median}))
    .add(prep::stage::standard_scaler());

// ("cat", categorical_pipeline, categorical_features)
// include_unknown_bucket = true (the default) is the handle_unknown="ignore" behavior.
final_pipe.on("cat", categorical_features)
    .add(prep::stage::simple_imputer({.strategy = prep::SimpleImputerStrategy::MostFrequent}))
    .add(prep::stage::one_hot_encoder({.include_unknown_bucket = true}));

const workflow::Workflow fitted = final_pipe.learn(raw, data_options);
```

That is the full mapping of a two-argument
`Pipeline([("preprocessing", ColumnTransformer([("num", …), ("cat", …)])),
("model", LogisticRegression())])` — the numeric branch's imputer/scaler
chain, the categorical branch's imputer/encoder chain, and the estimator
they both feed, in one `Workflow` instead of four nested objects.

The real difference: a branch holds a flat list of `Stage` *values*, not a
nested `Model`/`Workflow` object. You cannot build a `numeric_pipeline` as
its own standalone, independently-reusable value and hand it to a second
object the way a `Pipeline` instance gets passed into a `ColumnTransformer`
— `Workflow` is one flat object with N branches, not an object graph of
nested pipelines. If two branches genuinely need to share a sub-chain,
build it once as a `WorkflowBranchSpec` (see [`make`](../make.md)) and reuse
that value across `on(...)` calls instead.

**A single chain over every column** — the shape of a bare `Pipeline([("imputer",
...), ("scaler", ...), ("model", ...)])` with no column split at all — has no
dedicated constructor. Route one branch at `prep::route::remaining_of({})`
(the complement of an empty name list, i.e. every column) instead of naming
columns explicitly:

```cpp
wf.on(prep::route::remaining_of({}))
    .add(prep::stage::simple_imputer())
    .add(prep::stage::standard_scaler());
```

**A literal `Pipeline([(name, step), (name, step), ...])` list-of-pairs
constructor does not exist.** The closest equivalents are the branch DSL
above (declarative form: [`make`](../make.md)'s `WorkflowBranchSpec`/
`WorkflowStepSpec` list) or `models::Model::append(...)`'s bulk overload,
which takes a `{stage, route}` pair list directly on a bare `Model` with no
`Workflow`/`Policy` layer at all — reach for that when there is no raw-frame
admission contract to enforce and no column split to express.

**Custom transformers exist, but at a coarser grain.**
[`prep::stage::custom(name, fn, contract)`](../../prep/stage/custom.md) wraps
any `Series -> Series` callable as a stage — a lambda is enough; no
subclassing. Two real constraints follow directly from how it attaches,
though: it goes on with `Workflow::add(...)`/`Model::add(...)` at the
**whole-workflow level**, not routed to one branch's columns the way a
built-in stage is; and it makes the `Workflow` non-cloneable, so only the
in-place `learn_(...)` works with it — the value-returning `learn(...)`
clones first and rejects any workflow carrying one:

```cpp
const prep::CustomStage doubled = prep::stage::custom(
    "double", [](const Series& x) { return x.mul(2.0); }, prep::TransformContract{});

workflow::Workflow wf = workflow::make(models::logistic_regression());
wf.add(doubled);
wf.learn_(raw, data_options);      // works
(void)wf.learn(raw, data_options); // throws: Flow::clone rejects opaque stages
```

**Where `Workflow` genuinely goes further:** every column carries a real
dtype and runs through vectorized `Series` operations end to end — no
per-row interpreter loop, no separate array-conversion boundary between
"the pipeline" and "the model." `Policy` is a governance layer with no
`Pipeline`/`ColumnTransformer` counterpart at all — column admission
(`require`/`forbid`/`drop`) and row admission (`reject_missing`) are stated
once and replayed identically on every `learn`/`infer`/`assess`/`save` call,
instead of being ad hoc code the caller has to keep in sync by hand.
[`TimeOptions`](../TimeOptions.md) gives chronological cross-validation a
leakage contract (`horizon`/`gap`/`lookback`) it reads directly off the
`Workflow`, rather than the caller re-deriving fold windows at each call
site.

## Why It Lives Under `workflow`, Not `models`

`Model` owns *how one estimator learns and predicts*. `Workflow` owns
*what is allowed to reach that estimator, and how raw columns get routed to
it* — a layer above, not a replacement. A `Workflow` always holds exactly one
`Model` (`workflow.model()`); every data verb is `Policy::apply(...)` (when
the argument is a raw frame) followed by the identically-named `Model` call.
Nothing about *how* the model itself learns lives in `workflow` — that split
is deliberate, matching how `prep` shapes features and `metrics` measures
results without either becoming `models`.

## The Branch/Route DSL

`Workflow::on(...)` opens a **branch** — a named or anonymous group of columns
matched by a `Route` (explicit names, or a `(name, dtype) -> bool` predicate)
— and returns a `WorkflowRoute` you chain `.add(stage)` calls onto to build
that branch's chain of prep stages:

Route two column groups through separate stages with the workflow DSL. The program prints the branch count the workflow reports.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

namespace lymph = pluss::ai::lymph;
using pluss::table::DataFrame;
using pluss::table::Slate;

lymph::LabeledBatch fixture_batch(const pluss::Config& cfg) {
  const pluss::Series x = pluss::make_series(std::vector<double>{
      0.0, 0.1, 0.2, 0.0, 0.1, 0.3, 0.3, 0.2,
      1.0, 1.1, 1.2, 0.9, 1.1, 1.3, 1.4, 1.0}, {8, 2}, cfg);
  const pluss::Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, cfg);
  return lymph::LabeledBatch{
      .features = lymph::FeatureBatch{.x = x, .layout = lymph::ObservationLayout::Rows},
      .y = y,
  };
}

int main() {
  namespace prep = lymph::prep;
  lymph::workflow::Workflow workflow = lymph::workflow::make(lymph::workflow::Policy{});
  workflow.on({"income", "balance"}).add(prep::stage::standard_scaler());
  workflow.on("category", {"segment"}).add(prep::stage::one_hot_encoder());
  std::cout << "branches = " << workflow.branch_count() << "\n";
  return 0;
}
```

Output

```text
branches = 2
```

Every `.add(...)` call appends the stage straight onto the held `Model`'s own
prep pipeline, routed at that branch's columns — `Workflow` does not run a
second, separate transform step. `Workflow::branch(name)` is the same thing
spelled as two calls instead of one (`workflow.branch("category").route({...})`
is equivalent to `workflow.on("category", {...})`) — reach for whichever reads
better when a branch's route is decided somewhere other than the `on(...)`
call site. `join()` is purely informational bookkeeping today; nothing in
`Workflow` currently changes behavior based on it.

The declarative alternative — build every branch in one `workflow::make(...)`
call from `WorkflowBranchSpec`/`WorkflowStepSpec` values instead of chaining
`.on(...).add(...)` — is covered on [`make`](../make.md).

## `with(model)`: Replaying The Route Onto A New Base

`Workflow::with(model)` returns a new `Workflow` on a *different* base
`Model`, but keeps this `Workflow`'s `Policy`, branches, and every stage
recorded through `on(...)`/`branch(...)` — it replays those recorded steps
onto the new model rather than copying the old model's already-materialized
state. Reach for it when a `Search`/`Suite` factory closure builds a fresh
candidate `Model` per trial and needs to reuse a `Workflow`'s routing without
re-typing it.

## Reading Order

1. [`on`](on.md) — the branch/route DSL, `Workflow`'s one genuinely new
   concern next to `Model`.
2. [`learn`](learn.md) — fitting, the same four data shapes every other verb
   accepts.
3. [`map`](map.md) / [`infer`](infer.md) / [`assess`](assess.md) — the rest
   of the `Model`-shaped verb set.
4. [`save`](save.md) — persistence, `Policy` sidecar included.
5. [`with`](with.md) — rebuilding on a new base model.

## Notes

### Policy runs once per call, only on raw frames

`Policy::apply(...)` only ever sees a raw `DataFrame` argument — the
`LabeledBatch` overload of every verb skips it entirely (a `LabeledBatch` is
already past the raw-admission stage). The `(DataFrame x, Series y)` /
`(DataFrame x, Slate y)` overloads apply `Policy` to `x` but reject a
`Policy` with `filter_nulls` set (dropping rows there would desynchronize `x`
from the already-separate `y`) — use the `(frame, DataFrameBatchOptions)`
overload instead when the policy needs to filter rows.

### A `Payload`'s frame also goes through `Policy`

When a verb's argument is a `Payload` built from a `DataFrame`, `Workflow`
applies `Policy` to that frame exactly as it would a bare raw-frame argument
— `Payload` does not bypass admission.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="assess.html">
    <span class="label">Next</span>
    <span class="title">assess</span>
  </a>
</div>
<!-- apiref:nav:end -->
