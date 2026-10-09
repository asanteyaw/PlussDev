# `pluss::ai::ice::EpochLoggerCallback`

`EpochLoggerCallback` renders completed epoch metrics as ordinary log lines or as
an in-place terminal progress bar. `Coach` creates one automatically when
[`TrainOptions::progress.verbose`](../options/ProgressPolicy.md) is `true`; you
normally configure that policy instead of constructing the callback yourself.

Construct the callback explicitly when output belongs in a caller-owned stream,
such as an application log, test buffer, notebook channel, or GUI console. The
callback stores a non-owning pointer to that stream, so the stream must outlive
the callback and the training run.

## Signatures

```cpp
explicit EpochLoggerCallback(ProgressPolicy policy = {});
EpochLoggerCallback(ProgressPolicy policy, std::ostream& output);
```

The one-argument constructor selects `std::cout` or `std::cerr` from
`policy.output`. The two-argument constructor always writes to `output`; its
explicit stream takes precedence over `policy.output`.

The callback observes `RunState` only. It does not modify gradients, parameters,
metrics, stop requests, or callback persistence. It reports only
`StepStage::Learn` epoch boundaries, so reusing it in a chain that also drives
assessment and inference does not produce misleading epoch lines for those
stages.

`ProgressPolicy::verbose` controls automatic installation by `Coach`; an
explicitly constructed callback writes regardless of that flag. This distinction
allows a custom-stream callback to use the default policy without also asking
`Coach` to install a second reporter.

## Reporting Rules

- Epoch numbers in output are one-based; `RunState::epoch` remains zero-based.
- Cadence is measured by completed epochs.
- The configured final epoch is always emitted.
- An early-stopped epoch is emitted even when it is off cadence.
- Missing selected metrics are omitted.
- A progress-bar line is closed on run completion or exception.
- Metric order follows `ProgressPolicy::metrics`, not unordered-map iteration.

## Practical Examples

### Example 1

Automatic reporting through `Model::learn`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x_train = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y_train = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 25;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  options.progress.verbose = true;
  options.progress.log_every_n_epochs = 5;

  model.learn(x_train, y_train, options);
  return 0;
}
```

Output

```text
epoch 5/25: loss=0.631044
epoch 10/25: loss=0.336448
epoch 15/25: loss=0.310084
epoch 20/25: loss=0.0878253
epoch 25/25: loss=0.0895766
```

No explicit `Coach` or callback object is needed. `Model::learn` delegates to the same coach machinery and therefore receives the same reporter. Five lines appear, one every fifth epoch.

### Example 2

Capture reports in a caller-owned stream.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x_train = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y_train = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::ProgressPolicy policy{};
  policy.log_every_n_epochs = 2;
  policy.metrics = {"loss"};

  std::ostringstream log;
  auto reporter = std::make_shared<ice::EpochLoggerCallback>(policy, log);

  ice::TrainOptions options{};
  options.execution.max_epochs = 6;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  ice::Coach coach(options, {reporter});
  coach.learn(model, x_train, y_train);

  std::cout << log.str();
  return 0;
}
```

Output

```text
epoch 2/6: loss=1.79271
epoch 4/6: loss=0.928253
epoch 6/6: loss=0.424276
```

Because the callback is explicit, `policy.verbose` may remain `false`. The stream contains epochs 2, 4, and 6; the program prints the captured text.

### Example 3

Select a callback-produced metric.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x_train = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y_train = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  class ScoreCallback final : public ice::Callback {
  public:
    void on_epoch_end(ice::RunState& state) override {
      if (const auto loss = state.metric("loss")) {
        state.put_metric("score", 1.0 / (1.0 + *loss));
      }
    }
  };

  ice::ProgressPolicy policy{};
  policy.metrics = {"loss", "score"};
  policy.log_every_n_epochs = 2;

  ice::TrainOptions options{};
  options.execution.max_epochs = 4;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  ice::Coach coach(options);
  coach.add_callback(std::make_shared<ScoreCallback>());
  coach.add_callback(std::make_shared<ice::EpochLoggerCallback>(policy));
  coach.learn(model, x_train, y_train);
  return 0;
}
```

Output

```text
epoch 2/4: loss=1.79271, score=0.358075
epoch 4/4: loss=0.928253, score=0.518604
```

Callback order matters here: the score producer is added before the logger, so `score` exists when the logger reads the epoch state.

### Example 4

Keep interactive output on one line.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x_train = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y_train = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::ProgressPolicy policy{};
  policy.progress_bar = true;
  policy.bar_width = 40;
  policy.log_every_n_epochs = 1;

  ice::TrainOptions options{};
  options.execution.max_epochs = 4;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  ice::Coach coach(options, {std::make_shared<ice::EpochLoggerCallback>(policy)});
  coach.learn(model, x_train, y_train);
  return 0;
}
```

Output

```text

[==========                              ] 25% epoch 1/4: loss=2.35997

[====================                    ] 50% epoch 2/4: loss=1.79271

[==============================          ] 75% epoch 3/4: loss=1.31547

[========================================] 100% epoch 4/4: loss=0.928253
```

Use line mode instead when output is redirected to a file or CI log, because carriage-return rendering is meaningful only to an interactive display. The captured output shows each rewrite in sequence.

## See Also

- [`ProgressPolicy`](../options/ProgressPolicy.md) -- every rendering and cadence
  option.
- [`Callback`](Callback.md) -- the lifecycle surface this implementation uses.
- [`TrainingReport`](../options/TrainingReport.md) -- inspect the same core epoch
  losses after training without producing live output.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `policy() const` | `const ProgressPolicy&` | [`policy`](EpochLoggerCallback/policy.md) |

The callback's `on_epoch_end`, `on_run_end`, and `on_exception` hooks are the
`Callback` interface and are driven by `Coach`; they are not called directly.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

EpochLoggerCallback/policy
```
<!-- apiref:toctree:end -->
