# `lymph::models::Model` Introduction

## What This Type Represents

`Model` is the object that carries a concrete estimator through its normal
lifecycle.

It is not just "family options plus a fit function". It is the stable artifact
that can:

- accumulate attached preparation
- delay preparation materialization until real data arrives
- train in-place or return a trained copy
- infer on new data with the same preparation contract
- emit report objects instead of raw metric fragments
- survive persistence boundaries

## Why It Lives Under `models`

`Model` belongs in `models` because its core concern is estimator ownership.

It consumes `prep`, `options`, and `reports`, but it does not become those
families. That concern split matters:

- `prep` shapes the incoming features
- `options` configures the work
- `reports` packages the measured result
- `workflow` can later wrap the trained artifact into a larger runtime

## Reading Order

The pages below are ordered so you can move from setup to execution:

1. `prepare`
2. `append`
3. `learn`
4. `infer`
5. `assess`
6. `save`
7. `load`

## Notes

### State

Three quick state readings explain most surprises on this surface: `trained()`,
`stage_count()`, and `task_kind()`. The most common source of confusion is
pending materialization — a model can have a real preparation plan attached while
`stage_count()` still reads `0`. That is not a contradiction: the plan is staged
but not yet resolved against a concrete frame.

### Ownership

`learn_` changes the left-hand object; `learn` returns a trained copy. That split
is useful in experiments where one template is reused several times without
accidental mutation.

### Persistence

`save` and `load` belong to the artifact layer. If the thing you need to persist
is broader than one model, step upward into `workflow` or `io`.

## Related Pages

- [models family](../../models.md)
- [Builder type](../builder/Builder.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="append.html">
    <span class="label">Next</span>
    <span class="title">append</span>
  </a>
</div>
<!-- apiref:nav:end -->
