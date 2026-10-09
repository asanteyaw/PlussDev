# `ice::outline` Introduction

## What This Category Owns

`outline` owns exactly one type: [`Outline`](Outline.md), the
common-case [`Model`](../model/Model.md) subclass for composing
existing layers into a trainable model with no hand-written subclass.

## Why Just One Type

Like [`model`](../model.md), `outline` isn't a cluster of related
types -- `Outline`'s own page covers its full surface (a variadic
constructor, one `forward` override, and the `schedule()` accessor
pair), and its own `Introduction.md` child covers its role, its
construction/registration contract, and the C++ name-hiding gotcha
behind its `using Model::forward;` declaration in more depth than fits
here. This landing page exists for consistency with every other
category's own family -> Introduction -> symbol structure.

## Reading Order

1. [`Outline`](Outline.md) -- the landing page, with the method table.
2. [`Outline`'s own Introduction](Outline/Introduction.md) -- the
   construction/registration contract and the name-hiding gotcha.
3. [`forward`](Outline/forward.md) / [`schedule`](Outline/schedule.md)
   -- the two methods `Outline` adds on top of `Model`.

## Related Pages

- [`model`](../model.md) -- `Model`, the base class `Outline` derives
  from; every method not on `Outline`'s own page lives there.
- [`coach`](../coach.md) -- the orchestration `learn`/`infer`/`assess`
  delegate to.
- [`options`](../options.md) -- `TrainOptions`, what `learn` takes.
