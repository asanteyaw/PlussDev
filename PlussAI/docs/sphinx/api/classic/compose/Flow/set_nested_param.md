<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 36b126e63e02 -->

# `pluss::ai::classic::Flow::set_nested_param`

`set_nested_param` writes one configuration value by its dotted path — the
write half of [`nested_param`](nested_param.md). It is the mechanism a
generic hyperparameter search can use to mutate an arbitrary `Flow` (a
stage's option, or the model's) without knowing its concrete family or stage
type ahead of time.

## Signature

```cpp
classic::Status classic::Flow::set_nested_param(std::string_view path, double value);
```

## Parameters

`path` — `std::string_view`
: As [`nested_param`](nested_param.md) — `"<stage>::<field>"` or
  `"model::<field>"`. Not every readable path is writable the same way:
  `route_size` and `stage_index` are read-only (there is no field to set —
  route size is derived from `feature_route`'s own length), so only
  `remainder`, `kind`, and `opt.*` accept a write on a stage.

`value` — `double`
: The new value, converted back to the field's real type (a `bool` field
  treats `value >= 0.5` as `true`; an enum field truncates via
  `static_cast`).

## Returns

A `Status`. `.ok()` on success.

**Side effects that differ by what you write:**

- Writing a stage's `opt.*` field or `kind` **rebuilds that stage from
  scratch** (a fresh transformer, its previous fit discarded) and sets
  `trained() = false` — the same invalidation
  [`set_stage_route`](set_stage_route.md#returns)/[`set_stage_remainder`](set_stage_remainder.md#returns)
  cause, since a stage's configuration changed underneath it.
- Writing a stage's `remainder` calls [`set_stage_remainder`](set_stage_remainder.md)
  internally — same invalidation, same contract.
- Writing `model::*` rebuilds the `ModelSpec` and calls
  [`set_model`](set_model.md) with it — which resets `trained()` and clears
  any prior model fit, exactly as calling `set_model` directly would.

## Errors

- The same path-shape and stage-resolution errors as
  [`nested_param`](nested_param.md#errors).
- `Status::invalid_argument` for `route_size` / `stage_index` (read-only) or
  any field name `nested_param_keys()` would not have listed.
- `Status::precondition` for `model::*` with no `ModelSpec` attached — as
  `nested_param`.

## Practical Examples

### Example 1

Sweeping a model hyperparameter by path, without referencing
`LinearFamilyOptions` directly — the mechanism a generic tuner relies on.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LinearFamilyOptions linear{};
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();

  const cls::Series x = pluss::make_series(
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {10}, f64).reshape({10, 1});
  const cls::Series y = pluss::make_series(
      {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0}, {10}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0}, {1}, f64).reshape({1, 1});

  auto fit_and_predict = [&](double strength) {
    flow.set_nested_param("model::opt.regularization_strength", strength);
    flow.learn(batch);
    return flow.infer(probe).value().item<double>();
  };

  std::cout << "strength=1e-6 prediction : " << fit_and_predict(1e-6) << "\n";
  std::cout << "strength=1e3  prediction : " << fit_and_predict(1e3) << "\n";

  return 0;
}
```

Output

```text
strength=1e-6 prediction : 100
strength=1e3  prediction : 58.4296
```

### Example 2

Writing a stage's option field invalidates the flow — `trained()` drops back
to `false` even though only the scaler, not the model, changed.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;  // transform-only
  flow.add_standard_scaler();
  flow.set_stage_key(0, "scale");

  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  flow.learn_map(batch);
  std::cout << "trained() after learn_map        = " << std::boolalpha << flow.trained() << "\n";

  const cls::Status set = flow.set_nested_param("scale::opt.with_centering", 0.0);  // false
  std::cout << "set_nested_param ok               = " << set.ok() << "\n";
  std::cout << "trained() after set_nested_param  = " << flow.trained() << "\n";

  return 0;
}
```

Output

```text
trained() after learn_map        = true
set_nested_param ok               = true
trained() after set_nested_param  = false
```

### Example 3

`route_size` is readable but not writable — it is derived from
`feature_route`'s own length, not a field of its own to set.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler({}, {0});
  flow.set_stage_key(0, "scale");

  const cls::Status set = flow.set_nested_param("scale::route_size", 5.0);
  std::cout << "ok = " << std::boolalpha << set.ok() << "\n";
  std::cout << "message = \"" << set.message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "set_nested_param supports model::* and '<stage>::remainder|kind|opt.*'."
```

## See Also

- [`nested_param`](nested_param.md) / [`nested_param_keys`](nested_param_keys.md) —
  the read half and the path list.
- [`set_model`](set_model.md) — what a `model::*` write ultimately calls.
- [`set_stage_remainder`](set_stage_remainder.md) — what a stage's
  `remainder` write ultimately calls.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_model.html">
    <span class="label">Previous</span>
    <span class="title">set_model</span>
  </a>
  <a class="next-link" href="set_regressor.html">
    <span class="label">Next</span>
    <span class="title">set_regressor</span>
  </a>
</div>
<!-- apiref:nav:end -->
