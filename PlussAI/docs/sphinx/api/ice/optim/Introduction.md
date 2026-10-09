# `ice::optim` Introduction

## What This Category Owns

`optim` owns exactly one function:
[`make_optimizer`](make_optimizer.md), standalone `Optimizer`
construction from a `TrainOptions` value.

## Why Just One Function

Like [`no_grad`](../no_grad.md), `optim` is not a cluster of related
types -- `make_optimizer`'s own page covers its full surface (one
function, one overload). This landing page exists for consistency with
every other category's own family -> Introduction -> symbol structure.

## Reading Order

1. [`make_optimizer`](make_optimizer.md) -- the landing page, with the
   full signature, behavior, and three examples.

## Related Pages

- [`model`](../model.md) -- `Model::learn`, which builds its own
  optimizer through this exact function.
- [`coach`](../coach.md) -- where that internal call happens.
- [`options`](../options.md) -- `TrainOptions`, every field this
  function reads.
