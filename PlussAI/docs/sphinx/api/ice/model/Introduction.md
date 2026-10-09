# `ice::model` Introduction

## What This Category Owns

`model` owns exactly one type: [`Model`](Model.md), the base class every
trainable `ice` model derives from -- the generic escape hatch: define
parameters plus `forward`/`compute_loss`, get `learn`/`infer`/`assess`/
`save`/`load`, gradient clipping, and non-finite diagnostics for free.

## Why Just One Type

Unlike most `ice` categories, `model` isn't a cluster of related types --
`Model`'s own page covers its full surface (16 methods across an
override ladder, convenience overloads, and lifecycle accessors), and a
dedicated `Model/Introduction.md` child page covers its role and
construction contract in more depth than fits here. This landing page
exists for consistency with every other category's own
family -> Introduction -> symbol structure, not because `model` has
more than one thing to introduce.

## Reading Order

1. [`Model`](Model.md) -- the landing page, with the full method table.
2. [`Model`'s own Introduction](Model/Introduction.md) -- the override
   ladder (which method to override for which kind of change) and the
   construction contract (`register_parameter`/`register_module`).

## Related Pages

- [`outline`](../outline.md) -- `Outline`, the common-case `Model`
  subclass for a plain chain of layers.
- [`coach`](../coach.md) -- the orchestration `learn`/`infer`/`assess`
  delegate to.
- [`step`](../step.md) -- `Batch`/`StepContext`/`StepResult`, the types
  the override ladder's step methods operate on.
- [`options`](../options.md) -- `TrainOptions`, what every `learn`
  overload takes.
