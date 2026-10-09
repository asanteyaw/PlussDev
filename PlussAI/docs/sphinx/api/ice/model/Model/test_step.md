<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 64abaefb2d22 -->
# `pluss::ai::ice::Model::test_step`

[Back to `pluss::ai::ice::Model`](../Model.md)

## Signature

```cpp
virtual pluss::ai::ice::StepResult pluss::ai::ice::Model::test_step(const pluss::ai::ice::Batch& batch, const pluss::ai::ice::StepContext& ctx) const;
```

## What It Does

The per-batch test/evaluation step -- identical default body to
[`learn_step`](learn_step.md)/[`valid_step`](valid_step.md) (`forward`
then `compute_loss`). [`Coach::assess`](../../coach/Coach.md) calls this
once per batch, aggregating the results into an
[`AssessReport`](../../options/AssessReport.md). Override this
specifically (leaving `learn_step`/`valid_step` unaffected) for a model
whose assessment-time behavior should differ.

## Parameters

`const Batch& batch`
: The test batch -- must have both `inputs` and `targets` set.

`const StepContext& ctx`
: `ctx.loss_kind` must be set; `ctx.stage` is `StepStage::Test` when
  called from `Coach::assess`.

## Returns

[`StepResult`](../../step/StepResult.md) -- `predictions` and `loss`,
both defined.

## Errors

Same as [`learn_step`](learn_step.md): throws if `batch` has no
`targets`, or `ctx.loss_kind` is unset.

## Practical Examples

Three examples: a direct call, confirming it matches `learn_step`'s
output for the same inputs, and `assess` built on top of it end to end.

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

  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f, 2.0f}, {1, 2}));
  batch.set_targets(pluss::make_series({3.0f}, {1, 1}));

  ice::StepContext ctx{};
  ctx.loss_kind = ice::LossKind::MSE;

  const ice::StepResult result = model.test_step(batch, ctx);
  std::cout << "has_predictions() = " << result.has_predictions() << "\n";
  std::cout << "has_loss() = " << result.has_loss() << "\n";

  return 0;
}
```

Output

```text
has_predictions() = 1
has_loss() = 1
```

### Example 2

Same result as `learn_step` for the same inputs, on a fixed model.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));

  ice::Batch batch{};
  batch.set_inputs(pluss::make_series({1.0f, 2.0f}, {1, 2}));
  batch.set_targets(pluss::make_series({3.0f}, {1, 1}));

  ice::StepContext ctx{};
  ctx.loss_kind = ice::LossKind::MSE;

  const ice::StepResult learn_result = model.learn_step(batch, ctx);
  const ice::StepResult test_result = model.test_step(batch, ctx);

  const double diff = learn_result.loss.sub(test_result.loss).abs().amax().item<double>();
  std::cout << "loss diff = " << diff << "\n";

  return 0;
}
```

Output

```text
loss diff = 0
```

### Example 3

`assess` is `Coach`/`test_step` end to end -- the `AssessReport` it
returns aggregates exactly the per-batch `loss`/`predictions` this
method produces.

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

## See Also

- [`learn_step`](learn_step.md) / [`valid_step`](valid_step.md) -- the
  same default body, for the other two stages.
- [`assess`](assess.md) -- built on top of this, one call per batch.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save.html">
    <span class="label">Previous</span>
    <span class="title">save</span>
  </a>
  <a class="next-link" href="to.html">
    <span class="label">Next</span>
    <span class="title">to</span>
  </a>
</div>
<!-- apiref:nav:end -->
