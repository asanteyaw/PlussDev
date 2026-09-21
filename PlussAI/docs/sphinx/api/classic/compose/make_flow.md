# `pluss::ai::classic::make_flow`

`make_flow` builds a [`Flow`](Flow.md) seeded with a final model from a
[`ModelSpec`](../families/ModelSpec.md) — the usual starting point, so you
only need to add transform stages before [`learn`](Flow/learn.md). Aliased
`make_workflow`.

## Signature

```cpp
classic::StatusOr<classic::Flow> classic::make_flow(const classic::ModelSpec& spec);

// Alias:
classic::StatusOr<classic::Workflow> classic::make_workflow(const classic::ModelSpec& spec);
```

## Parameters

`spec` — `ModelSpec`
: The model the flow's final stage will be. `.family` / `.options` must
  agree, the same rule [`make_classifier`](../estimators/make_classifier.md) /
  [`make_regressor`](../estimators/make_regressor.md) enforce — see
  [`ModelSpec`](../families/ModelSpec.md#errors). `.task` may be
  `Classification`, `Regression`, or `Transform`; `make_flow` does not
  restrict it the way the three `make_*` estimator factories do, since a
  `Flow` can also be transform-only.

## Returns

A `StatusOr<Flow>`. `Flow` is move-only — extract it with
[`.move_value()`](../core/StatusOr.md), not `.value()`, since `.value()`
returns a reference and `Flow` cannot be copy-constructed from one.

## Errors

- `Status::invalid_argument` — `spec.family` does not match the alternative
  active in `spec.options`, the same check `make_classifier` /
  `make_regressor` / `make_transformer` perform on their own model specs.

## Practical Examples

### Example 1

Building a flow, extracting it with `move_value()`, and checking its initial
state before any stages or `learn` call.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made =
      cls::make_flow(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";

  cls::Flow flow = made.move_value();
  std::cout << "has_model_spec() = " << flow.has_model_spec() << "\n";
  std::cout << "has_model()      = " << flow.has_model() << "\n";
  std::cout << "stage_count()    = " << flow.stage_count() << "\n";
  std::cout << "trained()        = " << flow.trained() << "\n";

  return 0;
}
```

Output

```text
made.ok() = true
has_model_spec() = true
has_model()      = true
stage_count()    = 0
trained()        = false
```

### Example 2

`make_workflow` — the same function under its alias name.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::StatusOr<cls::Workflow> made = cls::make_workflow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";

  cls::Flow flow = made.move_value();  // classic::Workflow is classic::Flow
  std::cout << "task_kind() == Regression: " << (flow.task_kind() == cls::TaskKind::Regression) << "\n";

  return 0;
}
```

Output

```text
made.ok() = true
task_kind() == Regression: true
```

### Example 3

A mismatched `ModelSpec` — `make_flow` rejects it exactly like the estimator
factories.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ModelSpec mismatched{};
  mismatched.task = cls::TaskKind::Classification;
  mismatched.family = cls::ModelFamily::Tree;
  mismatched.options = cls::LinearFamilyOptions{};  // says Tree, holds Linear

  const cls::StatusOr<cls::Flow> made = cls::make_flow(mismatched);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Tree classifier requires TreeFamilyOptions in ModelSpec.options."
```

## See Also

- [`Flow`](Flow.md) — what you get back.
- [`ModelSpec`](../families/ModelSpec.md) — the input.
- [`set_model`](Flow/set_model.md) — attach (or replace) a model on a `Flow`
  built without one.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
