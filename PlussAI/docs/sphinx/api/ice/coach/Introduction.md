# `ice::coach` Introduction

## What This Category Owns

`coach` owns exactly one type: [`Coach`](Coach.md), the model-external
`learn`/`infer`/`assess` orchestration surface.

## Why Just One Type

Like [`optim`](../optim.md), `coach` is not a cluster of related types
-- `Coach`'s own page covers its full surface (construction,
`options`/`callbacks`/`add_callback`, and the `learn`/`infer`/`assess`
overload families), and its own
[`Introduction`](Coach/Introduction.md) child covers the `Source&`
primitive every overload reduces to, in more depth than fits here. This
landing page exists for consistency with every other category's own
family -> Introduction -> symbol structure.

## Reading Order

1. [`Coach`](Coach.md) -- the landing page, with the full method table.
2. [`Coach`'s own Introduction](Coach/Introduction.md) -- the
   `Source&` primitive every `learn`/`infer`/`assess` overload reduces
   to.
3. [`learn`](Coach/learn.md) / [`infer`](Coach/infer.md) /
   [`assess`](Coach/assess.md) -- each overload family's own page.

## Related Pages

- [`callback`](../callback.md) -- `Callback`/`RunState`/
  `CallbackChain`/`EarlyStopCallback`, everything a `Coach`-driven run
  threads through.
- [`source`](../source.md) -- `Source`/`TensorSource`/`DataSetSource`,
  what every overload ultimately runs through.
- [`model`](../model.md) -- `Model::learn`, the convenience wrapper
  built on top of this.
