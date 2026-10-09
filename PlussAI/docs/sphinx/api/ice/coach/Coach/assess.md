<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 00e68ff6f374 -->
# `pluss::ai::ice::Coach::assess`

[Back to `pluss::ai::ice::Coach`](../Coach.md)

## Signature

```cpp
[[nodiscard]] AssessReport pluss::ai::ice::Coach::assess(const Model& model, Source& source, LossKind loss) const;
[[nodiscard]] AssessReport pluss::ai::ice::Coach::assess(const Model& model, const Series& x,
                                                         const Series& y, LossKind loss) const;
[[nodiscard]] AssessReport pluss::ai::ice::Coach::assess(const Model& model, const pluss::table::DataFrame& df,
                                                         std::string_view target, LossKind loss) const;
```

## What It Does

Runs `model`'s predictions against ground truth and reports a single
summary metric plus the raw loss. Like [`learn`](learn.md)/
[`infer`](infer.md), the `Source&` overload is the primitive -- the
`(x, y)` overload builds a [`TensorSource`](../../source/TensorSource.md)
through its dedicated two-`Series` constructor (not the four-argument
training one -- `assess` has no notion of a validation split), and the
`DataFrame` overload extracts a feature matrix and target column, then
calls the `(x, y)` overload.

## Parameters

`const Model& model`
: The trained model -- `const`, briefly switched to evaluation mode
  internally and restored afterward.

`Source& source`
: A pluggable batch feed, iterated once over `test_batches()`.

`const Series& x`, `const Series& y`
: Features and ground-truth targets.

`const pluss::table::DataFrame& df`, `std::string_view target`
: Same convention as [`infer`](infer.md)'s `DataFrame` overload, except
  `target` is required here (not defaulted) -- its values are read as
  `y`, not just excluded from the feature matrix.

`LossKind loss`
: Which loss [`AssessReport::loss`](../../options/AssessReport.md) and
  the summary metric are derived from -- `MSE`/`BCEWithLogits`/
  `CrossEntropy` map to `rmse`/`accuracy`/`accuracy` respectively; see
  [`AssessReport`](../../options/AssessReport.md) for the exact mapping.

## Returns

[`AssessReport`](../../options/AssessReport.md) -- `loss`, `metric`,
and `metric_name`.

## Errors

- Throws if `source` produces no test batches.
- Throws whatever [`Model::test_step`](../../model/Model/test_step.md)
  throws for a shape mismatch.

## Practical Examples

Three examples, one per overload: the common `(x, y)` case, the
`Source&` primitive reconstructing exactly what that case builds
internally, and the `DataFrame` overload.

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

  const ice::Coach coach(options);
  coach.learn(model, x, y);

  const auto report = coach.assess(model, x, y, ice::LossKind::MSE);
  std::cout << "metric_name = " << report.metric_name << "\n";
  std::cout << "metric < 0.1: " << (report.metric < 0.1) << "\n";

  return 0;
}
```

Output

```text
metric_name = rmse
metric < 0.1: 1
```

### Example 2

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

  const ice::Coach coach(options);
  coach.learn(model, x, y);

  // assess(model, x, y, loss) constructs exactly this TensorSource
  // internally -- the dedicated two-Series constructor, not the
  // four-argument training one.
  ice::TensorSource source(x, y);
  const auto report_via_source = coach.assess(model, source, ice::LossKind::MSE);
  const auto report_via_pair = coach.assess(model, x, y, ice::LossKind::MSE);

  std::cout << "loss diff = " << std::abs(report_via_source.loss - report_via_pair.loss) << "\n";
  std::cout << "metric_name matches: " << (report_via_source.metric_name == report_via_pair.metric_name) << "\n";

  return 0;
}
```

Output

```text
loss diff = 0
metric_name matches: 1
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f32 = pluss::Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const pluss::Series x0 = pluss::make_series({0.0f, 0.0f, 1.0f, 1.0f}, {4}, f32);
  const pluss::Series x1 = pluss::make_series({0.0f, 1.0f, 0.0f, 1.0f}, {4}, f32);
  const pluss::Series label = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4}, f32);

  const DataFrame df({
      {"x0", Slate(x0)},
      {"x1", Slate(x1)},
      {"label", Slate(label)},
  });

  ice::Outline model(nn::Fusion(2, 1));
  ice::TrainOptions options{};
  options.execution.max_epochs = 200;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;

  const ice::Coach coach(options);
  coach.learn(model, df, "label");

  const auto report = coach.assess(model, df, "label", ice::LossKind::MSE);
  std::cout << "metric_name = " << report.metric_name << "\n";
  std::cout << "metric < 0.1: " << (report.metric < 0.1) << "\n";

  return 0;
}
```

Output

```text
metric_name = rmse
metric < 0.1: 1
```

## See Also

- [`learn`](learn.md) / [`infer`](infer.md) -- the matching overload
  families for the other two phases.
- [`AssessReport`](../../options/AssessReport.md) -- the return type,
  including the `LossKind` -> metric name mapping.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_callback.html">
    <span class="label">Previous</span>
    <span class="title">add_callback</span>
  </a>
  <a class="next-link" href="callbacks.html">
    <span class="label">Next</span>
    <span class="title">callbacks</span>
  </a>
</div>
<!-- apiref:nav:end -->
