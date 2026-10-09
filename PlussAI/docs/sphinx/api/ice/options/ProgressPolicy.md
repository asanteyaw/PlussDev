# `pluss::ai::ice::ProgressPolicy`

`ProgressPolicy` controls live, epoch-level reporting from an Ice training run.
It answers three separate questions: whether `Coach` should install a reporter,
which completed epochs should be shown, and how each report should be rendered.
It does not change optimization, validation, stopping, or the retained
[`TrainingReport`](TrainingReport.md).

The policy is stored in [`TrainOptions::progress`](TrainOptions.md). Its default
is deliberately silent, so existing applications and libraries do not begin
writing to a process stream after an upgrade. Set `verbose = true` to make
`Model::learn(...)` and `Coach::learn(...)` install an
[`EpochLoggerCallback`](../callback/EpochLoggerCallback.md) automatically.
The same field and logger are used by `TFTTrainOptions`,
`AutoregressiveTrainOptions`, and `ActionValueTrainOptions`. `TextJudge` retains
its classifier-specific training report and does not currently expose this
epoch logger because its `max_epochs` setting is delegated to a task optimizer
as an iteration budget rather than driven by Ice's epoch loop.

## Fields

### `verbose`

`bool`, default `false`. Turns on the built-in epoch logger that `Coach` installs
automatically. A manually installed [`EpochLoggerCallback`](../callback/EpochLoggerCallback.md)
writes regardless of this flag, so the flag only controls automatic reporting.

### `log_every_n_epochs`

`std::int64_t`, default `1`. Reports every Nth completed epoch. It counts completed
epochs, not zero-based indices, so a cadence of `5` reports epochs 5, 10, 15, and so on.

### `progress_bar`

`bool`, default `false`. Rewrites one live terminal line with a filled bar instead of
emitting a permanent line per report. Use it only for an interactive terminal.

### `output`

`ProgressOutput`, default `ProgressOutput::Stdout`. The process stream the reports go to:
`Stdout` or `Stderr`.

### `metrics`

`std::vector<std::string>`, default `{"loss", "val_loss"}`. The metric names to show, in
the order given. A metric the epoch did not produce is omitted from the line.

### `bar_width`

`std::int64_t`, default `24`. The character width of the filled and unfilled parts of the
progress bar.

### `precision`

`std::int64_t`, default `6`. The number of significant digits used for metric values.

## Guidance

`log_every_n_epochs` counts completed epochs, not zero-based epoch indices. A
cadence of `5` therefore reports epochs 5, 10, 15, and so on. The final epoch is
always reported even when it falls between cadence boundaries. An epoch that
requests early stopping is also reported, with its stop reason, so a sparse
cadence cannot hide why the run ended.

Metric names preserve the order in `metrics`. `loss` is the mean training loss
for the completed epoch. `val_loss` exists only on epochs where validation ran;
the logger omits it on other epochs instead of printing a fabricated zero.
Custom callbacks may place additional values in `RunState::metrics`, and those
values can be selected here by name.

`progress_bar = true` is intended for an interactive terminal. It writes carriage
returns while training and terminates the live line on completion, early stop,
or exception. Use ordinary line mode for redirected logs, CI output, or records
that another program will parse.

## Validation

When reporting is enabled, `log_every_n_epochs` and `bar_width` must be positive,
`precision` must be non-negative, and every requested metric name must be
nonempty. Invalid settings fail before the first epoch. Settings are preserved in
full checkpoints; older checkpoints without progress fields load with the silent
defaults.

## Practical Examples

### Example 1

Report every epoch.

With validation enabled, each line contains the completed and configured epoch counts, followed by that run's measured `loss` and `val_loss` values.

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
  options.execution.max_epochs = 6;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.25;
  options.progress.verbose = true;

  model.learn(x, y, options);
  return 0;
}
```

Output

```text
epoch 1/6: loss=1.05412, val_loss=4.86421
epoch 2/6: loss=0.768881, val_loss=3.63218
epoch 3/6: loss=0.543714, val_loss=2.59305
epoch 4/6: loss=0.379738, val_loss=1.75901
epoch 5/6: loss=0.276275, val_loss=1.13142
epoch 6/6: loss=0.226552, val_loss=0.695925
```

### Example 2

Use sparse, machine-readable lines.

This example reports every second epoch, with one metric and ten significant digits. Output goes to standard output here so it appears in the example; the `Stderr` setting sends the same lines to standard error for programs that read standard output.

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
  options.execution.max_epochs = 6;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  options.progress.verbose = true;
  options.progress.log_every_n_epochs = 2;
  options.progress.output = ice::ProgressOutput::Stdout;
  options.progress.metrics = {"loss"};
  options.progress.precision = 10;

  model.learn(x, y, options);
  return 0;
}
```

Output

```text
epoch 2/6: loss=1.792711735
epoch 4/6: loss=0.9282534719
epoch 6/6: loss=0.4242762327
```

The reports fall on epochs 2, 4, and 6, and the final epoch is always reported.

### Example 3

Render an interactive progress bar.

During training, one line is rewritten with the filled bar, percentage, completed and configured epoch counts, and whichever selected metrics are available. The captured output below shows each rewrite in sequence.

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
  options.progress.verbose = true;
  options.progress.progress_bar = true;
  options.progress.bar_width = 32;
  options.progress.metrics = {"loss", "val_loss"};

  model.learn(x, y, options);
  return 0;
}
```

Output

```text

[======                          ] 20% epoch 1/5: loss=1.05412, val_loss=4.86421

[============                    ] 40% epoch 2/5: loss=0.768881, val_loss=3.63218

[===================             ] 60% epoch 3/5: loss=0.543714, val_loss=2.59305

[=========================       ] 80% epoch 4/5: loss=0.379738, val_loss=1.75901

[================================] 100% epoch 5/5: loss=0.276275, val_loss=1.13142
```

## See Also

- [`EpochLoggerCallback`](../callback/EpochLoggerCallback.md) -- the reporter
  configured by this policy, including custom-stream use.
- [`TrainingReport`](TrainingReport.md) -- retained epoch history available after
  `learn()` returns.
- [`RunState`](../callback/RunState.md) -- owns the named metrics read by the logger.
