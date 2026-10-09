# `ice::source` Introduction

## What This Category Owns

`source` owns the pluggable batch-feeding abstraction
[`Coach`](../coach/Coach.md) iterates over: the base class
[`Source`](Source.md), its two built-in implementations
([`TensorSource`](TensorSource.md)/[`DataSetSource`](DataSetSource.md)),
and the two plain context types every call threads through
([`BatchPlan`](BatchPlan.md)/[`SourceContext`](SourceContext.md)).

## Why `Source` Exists

Every [`Coach::learn`/`infer`/`assess`](../coach/Coach.md) overload
that does not already take a `Source&` directly builds one internally
-- `(x, y)` and `DataFrame` overloads build a
[`TensorSource`](TensorSource.md); nothing about `Coach`'s own loop
cares which concrete `Source` it is driving. This is what makes
[`data::DataSet`](../data/DataSet.md)-backed, windowed/panel training
possible through the exact same `Coach::learn(model, source)` call a
plain `Series` pair uses, and what makes a hand-written `Source`
subclass a genuine extension point rather than a special case -- see
[`Source`'s own Introduction](Source/Introduction.md#writing-a-custom-source)
for a complete one.

## Reading Order

1. [`Source`](Source.md) -- the landing page, with the full method
   table.
2. [`Source`'s own Introduction](Source/Introduction.md) -- the
   `prepare`/`*_batches()`/`transfer_batch` contract, and a
   hand-written subclass exercising it.
3. [`TensorSource`](TensorSource.md) / [`DataSetSource`](DataSetSource.md)
   -- the two built-in implementations, and exactly which constructor
   populates which `*_batches()` accessor.
4. [`BatchPlan`](BatchPlan.md) / [`SourceContext`](SourceContext.md) --
   the two plain context types.

## Related Pages

- [`coach`](../coach.md) -- `Coach`, every consumer of `Source`.
- [`data`](../data.md) -- `data::Batch`/`data::DataSet`, what
  `DataSetSource` is built from.
- [`step`](../step.md) -- the core `Batch`, what `transfer_batch`
  produces.
