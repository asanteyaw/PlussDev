<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 977afdea3225 -->
# `pluss::ai::classic::Flow::set_regressor`

`set_regressor` is [`set_classifier`](set_classifier.md)'s exact counterpart
for `RegressorBase`: attaches an already-built regressor directly, bypassing
[`ModelSpec`](../../families/ModelSpec.md)/
[`make_regressor`](../../estimators/make_regressor.md).

## Signature

```cpp
classic::Status classic::Flow::set_regressor(std::unique_ptr<classic::RegressorBase> model);
```

## Parameters

`model` — `std::unique_ptr<RegressorBase>`
: Ownership transfers to the `Flow`. Replaces any existing classifier or
  regressor outright — [`set_classifier`](set_classifier.md) and
  `set_regressor` are mutually exclusive, not additive.

## Returns

A `Status`. `.ok()` on success —
[`has_model()`](../Flow.md#state) becomes `true`,
[`has_model_spec()`](../Flow.md#state) becomes `false`,
[`task_kind()`](../Flow.md#state) becomes `Regression`, and
[`trained()`](../Flow.md#state) resets to `false`.

## Errors

- `Status::invalid_argument` — `model` is null.

## Practical Examples

### Example 1

Attaching a regressor built by hand via
[`make_regressor`](../../estimators/make_regressor.md).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};  // default objective (LeastSquares) is fine for regression
  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> model_or =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, linear});

  cls::Flow flow{};
  const cls::Status s = flow.set_regressor(model_or.move_value());
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";
  std::cout << "task_kind() == Regression: " << (flow.task_kind() == cls::TaskKind::Regression) << "\n";

  return 0;
}
```

Output

```text
ok = true
has_model() = true
task_kind() == Regression: true
```

### Example 2

A null model.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow{};
  const cls::Status s = flow.set_regressor(nullptr);
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow::set_regressor requires a non-null model."
```

### Example 3

Replaces an existing classifier outright — `set_classifier` and
`set_regressor` are mutually exclusive, not additive.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions classifier_opts{};
  classifier_opts.objective = cls::LinearObjective::Logistic;
  cls::Flow flow{};
  flow.set_classifier(
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, classifier_opts})
          .move_value());
  std::cout << "task_kind() after set_classifier() == Classification: "
            << std::boolalpha << (flow.task_kind() == cls::TaskKind::Classification) << "\n";

  cls::LinearFamilyOptions regressor_opts{};
  flow.set_regressor(
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, regressor_opts})
          .move_value());
  std::cout << "task_kind() after set_regressor() == Regression: "
            << (flow.task_kind() == cls::TaskKind::Regression) << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";

  return 0;
}
```

Output

```text
task_kind() after set_classifier() == Classification: true
task_kind() after set_regressor() == Regression: true
has_model() = true
```

## See Also

- [`set_classifier`](set_classifier.md) — the mutually-exclusive counterpart.
- [`set_model`](set_model.md) — build from a `ModelSpec` instead of an
  already-built model.
- [`clear_model`](clear_model.md) — remove whichever of the two is set.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_nested_param.html">
    <span class="label">Previous</span>
    <span class="title">set_nested_param</span>
  </a>
  <a class="next-link" href="set_stage_key.html">
    <span class="label">Next</span>
    <span class="title">set_stage_key</span>
  </a>
</div>
<!-- apiref:nav:end -->
