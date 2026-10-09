# `pluss::ai::ice::source`

`source` owns the pluggable batch-feeding abstraction
[`Model`](model.md)/[`Coach`](coach.md) iterate over:
[`Source`](source/Source.md) (the base class), two built-in
implementations ([`TensorSource`](source/TensorSource.md) over raw
`Series` pairs, [`DataSetSource`](source/DataSetSource.md) over
[`data::DataSet`](data/DataSet.md) parts), and the two context types
every call threads through
([`BatchPlan`](source/BatchPlan.md)/[`SourceContext`](source/SourceContext.md)).

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Source`](source/Source.md) | the abstraction: `prepare()` once, then `learn_batches`/`valid_batches`/`test_batches`/`infer_batches`, and `transfer_batch` to convert a host batch to a device-resident core `Batch` |
| [`TensorSource`](source/TensorSource.md) | the `Source` over raw `Series` pairs -- what every `Coach::learn`/`infer`/`assess` `Series`/`DataFrame` overload builds internally |
| [`DataSetSource`](source/DataSetSource.md) | the `Source` over `data::DataSet` parts, with independent train/eval `BatchPlan`s |
| [`BatchPlan`](source/BatchPlan.md) | batch size/shuffle/seed for a `Source`'s own materialization |
| [`SourceContext`](source/SourceContext.md) | the read-only context `transfer_batch` reads: recipe, stage, target platform/dtype |

## How To Call It

```cpp
namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

ice::Outline model(nn::Fusion(2, 1));
const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

ice::TrainOptions options{};
options.execution.max_epochs = 1;
options.validation.split_ratio = 0.0;

// Never calling source.prepare() -- Coach::learn calls it.
ice::TensorSource source(x, y, std::nullopt, std::nullopt, ice::BatchPlan{});
ice::Coach coach(options);
coach.learn(model, source);

std::cout << "ready() = " << model.ready() << "\n";
```

```text
ready() = 1
```

Reach for `TensorSource`/`DataSetSource` constructed directly (instead
of `Coach`'s `Series`/`DataFrame` convenience overloads) for finer
control over train/valid/test/infer roles and their own `BatchPlan`s;
reach for a hand-written `Source` subclass for data that doesn't fit
either shape -- see
[`Source`'s own Introduction](source/Source/Introduction.md#writing-a-custom-source).

## Related Pages

- [`coach`](coach.md) -- `Coach`, every consumer of `Source`.
- [`data`](data.md) -- `data::Batch`/`data::DataSet`, what
  `DataSetSource` is built from and every `*_batches()` accessor
  returns.
- [`step`](step.md) -- the core `Batch`, what `transfer_batch` produces.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

source/Introduction
source/BatchPlan
source/SourceContext
source/Source
source/TensorSource
source/DataSetSource
```
<!-- apiref:toctree:end -->
