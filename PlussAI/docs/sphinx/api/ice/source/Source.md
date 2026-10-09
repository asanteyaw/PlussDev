# `pluss::ai::ice::Source`

The pluggable train/valid/test/infer batch-feeding abstraction
[`Model`](../model/Model.md)/[`Coach`](../coach/Coach.md) iterate over
-- [`prepare()`](Source/prepare.md) once, then the four `*_batches()`
accessors and [`transfer_batch()`](Source/transfer_batch.md) to
device-resident core [`Batch`](../step/Batch.md). Built-in
implementations: [`TensorSource`](TensorSource.md) over raw
`Series` pairs, and [`DataSetSource`](DataSetSource.md) over
[`data::DataSet`](../data/DataSet.md) parts. Every
[`Coach::learn`/`infer`/`assess`](../coach/Coach.md) overload that does
not already take a `Source&` directly builds one of these two
internally -- see each method's own page for exactly which.

## Signature

```cpp
pluss::ai::ice::Source::~Source() = default;
```

An abstract base class -- `prepare`/`learn_batches`/`valid_batches`/
`test_batches`/`infer_batches` are all pure virtual, so `Source` itself
is never instantiated directly.

## Construction

Never constructed directly (it is abstract); construct
[`TensorSource`](TensorSource.md)/[`DataSetSource`](DataSetSource.md),
or a hand-written subclass -- see
[`Source`'s own Introduction](Source/Introduction.md#writing-a-custom-source)
for the three protected extension points a subclass implements.

## State

Not applicable -- `Source` itself holds no state; everything a
subclass needs to materialize and serve batches is its own concern.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `prepare()` | `void` | [`prepare`](Source/prepare.md) |
| `learn_batches()` | `const std::vector<data::Batch>&` | [`learn_batches`](Source/learn_batches.md) |
| `valid_batches()` | `const std::vector<data::Batch>&` | [`valid_batches`](Source/valid_batches.md) |
| `test_batches()` | `const std::vector<data::Batch>&` | [`test_batches`](Source/test_batches.md) |
| `infer_batches()` | `const std::vector<data::Batch>&` | [`infer_batches`](Source/infer_batches.md) |
| `transfer_batch(batch, ctx)` | `Batch` | [`transfer_batch`](Source/transfer_batch.md) |

## Changelog

- **0.1** -- `Source`: the four-stage batch abstraction, with
  `transfer_batch`'s fixed `before_transfer`/`do_transfer_batch`/
  `after_transfer` extension sequence.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Source/Introduction
Source/infer_batches
Source/learn_batches
Source/prepare
Source/test_batches
Source/transfer_batch
Source/valid_batches
```
<!-- apiref:toctree:end -->
