# `pluss::ai::ice::step`

`step` owns the three types that flow through a single
[`Model`](model.md) training step: [`Batch`](step/Batch.md) (a flat
named-field list -- distinct from [`data::Batch`](data/Batch.md), see
the Introduction page for how the two relate),
[`StepContext`](step/StepContext.md) (read-only per-step bookkeeping),
and [`StepResult`](step/StepResult.md) (predictions and/or loss).

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Batch`](step/Batch.md) | `inputs`/`targets` typed accessors, plus `field`/`has`/`set` for any other named tensor |
| [`StepContext`](step/StepContext.md) | `recipe`/`loss_kind`/`stage`/`epoch`/`batch_index` |
| [`StepResult`](step/StepResult.md) | `predictions`/`loss`, each possibly undefined |

## How To Call It

```cpp
namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

ice::Outline model(nn::Fusion(2, 1));

ice::Batch batch{};
batch.set_inputs(pluss::make_series({1.0f, 2.0f}, {1, 2}));
batch.set_targets(pluss::make_series({3.0f}, {1, 1}));

ice::StepContext ctx{};
ctx.loss_kind = ice::LossKind::MSE;

const ice::StepResult result = model.learn_step(batch, ctx);
std::cout << "has_predictions() = " << result.has_predictions() << "\n";
std::cout << "has_loss() = " << result.has_loss() << "\n";
```

```text
has_predictions() = 1
has_loss() = 1
```

Most callers never touch `Batch`/`StepContext`/`StepResult` directly --
[`Model::learn`](model/Model.md)/`infer`/`assess` build and consume them
internally through [`Coach`](coach/Coach.md). They matter directly in
two cases: calling a step method by hand (as above, mainly for testing a
custom override in isolation), or overriding a step method in a `Model`
subclass and reading its `StepContext` argument -- see
[`StepContext`](step/StepContext.md)'s own example for the second case,
training end to end with a logging override.

## Related Pages

- [`model`](model.md) -- `Model`'s own step methods, the primary
  consumer.
- [`data`](data.md) -- `data::Batch`, the different type a `Source`
  iterates over.
- [`source`](source.md) -- `Source::transfer_batch`, converting between
  the two `Batch` types.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

step/Introduction
step/Batch
step/StepContext
step/StepResult
```
<!-- apiref:toctree:end -->
