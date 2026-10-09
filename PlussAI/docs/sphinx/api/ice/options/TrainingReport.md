# `pluss::ai::ice::TrainingReport`

The record of the most recent [`learn`](../model/Model/learn.md) call on a
model, returned by [`Model::training_report`](../model/Model/training_report.md).
It is a plain data struct: calling code reads its fields and never constructs one.
Each `learn` call replaces the previous record, and a fresh model has an empty one.

## Signature

```cpp
pluss::ai::ice::TrainingReport::TrainingReport() = default;
```

## State

### `epochs`

`std::vector<EpochReport> epochs`
: One entry per epoch that ran, in order. Its length is the number of epochs
  the run completed, which is less than the configured maximum when the run
  stopped early. Empty for a model that has never been trained.

### `global_steps`

`std::int64_t global_steps = 0`
: The number of optimizer steps the run took, counted across every epoch.
  It equals the number of training batches times the number of epochs, unless
  the run stopped in the middle of an epoch.

### `stop_reason`

`std::string stop_reason`
: Why the run ended before its configured epochs. Empty when the run completed
  every epoch. Set by a stopping callback, such as early stopping, and set to
  `"exception"` when the run raised an error.

## Methods

None. The record is plain data; every field is public and read directly.

## Guidance

Read `epochs` for the learning curve and `stop_reason` to tell a run that
converged early from one that hit its epoch limit. Read `global_steps` when you
compare runs with different batch sizes, since the epoch count alone does not
say how much optimization happened.

The record is replaced by the next `learn` call, so copy anything you need to
keep before training again. It is also not part of a saved model: a loaded model
starts with an empty record.

## See Also

- [`Model::training_report`](../model/Model/training_report.md) -- returns this record.
- [`EpochReport`](EpochReport.md) -- the type of each `epochs` entry.
- [`AssessReport`](AssessReport.md) -- the evaluation counterpart, for held-out data.

## Practical Examples

Three examples: the learning curve of a run, the early-stop reason, and a
comparison of two runs by their step counts.

### Example 1

Printing the learning curve from `epochs`, one line per epoch.

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
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const ice::TrainingReport& report = model.training_report();
  for (const ice::EpochReport& epoch : report.epochs) {
    std::cout << "epoch " << epoch.epoch << " train_loss = " << epoch.train_loss << "\n";
  }
  return 0;
}
```

Output

```text
epoch 0 train_loss = 2.35997
epoch 1 train_loss = 1.79271
epoch 2 train_loss = 1.31547
epoch 3 train_loss = 0.928253
```

### Example 2

The stop reason of an early-stopped run. The run is configured for twenty
epochs, and an impossible improvement threshold stops it early, so the
program prints the reason next to the epochs actually run.

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
  options.execution.max_epochs = 20;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.25;
  options.loop_regularization.patience = 1;
  options.loop_regularization.min_delta = 1000.0;
  model.learn(x, y, options);

  const ice::TrainingReport& report = model.training_report();
  std::cout << "epochs run = " << report.epochs.size() << " of " << options.execution.max_epochs << "\n";
  std::cout << "stop_reason = \"" << report.stop_reason << "\"\n";
  return 0;
}
```

Output

```text
epochs run = 2 of 20
stop_reason = "early_stopping"
```

### Example 3

Comparing two runs by their optimizer steps. Both runs train on the same data
for the same number of epochs, but with different batch sizes, so the step
counts differ even though the epoch counts match.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  for (const std::int64_t batch_size : {4, 2}) {
    ice::Outline model(nn::Fusion(2, 1));
    ice::TrainOptions options{};
    options.execution.max_epochs = 3;
    options.batch_size = batch_size;
    options.learning_rate = 0.1;
    options.validation.split_ratio = 0.0;
    model.learn(x, y, options);

    const ice::TrainingReport& report = model.training_report();
    std::cout << "batch_size = " << batch_size << ": epochs = " << report.epochs.size()
              << ", global_steps = " << report.global_steps << "\n";
  }
  return 0;
}
```

Output

```text
batch_size = 4: epochs = 3, global_steps = 3
batch_size = 2: epochs = 3, global_steps = 6
```

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
