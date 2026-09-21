# `classic::compose` Introduction

## What This Family Owns

`compose` owns the shape of a *deployable* pipeline: a `Flow` that carries
zero or more routed transform stages and an optional final model, learned and
run together. Everything it needs comes from families you already know —
[`prep`](../prep.md) supplies the transform kernels, [`families`](../families.md)
supplies the model spec, [`estimators`](../estimators.md) supplies the
classifier/regressor a `Flow` wraps internally.

## Why A Separate Pipeline Type

An [`EstimatorBase`](../estimators/EstimatorBase.md) fits one model on
already-prepared features. Real data usually is not already prepared — it
needs scaling, encoding, imputation, routed to the right columns — and that
preparation needs to travel with the model, fit on the same training call,
applied consistently at inference. `Flow` is that unit: a list of
[`TransformSpec`](../compose/TransformSpec.md) stages plus an optional model,
`learn`ed together in one call, and `infer`/`map`/`assess`ed together too.

## Routing

Every stage can be routed to a subset of feature columns (`feature_route`, a
list of column indices) with a `remainder` policy (`PassThrough` or `Drop`)
for everything the route does not name. A stage with no route touches every
column. This is what lets one `Flow` scale numeric columns and one-hot-encode
categorical columns in the same pipeline, each stage seeing only the columns
it should.

## Two Ways To Add A Stage

- The **typed convenience methods** — `add_standard_scaler`,
  `add_one_hot_encoder`, and eighteen more, one per
  [`prep`](../prep.md) kernel — covered together on [`add_stage`](Flow/add_stage.md)
  since they are all the same shape: options struct in, routed stage out.
- The **kind + options form** — [`add_named_transform`](Flow/add_named_transform.md) /
  [`add_column_stage`](Flow/add_column_stage.md) — for building a stage from a
  `TransformKind` enum value and a `TransformOptions` variant dynamically
  (reading a kind from configuration, building stages in a loop), rather than
  naming the typed method directly.

## Reading Order

1. [`Flow`](../compose/Flow.md) — the type itself; its `## Signature` /
   `## State` sections before diving into individual methods.
2. [`make_flow`](../compose/make_flow.md) — how you obtain one.
3. Pipeline construction: [`add_stage`](Flow/add_stage.md),
   [`add_named_transform`](Flow/add_named_transform.md),
   [`add_column_stage`](Flow/add_column_stage.md).
4. Fit/predict/score: [`learn`](Flow/learn.md), [`map`](Flow/map.md),
   [`infer`](Flow/infer.md), [`assess`](Flow/assess.md).
5. Everything else (stage mutation, model attachment, introspection,
   persistence) as needed.

## Related Pages

- [`prep`](../prep.md) — the transform kernels.
- [`estimators`](../estimators.md) — the model a `Flow` can carry.
- [`io`](../io.md) — whole-`Flow` persistence.
