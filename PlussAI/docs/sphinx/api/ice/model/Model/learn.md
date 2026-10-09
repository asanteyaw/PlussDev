<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 25a5bffd31c9 -->
# `pluss::ai::ice::Model::learn`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
void pluss::ai::ice::Model::learn(pluss::ai::ice::Source& source, const pluss::ai::ice::TrainOptions& options = {});
void pluss::ai::ice::Model::learn(const pluss::Series& x_train, const pluss::Series& y_train, const pluss::ai::ice::TrainOptions& options = {});
void pluss::ai::ice::Model::learn(const pluss::Series& x_train, const pluss::Series& y_train, const pluss::Series& x_val, const pluss::Series& y_val, const pluss::ai::ice::TrainOptions& options = {});
void pluss::ai::ice::Model::learn(const pluss::table::DataFrame& df, std::string_view target, const pluss::ai::ice::TrainOptions& options = {});
```

## What It Does

Trains the model -- every overload is a thin wrapper around
`Coach(options).learn(*this, ...)`; nothing here is virtual, so a
subclass changes `learn`'s actual behavior by overriding `forward`/
`compute_loss`/the step methods, not `learn` itself.

- The `(x_train, y_train, options)` overload carves out an internal
  validation split when `options.validation.split_ratio > 0`.
- The `(x_train, y_train, x_val, y_val, options)` overload instead uses
  an already-split validation pair the caller supplies -- for callers
  managing their own splitting.
- The `(df, target, options)` overload pulls every column except
  `target` as features via one batched `DataFrame::matrix()` call,
  optionally standardizes them (`options.standardize`), then delegates
  to the `Series` overload. When `standardize = true`, the fitted
  per-column mean/std are kept on the model (persisted through
  [`save`](save.md)/[`load`](load.md)) and reapplied automatically by
  the `DataFrame` overloads of [`infer`](infer.md)/[`assess`](assess.md).
- Every overload leaves a record of the run behind. [`training_report`](training_report.md)
  returns it: one entry per epoch, with the training and validation losses, and
  the step count and stop reason.
- The `Source` overload trains against an already-constructed
  [`Source`](../../source/Source.md) directly -- the common path when a
  [`DataSet`](../../data/DataSet.md)/[`TimeSeriesDataSet`](../../data/TimeSeriesDataSet.md)
  or a hand-built `TensorSource`/`DataSetSource` is already in hand,
  rather than raw tensors.

## Parameters

`Source& source`
: An already-constructed data source.

`const Series& x_train` / `const Series& y_train`
: The training features/targets.

`const Series& x_val` / `const Series& y_val`
: An already-split validation features/targets pair.

`const pluss::table::DataFrame& df`
: A `DataFrame` whose columns are features plus one target column.

`std::string_view target`
: The target column's name; every other column is used as a feature.

`TrainOptions options = {}`
: See [`TrainOptions`](../../options/TrainOptions.md).

## Returns

`void`.

## Errors

Throws on malformed options (see each nested policy struct's own page)
or a numerically unstable step, when
[`ExecutionPolicy::fail_on_non_finite`](../../options/ExecutionPolicy.md)
is set (the default).

## Practical Examples

Four examples, one per overload.

### Example 1

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
  options.execution.max_epochs = 100;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const auto& report = model.training_report();
  std::cout << "epochs = " << report.epochs.size() << ", global_steps = " << report.global_steps << "\n";
  std::cout << "first train_loss = " << report.epochs.front().train_loss
            << ", last train_loss = " << report.epochs.back().train_loss << "\n";
  std::cout << "stop_reason = \"" << report.stop_reason << "\"\n";

  return 0;
}
```

Output

```text
epochs = 100, global_steps = 100
first train_loss = 2.35997, last train_loss = 5.02573e-05
stop_reason = ""
```

### Example 2

The already-split train/val pair overload.

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
  options.execution.max_epochs = 100;
  options.learning_rate = 0.1;
  model.learn(x, y, x, y, options);

  const auto& report = model.training_report();
  std::cout << "epochs = " << report.epochs.size() << ", global_steps = " << report.global_steps << "\n";
  std::cout << "first train_loss = " << report.epochs.front().train_loss
            << ", last train_loss = " << report.epochs.back().train_loss << "\n";
  std::cout << "stop_reason = \"" << report.stop_reason << "\"\n";

  return 0;
}
```

Output

```text
epochs = 54, global_steps = 54
first train_loss = 2.35997, last train_loss = 0.00239722
stop_reason = "early_stopping"
```

### Example 3

The `DataFrame` overload -- every column except `"label"` becomes a
feature.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/mic.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const DataFrame df({
      {"a", Slate(pluss::make_series({0.0f, 0.0f, 1.0f, 1.0f}, {4}))},
      {"b", Slate(pluss::make_series({0.0f, 1.0f, 0.0f, 1.0f}, {4}))},
      {"label", Slate(pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4}))},
  });

  ice::Outline model(nn::Fusion(2, 1));
  ice::TrainOptions options{};
  options.execution.max_epochs = 100;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(df, "label", options);

  const auto& report = model.training_report();
  std::cout << "epochs = " << report.epochs.size() << ", global_steps = " << report.global_steps << "\n";
  std::cout << "first train_loss = " << report.epochs.front().train_loss
            << ", last train_loss = " << report.epochs.back().train_loss << "\n";
  std::cout << "stop_reason = \"" << report.stop_reason << "\"\n";

  return 0;
}
```

Output

```text
epochs = 100, global_steps = 100
first train_loss = 2.35997, last train_loss = 5.02573e-05
stop_reason = ""
```

### Example 4

The `Source` overload, over a [`TensorSource`](../../source/TensorSource.md).

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

  ice::Outline model(nn::Fusion(2, 1));
  ice::BatchPlan plan{};
  plan.batch_size = 4;
  ice::TensorSource source(x, y, std::nullopt, std::nullopt, plan);

  ice::TrainOptions options{};
  options.execution.max_epochs = 100;
  options.learning_rate = 0.1;
  model.learn(source, options);

  const auto& report = model.training_report();
  std::cout << "epochs = " << report.epochs.size() << ", global_steps = " << report.global_steps << "\n";
  std::cout << "first train_loss = " << report.epochs.front().train_loss
            << ", last train_loss = " << report.epochs.back().train_loss << "\n";
  std::cout << "stop_reason = \"" << report.stop_reason << "\"\n";

  return 0;
}
```

Output

```text
epochs = 100, global_steps = 100
first train_loss = 2.35997, last train_loss = 5.02573e-05
stop_reason = ""
```

## See Also

- [`TrainOptions`](../../options/TrainOptions.md) -- every field this
  takes.
- [`Coach::learn`](../../coach/Coach.md) -- what every overload here
  delegates to.
- [`infer`](infer.md) / [`assess`](assess.md) -- the matching
  post-training calls.
- [`Source`](../../source/Source.md) -- `TensorSource`/`DataSetSource`,
  the `Source` overload's argument.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_training.html">
    <span class="label">Previous</span>
    <span class="title">is_training</span>
  </a>
  <a class="next-link" href="learn_step.html">
    <span class="label">Next</span>
    <span class="title">learn_step</span>
  </a>
</div>
<!-- apiref:nav:end -->
