# `pluss::ai::ice::model`

`model` owns [`Model`](model/Model.md), the base class every trainable
`ice` model derives from -- directly, or through
[`Outline`](outline/Outline.md) for the common case of a plain chain of
layers. Define `forward`/`compute_loss`, get `learn`/`infer`/`assess`/
`save`/`load`/`to` for free.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`Model`](model/Model.md) | the generic base: `forward`/`compute_loss` extension points, four step methods, `learn`/`infer`/`assess`, `save`/`load`, `to`, lifecycle flags |

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

const auto& report = model.training_report();
std::cout << "epochs = " << report.epochs.size() << ", global_steps = " << report.global_steps << "\n";
std::cout << "last train_loss = " << report.epochs.back().train_loss << "\n";
```

```text
epochs = 100, global_steps = 100
last train_loss = 5.02573e-05
```

`Outline` is a `Model` subclass, not a separate thing -- every `Model`
method documented here works on it exactly as written. A hand-written
`Model` subclass (for anything beyond a plain chain of layers) only ever
has to implement `forward(const Series&)`; everything else -- the four
step methods, `learn`/`infer`/`assess`, `save`/`load` -- is inherited
unchanged.

## Related Pages

- [`outline`](outline.md) -- `Outline`, the common-case `Model`
  subclass.
- [`coach`](coach.md) -- the orchestration `learn`/`infer`/`assess`
  delegate to.
- [`step`](step.md) -- `Batch`/`StepContext`/`StepResult`, the types the
  override ladder's step methods operate on.
- [`options`](options.md) -- `TrainOptions`, what every `learn` overload
  takes.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

model/Introduction
model/Model
```
<!-- apiref:toctree:end -->
