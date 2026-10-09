# `pluss::ai::ice::EpochReport`

One epoch of a training run, as an entry of
[`TrainingReport::epochs`](TrainingReport.md). It records the epoch's index,
its mean training loss, and its validation loss when the epoch validated. It is
plain data, and calling code only reads it.

## Signature

```cpp
pluss::ai::ice::EpochReport::EpochReport() = default;
```

## State

### `epoch`

`std::int64_t epoch = -1`
: The zero-based index of the epoch. The first epoch of a run is `0`, and the
  index counts up by one for each epoch that ran.

### `train_loss`

`double train_loss = 0.0`
: The mean training loss over the epoch's batches, computed with the model's
  configured loss. It is measured while the model is training, so it reflects the
  weights as they changed during the epoch.

### `val_loss`

`std::optional<double> val_loss = std::nullopt`
: The mean validation loss over the epoch's validation rows. Empty when the run
  had no validation split, and when the epoch did not validate because of
  `validation.every_n_epochs`. Check it with `has_value()` before reading it.

## Methods

None. The entry is plain data; every field is public and read directly.

## Guidance

Compare `train_loss` with `val_loss` epoch by epoch. Validation loss that keeps
falling while training loss falls is healthy. Validation loss that starts rising
while training loss keeps falling is the usual sign of overfitting, and it is the
pattern early stopping watches for.

Use `train_loss` alone only when the run has no validation split. Use the
validation values to choose a stopping point, rather than the final epoch, when
the run was not stopped early.

## See Also

- [`TrainingReport`](TrainingReport.md) -- the record that holds these entries.
- [`Model::training_report`](../model/Model/training_report.md) -- returns the record.

## Practical Examples

Two examples: a run without validation, and a run where validation runs only on
some epochs.

### Example 1

An entry from a run without a validation split. Its `val_loss` is empty, so the
program checks it before reading it.

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
  options.execution.max_epochs = 2;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const ice::EpochReport& first = model.training_report().epochs.front();
  std::cout << "epoch = " << first.epoch << ", train_loss = " << first.train_loss << "\n";
  std::cout << "has val_loss = " << first.val_loss.has_value() << "\n";
  return 0;
}
```

Output

```text
epoch = 0, train_loss = 2.35997
has val_loss = 0
```

### Example 2

Validation every second epoch. The epochs that did not validate report an empty
`val_loss`, and the program prints both kinds of entry.

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
  options.execution.max_epochs = 4;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.25;
  options.validation.every_n_epochs = 2;
  model.learn(x, y, options);

  for (const ice::EpochReport& epoch : model.training_report().epochs) {
    std::cout << "epoch " << epoch.epoch << ": validated = " << epoch.val_loss.has_value() << "\n";
  }
  return 0;
}
```

Output

```text
epoch 0: validated = 1
epoch 1: validated = 0
epoch 2: validated = 1
epoch 3: validated = 0
```

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
