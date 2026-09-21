<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1fe0cb62d2de -->
# `pluss::ai::classic::Flow::set_classifier`

`set_classifier` attaches an already-built `ClassifierBase` directly,
bypassing [`ModelSpec`](../../families/ModelSpec.md)/
[`make_classifier`](../../estimators/make_classifier.md) entirely — useful
when the classifier came from somewhere other than a `ModelSpec` (loaded
state, a custom subclass). Unlike [`set_model`](set_model.md), there is no
spec behind the attached model: [`has_model_spec()`](../Flow.md#state)
becomes `false`.

## Signature

```cpp
classic::Status classic::Flow::set_classifier(std::unique_ptr<classic::ClassifierBase> model);
```

## Parameters

`model` — `std::unique_ptr<ClassifierBase>`
: Ownership transfers to the `Flow`. Replaces any existing
  classifier or regressor outright — `set_classifier` and
  [`set_regressor`](set_regressor.md) are mutually exclusive, not additive.

## Returns

A `Status`. `.ok()` on success —
[`has_model()`](../Flow.md#state) becomes `true`,
[`has_model_spec()`](../Flow.md#state) becomes `false`,
[`task_kind()`](../Flow.md#state) becomes `Classification`, and
[`trained()`](../Flow.md#state) resets to `false`. Any prior
[`transform_specs()`](../TransformSpec.md) stages are untouched.

## Errors

- `Status::invalid_argument` — `model` is null.

## Practical Examples

### Example 1

Attaching a classifier built by hand via
[`make_classifier`](../../estimators/make_classifier.md), rather than through
`Flow`'s own [`set_model`](set_model.md).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> model_or =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});

  cls::Flow flow{};
  const cls::Status s = flow.set_classifier(model_or.move_value());
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";
  std::cout << "has_model_spec() = " << flow.has_model_spec() << "\n";
  std::cout << "task_kind() == Classification: " << (flow.task_kind() == cls::TaskKind::Classification) << "\n";

  return 0;
}
```

Output

```text
ok = true
has_model() = true
has_model_spec() = false
task_kind() == Classification: true
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
  const cls::Status s = flow.set_classifier(nullptr);
  std::cout << "ok = " << std::boolalpha << s.ok() << "\n";
  std::cout << "message = \"" << s.message << "\"\n";
  std::cout << "has_model() = " << flow.has_model() << "\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow::set_classifier requires a non-null model."
has_model() = false
```

### Example 3

Replacing a `set_model`-built classifier: `has_model_spec()` drops to
`false` since there is no `ModelSpec` behind a hand-built model, while
`has_model()` stays `true` throughout.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow =
      cls::make_flow(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear}).move_value();
  std::cout << "has_model_spec() after make_flow() = " << std::boolalpha << flow.has_model_spec() << "\n";

  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> replacement =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  flow.set_classifier(replacement.move_value());
  std::cout << "has_model_spec() after set_classifier() = " << flow.has_model_spec() << "\n";
  std::cout << "has_model() after set_classifier() = " << flow.has_model() << "\n";

  return 0;
}
```

Output

```text
has_model_spec() after make_flow() = true
has_model_spec() after set_classifier() = false
has_model() after set_classifier() = true
```

## See Also

- [`set_regressor`](set_regressor.md) — the mutually-exclusive counterpart.
- [`set_model`](set_model.md) — build from a `ModelSpec` instead of an
  already-built model.
- [`clear_model`](clear_model.md) — remove whichever of the two is set.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_blueprint.html">
    <span class="label">Previous</span>
    <span class="title">save_blueprint</span>
  </a>
  <a class="next-link" href="set_model.html">
    <span class="label">Next</span>
    <span class="title">set_model</span>
  </a>
</div>
<!-- apiref:nav:end -->
