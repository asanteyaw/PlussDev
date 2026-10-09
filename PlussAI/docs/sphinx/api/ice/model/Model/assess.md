<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 78029ff3604d -->
# `pluss::ai::ice::Model::assess`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
pluss::ai::ice::AssessReport pluss::ai::ice::Model::assess(pluss::ai::ice::Source& source, pluss::ai::ice::LossKind loss) const;
pluss::ai::ice::AssessReport pluss::ai::ice::Model::assess(const pluss::Series& x, const pluss::Series& y, pluss::ai::ice::LossKind loss) const;
pluss::ai::ice::AssessReport pluss::ai::ice::Model::assess(const pluss::table::DataFrame& df, std::string_view target, pluss::ai::ice::LossKind loss) const;
```

## What It Does

Runs one batched forward + loss pass, plus a loss-appropriate metric
derived from it -- every overload is a thin wrapper around
`Coach(...).assess(*this, ...)`. `loss` is required rather than
remembered from the last `learn()` call on purpose: a remembered value
wouldn't survive [`save`](save.md)/[`load`](load.md) (the artifact has
no metadata channel for it), so it would be silently wrong for a freshly
loaded model -- always pass the same `LossKind` used to train.

The `DataFrame` overload mirrors `learn(df, target, options)`: pulls
every column except `target` as a feature, applies whatever
standardization `learn` fit, then delegates to the `Series` overload.

## Parameters

`Source& source`
: An already-constructed data source.

`const Series& x` / `const Series& y`
: The features/targets to assess against.

`const pluss::table::DataFrame& df`
: A `DataFrame` whose columns are features plus one target column.

`std::string_view target`
: The target column's name.

`LossKind loss`
: Which loss to compute -- must match the `LossKind` the model was
  trained with for the result to be meaningful.

## Returns

[`AssessReport`](../../options/AssessReport.md) -- the loss and a
loss-appropriate metric (RMSE for `MSE`, accuracy for
`BCEWithLogits`/`CrossEntropy`).

## Errors

Throws if the source produces no rows, or on a malformed input shape.

## Practical Examples

Three examples, one per overload.

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
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const auto report = model.assess(x, y, ice::LossKind::MSE);
  std::cout << "metric_name = " << report.metric_name << "\n";

  return 0;
}
```

Output

```text
metric_name = rmse
```

### Example 2

The `Source` overload.

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
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  ice::TensorSource source(x, y);
  const auto report = model.assess(source, ice::LossKind::MSE);
  std::cout << "metric_name = " << report.metric_name << " metric = " << report.metric << "\n";

  return 0;
}
```

Output

```text
metric_name = rmse metric = 2.81476e-05
```

### Example 3

The `DataFrame` overload.

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

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const DataFrame df({
      {"a", Slate(pluss::make_series({0.0f, 0.0f, 1.0f, 1.0f}, {4}))},
      {"b", Slate(pluss::make_series({0.0f, 1.0f, 0.0f, 1.0f}, {4}))},
      {"label", Slate(pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4}))},
  });
  const auto report = model.assess(df, "label", ice::LossKind::MSE);
  std::cout << "metric_name = " << report.metric_name << " metric = " << report.metric << "\n";

  return 0;
}
```

Output

```text
metric_name = rmse metric = 2.65688e-05
```

## See Also

- [`AssessReport`](../../options/AssessReport.md) -- the result
  returned.
- [`learn`](learn.md) / [`infer`](infer.md) -- training and plain
  inference.
- [`test_step`](test_step.md) -- the per-batch step this aggregates.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="compute_loss.html">
    <span class="label">Next</span>
    <span class="title">compute_loss</span>
  </a>
</div>
<!-- apiref:nav:end -->
