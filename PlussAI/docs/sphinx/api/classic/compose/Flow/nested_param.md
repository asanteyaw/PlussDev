<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8634db6a2bc6 -->

# `pluss::ai::classic::Flow::nested_param`

`nested_param` reads one configuration value by its dotted path — a stage's
route metadata or option field, or the attached model's task/family/option
field — as a single `double`, regardless of the field's real type (`bool`,
an enum, an `int64_t`). It is the generic read half of the same mechanism
[`nested_param_keys`](nested_param_keys.md) enumerates and
[`set_nested_param`](set_nested_param.md) writes through.

## Signature

```cpp
[[nodiscard]] classic::StatusOr<double> classic::Flow::nested_param(std::string_view path) const;
```

## Parameters

`path` — `std::string_view`
: A key from [`nested_param_keys()`](nested_param_keys.md): `"<stage>::<field>"`
  or `"model::<field>"`. Every enum and `bool` field reads back as the
  underlying integer value (a `bool` as `0.0`/`1.0`, an enum as its
  `static_cast<std::uint8_t>`) — `nested_param` has no notion of the field's
  real C++ type, only that it fits in a `double`.

## Returns

A `StatusOr<double>` — the field's current value.

## Errors

- `Status::invalid_argument`, `"nested_param path must be '<stage>::<param>'."`
  — the path is not exactly three `:`-delimited parts with an empty middle
  part (i.e. not shaped `"a::b"`).
- `Status::invalid_argument`, `"nested_param could not resolve stage key."`
  — `<stage>` names neither an existing stage key nor a valid positional
  fallback.
- `Status::precondition`, `"nested_param model::* requires Flow configured
  via set_model(ModelSpec)."` — `<stage>` is `"model"` but no `ModelSpec` was
  ever attached (a `Flow` built with `set_classifier`/`set_regressor` rather
  than `set_model`/`make_flow` has a live model but no `ModelSpec` to read
  fields from).
- `Status::invalid_argument` for a field name `nested_param_keys()` would not
  have listed for this stage/model.

## Practical Examples

### Example 1

Reading a stage's route metadata and option field, and the model's
hyperparameter, all through the same call.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  linear.regularization_strength = 0.5;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler({}, {0});
  flow.set_stage_key(0, "scale");

  std::cout << "scale::route_size          = " << flow.nested_param("scale::route_size").value() << "\n";
  std::cout << "scale::opt.variance_floor  = " << flow.nested_param("scale::opt.variance_floor").value() << "\n";
  std::cout << "model::opt.regularization_strength = "
            << flow.nested_param("model::opt.regularization_strength").value() << "\n";

  return 0;
}
```

Output

```text
scale::route_size          = 1
scale::opt.variance_floor  = 1e-12
model::opt.regularization_strength = 0.5
```

### Example 2

A malformed path and an unresolvable stage key.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler();

  const cls::StatusOr<double> bad_shape = flow.nested_param("not_shaped_right");
  std::cout << "bad shape: ok = " << std::boolalpha << bad_shape.ok()
            << ", message = \"" << bad_shape.status().message << "\"\n";

  const cls::StatusOr<double> bad_stage = flow.nested_param("nonexistent::route_size");
  std::cout << "bad stage: ok = " << bad_stage.ok()
            << ", message = \"" << bad_stage.status().message << "\"\n";

  return 0;
}
```

Output

```text
bad shape: ok = false, message = "nested_param path must be '<stage>::<param>'."
bad stage: ok = false, message = "nested_param could not resolve stage key."
```

### Example 3

`model::*` on a `Flow` whose model was attached via
[`set_classifier`](set_classifier.md) directly (a hand-built estimator, not
a `ModelSpec`) — there is no spec to read a field from.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made = cls::make_classifier(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear,
                     cls::LinearFamilyOptions{.objective = cls::LinearObjective::Logistic}});

  cls::Flow flow;
  flow.set_classifier(made.move_value());
  const cls::StatusOr<double> result = flow.nested_param("model::task");
  std::cout << "ok = " << std::boolalpha << result.ok() << "\n";
  std::cout << "message = \"" << result.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "nested_param model::* requires Flow configured via set_model(ModelSpec)."
```

## See Also

- [`nested_param_keys`](nested_param_keys.md) — every valid `path` for a
  given `Flow`.
- [`set_nested_param`](set_nested_param.md) — the write half.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mark_trained.html">
    <span class="label">Previous</span>
    <span class="title">mark_trained</span>
  </a>
  <a class="next-link" href="nested_param_keys.html">
    <span class="label">Next</span>
    <span class="title">nested_param_keys</span>
  </a>
</div>
<!-- apiref:nav:end -->
