# `pluss::ai::ice::outline`

`outline` owns [`Outline`](outline/Outline.md), the common-case
[`Model`](model/Model.md) subclass for the plain-chain-of-layers case:
compose existing layers via a
[`Schedule`](../nn/module/Schedule.md) and get every `Model` method --
`learn`/`infer`/`assess`/`save`/`load`/`to` -- for free, with no
hand-written subclass.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Outline`](outline/Outline.md) | the one-liner `Model`: a constructor forwarding into an owned `Schedule`, a `forward` override delegating to it, and a `schedule()` accessor pair for introspecting or extending the composition afterward |

## How To Call It

```cpp
namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

ice::Outline model(nn::Fusion(2, 1));
const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

ice::TrainOptions options{};
options.execution.max_epochs = 100;
options.learning_rate = 0.1;
options.validation.split_ratio = 0.0;
model.learn(x, y, options);

std::cout << "ready() = " << model.ready() << "\n";
```

```text
ready() = 1
```

`Outline` is a `Model` subclass, not a separate thing -- every `Model`
method documented under [`model`](model.md) works on it exactly as
written. Reach for a hand-written `Model` subclass instead only when
the architecture is not a straight chain: branching, multiple inputs, a
custom loss, or per-stage logic beyond what `Schedule` itself can
express.

## Related Pages

- [`model`](model.md) -- `Model`, the base class `Outline` derives
  from; every method not on `Outline`'s own page lives there.
- [`coach`](coach.md) -- the orchestration `learn`/`infer`/`assess`
  delegate to, for `Outline` exactly as for any other `Model`.
- [`options`](options.md) -- `TrainOptions`, what `learn` takes.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

outline/Introduction
outline/Outline
```
<!-- apiref:toctree:end -->
