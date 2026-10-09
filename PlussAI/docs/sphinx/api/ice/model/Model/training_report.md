<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a7ac9aa26a37 -->
# `pluss::ai::ice::Model::training_report`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
const pluss::ai::ice::TrainingReport& pluss::ai::ice::Model::training_report() const noexcept;
```

## What It Does

Returns the record of the most recent [`learn`](learn.md) call on this model:
one [`EpochReport`](../../options/EpochReport.md) for every epoch that ran, the
optimizer step count, and the reason the run stopped early if it did. The
record is the training statistics you would otherwise have to collect with a
custom callback, and it is available as soon as `learn` returns.

Every `learn` overload writes the record, and each call replaces the previous
one. The record is not cleared by `infer`, `assess`, or `eval`, because those
do not train. A model that has never been trained returns an empty record:
no epochs, zero steps, and an empty stop reason.

The record is not saved. [`save`](save.md) writes the weights and the
standardization statistics, and [`load`](load.md) restores them, but a loaded
model reports an empty training record until it is trained again. Copy the
record out before saving if you need it later.

Read the per-epoch losses to see whether training converged, to compare two
runs by their final losses, or to plot a learning curve. `train_loss` is the mean
loss over the epoch's training batches. `val_loss` is present only on epochs that
validated, so it is empty for every epoch of a run with no validation split.

## Parameters

None.

## Returns

A `const` reference to the model's [`TrainingReport`](../../options/TrainingReport.md).
The reference stays valid until the next `learn` call on this model, so copy the
fields you need before training again.

## Errors

None. `training_report` is `noexcept` and never throws.

## Practical Examples

Four examples: a plain run, a run with a validation split, a run that stops
early, and a model that was never trained.

### Example 1

A plain run. The report lists one entry per epoch, and the training loss falls
across them.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 5;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const auto& report = model.training_report();
  std::cout << "epochs = " << report.epochs.size() << ", global_steps = " << report.global_steps << "\n";
  std::cout << "stop_reason = \"" << report.stop_reason << "\"\n";
  std::cout << "first train_loss = " << report.epochs.front().train_loss << "\n";
  std::cout << "last train_loss = " << report.epochs.back().train_loss << "\n";
  return 0;
}
```

Output

```text
epochs = 5, global_steps = 5
stop_reason = ""
first train_loss = 2.35997
last train_loss = 0.631044
```

### Example 2

A run with a validation split. Every epoch now carries a validation loss next
to its training loss, and the program prints both for each epoch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 5;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.25;
  model.learn(x, y, options);

  for (const auto& epoch : model.training_report().epochs) {
    std::cout << "epoch " << epoch.epoch << ": train_loss = " << epoch.train_loss;
    if (epoch.val_loss.has_value()) {
      std::cout << ", val_loss = " << *epoch.val_loss;
    }
    std::cout << "\n";
  }
  return 0;
}
```

Output

```text
epoch 0: train_loss = 1.05412, val_loss = 4.86421
epoch 1: train_loss = 0.768881, val_loss = 3.63218
epoch 2: train_loss = 0.543714, val_loss = 2.59305
epoch 3: train_loss = 0.379738, val_loss = 1.75901
epoch 4: train_loss = 0.276275, val_loss = 1.13142
```

### Example 3

A run that stops early. The validation loss is given a large minimum
improvement, so it never counts as improving and the run stops after the
patience window. The report shows fewer epochs than the configured maximum and
a non-empty stop reason.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 50;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.25;
  options.loop_regularization.early_stopping = true;
  options.loop_regularization.patience = 1;
  options.loop_regularization.min_delta = 1000.0;
  model.learn(x, y, options);

  const auto& report = model.training_report();
  std::cout << "configured max_epochs = " << options.execution.max_epochs << "\n";
  std::cout << "epochs run = " << report.epochs.size() << "\n";
  std::cout << "stop_reason = \"" << report.stop_reason << "\"\n";
  return 0;
}
```

Output

```text
configured max_epochs = 50
epochs run = 2
stop_reason = "early_stopping"
```

### Example 4

A model that has never been trained. Its report is empty, which is how a caller
tells a fresh model from one that trained and ran zero epochs.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const auto& report = model.training_report();
  std::cout << "epochs = " << report.epochs.size() << ", global_steps = " << report.global_steps << "\n";
  std::cout << "ready() = " << model.ready() << "\n";
  return 0;
}
```

Output

```text
epochs = 0, global_steps = 0
ready() = 0
```

## See Also

- [`learn`](learn.md) -- produces the record this method returns.
- [`TrainingReport`](../../options/TrainingReport.md) -- the record's fields.
- [`EpochReport`](../../options/EpochReport.md) -- one epoch's entry.
- [`ready`](ready.md) -- whether the model has been trained or loaded.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="train.html">
    <span class="label">Previous</span>
    <span class="title">train</span>
  </a>
  <a class="next-link" href="valid_step.html">
    <span class="label">Next</span>
    <span class="title">valid_step</span>
  </a>
</div>
<!-- apiref:nav:end -->
