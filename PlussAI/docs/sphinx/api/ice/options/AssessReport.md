# `pluss::ai::ice::AssessReport`

A plain result struct: the loss and a loss-appropriate metric returned by
[`Model::assess`](../model/Model.md)/[`Coach::assess`](../coach/Coach.md).
Never constructed directly by calling code -- every instance comes back
from one of those calls.

## Signature

```cpp
pluss::ai::ice::AssessReport::AssessReport() = default;
```

A plain aggregate -- every field below defaults to `0.0`/empty, but a
default-constructed instance is never meaningful on its own; every real
instance comes from `assess()`.

## State

### `loss`

`double loss = 0.0`
: The mean per-row loss over every row the `assess()` call covered, at
  the `LossKind` passed to it.

### `metric`

`double metric = 0.0`
: A loss-appropriate metric computed from the same predictions/targets:
  RMSE for `LossKind::MSE`, accuracy for `LossKind::BCEWithLogits` or
  `LossKind::CrossEntropy`.

### `metric_name`

`std::string metric_name`
: Which metric `metric` is -- `"rmse"` or `"accuracy"`, matching the
  `LossKind` above. Read this rather than assuming a fixed meaning for
  `metric`, since it depends on the loss passed to `assess()`.

## Methods

None -- `AssessReport` is a plain data result; every field is public and
read directly, see `## State` above.

## Practical Examples

One example: training a small regression model, then reading `loss`/
`metric`/`metric_name` back off `assess()`.

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
  options.loss = ice::LossKind::MSE;
  options.validation.split_ratio = 0.0;
  model.learn(x, y, options);

  const ice::AssessReport report = model.assess(x, y, ice::LossKind::MSE);
  std::cout << "loss = " << report.loss << "\n";
  std::cout << "metric_name = " << report.metric_name << "\n";
  std::cout << "metric = " << report.metric << "\n";

  return 0;
}
```

Output

```text
loss = 6.06732e-10
metric_name = rmse
metric = 2.46319e-05
```

## Changelog

- **0.1** — the loss/metric/metric_name result of `assess()`.

## See Also

- [`Model::assess`](../model/Model.md) / [`Coach::assess`](../coach/Coach.md)
  -- the two producers.
- [`TrainOptions::loss`](TrainOptions.md) -- the `LossKind` that decides
  which metric `assess()` computes.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
