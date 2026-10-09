# `ice::no_grad` Introduction

## What This Category Owns

`no_grad` owns exactly one type:
[`NoGradGuard`](NoGradGuard.md), the `ice`-local RAII no-grad scope.

## Why Just One Type

Like [`optim`](../optim.md), `no_grad` is not a cluster of related
types -- `NoGradGuard` has no methods beyond its constructors and
destructor, so its own page covers its full surface directly, with no
`Introduction` child of its own (its role, construction contract, and
examples all live on that one page). This landing page exists for
consistency with every other category's own
family -> Introduction -> symbol structure.

## Reading Order

1. [`NoGradGuard`](NoGradGuard.md) -- the landing page, with the full
   signature, construction contract, and three examples.
2. [`nn::NoGradGuard`](../../nn/grad/NoGradGuard.md) -- the type this
   delegates to, for the underlying `AutoGradMode` mechanics.

## Related Pages

- [`model`](../model.md) -- `Model::infer`/`assess`, already wrapped in
  one of these.
- [`coach`](../coach.md) -- where that wrapping actually happens.
