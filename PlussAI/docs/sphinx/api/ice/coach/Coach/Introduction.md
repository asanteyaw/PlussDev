# `pluss::ai::ice::Coach` Introduction

## What `Coach` Is

`Coach` is where the actual `learn`/`infer`/`assess` orchestration
lives. [`Model`](../../model/Model.md)'s own convenience methods --
`model.learn(x, y, options)` and friends -- are thin wrappers:
`Model::learn` constructs `Coach(options)` and calls its `learn`
right back, every time, with no callbacks attached (see
[`Model::learn`'s own page](../../model/Model/learn.md)). Constructing
a `Coach` directly is the only way to attach a
[`Callback`](../../callback/Callback.md) to a run, or to reuse one
`TrainOptions` value across several `learn`/`infer`/`assess` calls
without repeating it.

## The `Source&` Primitive

Every one of `learn`/`infer`/`assess`'s several overloads reduces to
one `Source&`-taking call: the `Series`-pair and `DataFrame` overloads
each construct a [`TensorSource`](../../source/TensorSource.md) (using
`options().batch_size`/`shuffle`/`seed` for its
[`BatchPlan`](../../source/BatchPlan.md)) and forward to it. This is
not just an implementation detail -- it is the documented, stable
contract each method's own page proves with a side-by-side comparison.
Understanding `Source` is therefore the key to using `Coach` against
anything beyond a plain `(x, y)` pair: windowed/panel data through
[`data::DataSet`](../../data/DataSet.md)'s own
[`source()`](../../data/DataSet/source.md), or a hand-written `Source`
subclass for a feed this reference doesn't have a built-in for.

## Related Pages

- [`callback`](../../callback.md) -- `Callback`/`RunState`/
  `CallbackChain`/`EarlyStopCallback`, everything a `Coach`-driven run
  threads through.
- [`source`](../../source.md) -- `Source`/`TensorSource`/
  `DataSetSource`, what every overload ultimately runs through.
- [`model`](../../model.md) -- `Model::learn`, the convenience wrapper
  built on top of this.
- [`options`](../../options.md) -- `TrainOptions`, what every call
  reads.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="add_callback.html">
    <span class="label">Next</span>
    <span class="title">add_callback</span>
  </a>
</div>
<!-- apiref:nav:end -->
