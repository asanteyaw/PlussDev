# `pluss::ai::ice::options`

`options` owns `TrainOptions` (public alias for the declared `Recipe`
struct) -- the central training configuration every `Model::learn` call
and every `models_*` catalog entry's `learn()` takes -- its five nested
policy structs, and `AssessReport`, the result `assess()` returns.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`TrainOptions`](options/TrainOptions.md) | batch size, learning rate, optimizer/loss kind, shuffle/seed, standardize, plus the five policy structs below |
| [`ValidationPolicy`](options/ValidationPolicy.md) | validation split ratio, evaluation cadence, monitored metric name/direction |
| [`ArchitectureRegularization`](options/ArchitectureRegularization.md) | dropout strength (`TextJudge`-specific override) |
| [`ObjectiveRegularization`](options/ObjectiveRegularization.md) | weight decay, L1/L2 penalties, label smoothing |
| [`LoopRegularization`](options/LoopRegularization.md) | early stopping, patience, min delta, best-weight restoration |
| [`CheckpointPolicy`](options/CheckpointPolicy.md) | whether/when/where to write checkpoints, and how to resume |
| [`ExecutionPolicy`](options/ExecutionPolicy.md) | gradient clipping, epoch/batch limits, non-finite failure, determinism, precision, platform |
| [`ProgressPolicy`](options/ProgressPolicy.md) | live epoch output, cadence, progress-bar rendering, stream selection, and displayed metrics |
| [`AssessReport`](options/AssessReport.md) | the loss + metric result `assess()` returns |

## How To Call It

```cpp
namespace ice = pluss::ai::ice;
namespace nn = pluss::nn;

ice::Outline model(nn::Fusion(2, 1));

const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

ice::TrainOptions options{};
options.execution.max_epochs = 100;
options.learning_rate = 0.05;
options.loss = ice::LossKind::MSE;
options.validation.split_ratio = 0.0;

model.learn(x, y, options);
std::cout << "ready() = " << model.ready() << "\n";

const auto predictions = model.infer(x);
std::cout << "predictions:\n" << predictions << "\n";
```

```text
ready() = 1
predictions:
pluss::Series(shape=[4,1], strides=[1,1], dtype=float32, platform=cpu, numel=4, contiguous=true)
data=[[-0.0127552],
  [0.99250835],
  [0.99608034],
  [2.001344]]
```

`TrainOptions{}` on its own -- no fields set at all -- is already a
reasonable first training configuration: Adam at `1e-3`, 60 epochs,
gradient clipping and non-finite failure already on, no validation
split. Setting only the fields that matter for a given run (as above) is
the common pattern; every field not touched keeps its default.

## Related Pages

- [`model`](model.md) -- `Model::learn`/`infer`/`assess` are
  `TrainOptions`'s primary consumers.
- [`coach`](coach.md) -- the model-external orchestration surface built
  around the same `TrainOptions`.
- [`optim`](optim.md) -- `make_optimizer`, built from
  `TrainOptions::optimizer`/`learning_rate`/`objective_regularization`/
  `execution`.
- [`callback`](callback.md) -- `EarlyStopCallback`, built from
  `TrainOptions::validation`/`loop_regularization`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

options/Introduction
options/TrainOptions
options/ValidationPolicy
options/ArchitectureRegularization
options/ObjectiveRegularization
options/LoopRegularization
options/CheckpointPolicy
options/ExecutionPolicy
options/ProgressPolicy
options/AssessReport
options/TrainingReport
options/EpochReport
```
<!-- apiref:toctree:end -->
