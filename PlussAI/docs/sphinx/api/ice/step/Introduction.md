# `ice::step` Introduction

## What This Category Owns

`step` owns the three types that flow through a single training step:
[`Batch`](Batch.md) (what a step computes against),
[`StepContext`](StepContext.md) (read-only bookkeeping passed alongside
it), and [`StepResult`](StepResult.md) (what a step returns). Together
they define [`Model`](../model/Model.md)'s own extension points --
`learn_step`/`valid_step`/`test_step`/`infer_step` all share this exact
`(Batch, StepContext) -> StepResult` shape.

## Two `Batch` Types, Not One

`pluss::ai::ice::Batch` (this category) and
[`pluss::ai::ice::data::Batch`](../data/Batch.md) are different types
with overlapping names and different shapes -- the former is a flat
named-field list a `Model` computes against directly; the latter is the
`Layout`-tagged, categorical-aware batch a [`Source`](../source/Source.md)
actually iterates over. [`Source::transfer_batch`](../source/Source.md)
is the conversion point between them, called once per batch inside
[`Coach::learn`](../coach/Coach.md)'s own loop. Every page on this
reference says "core `Batch`" or "`data::Batch`" explicitly wherever the
two could be confused, never just "`Batch`" alone.

## Reading Order

1. [`Batch`](Batch.md) -- the type every step method's first parameter
   is.
2. [`StepContext`](StepContext.md) -- the second parameter; read-only,
   never constructed outside a direct step-method call.
3. [`StepResult`](StepResult.md) -- what every step method returns.

## Related Pages

- [`model`](../model.md) -- `Model`'s own step methods, the primary
  consumer of all three types here.
- [`data`](../data.md) -- `data::Batch`, the different type a `Source`
  iterates over.
- [`source`](../source.md) -- `Source::transfer_batch`, which converts
  between the two `Batch` types.
