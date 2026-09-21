<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d7ee95b9c53c -->

# `pluss::ai::classic::Flow::load_model_state`

`load_model_state` restores the attached classifier's or regressor's fitted
parameters from a [`dump_model_state()`](dump_model_state.md) result — the
model half of reconstructing a `Flow`, paired with
[`load_transform_states`](load_transform_states.md) for the stages.

## Signature

```cpp
classic::Status classic::Flow::load_model_state(
    /* a range of pluss::Series -- pass a std::vector<Series> or braced list */);
```

## Parameters

`state` — a range of `pluss::Series`
: The model's own state — ordinarily a `std::vector<Series>` from a prior
  [`dump_model_state()`](dump_model_state.md) call. Empty is the correct
  value for a transform-only `Flow` (see Returns).

## Returns

A `Status`. `.ok()` on success. **Does not set `trained()`** — call
[`mark_trained(true)`](mark_trained.md) once every stage and the model have
been restored, as [`load_transform_states`](load_transform_states.md#returns)
also notes.

## Errors

- `Status::invalid_argument`, `"Flow::load_model_state received model state
  for a transform-only flow."` — `state` is non-empty but this `Flow` has no
  model attached (`has_model() == false`). A transform-only `Flow` should be
  called with an empty `state`, which succeeds trivially.
- `Status::invalid_argument`, `"Flow::load_model_state requires non-empty
  model state for model flows."` — `state` is empty but a model *is*
  attached; an empty dump can only have come from a transform-only flow, so
  loading it onto a model-backed one is a mismatch worth rejecting rather
  than silently leaving the model unfit.
- Whatever the model's own `load_state` rejects otherwise — see
  [`EstimatorBase::load_state`](../../estimators/EstimatorBase/load_state.md#errors).

## Practical Examples

### Example 1

The mismatch `load_model_state` exists to catch: dumping a transform-only
flow's (empty) model state, then loading it onto a model-backed one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow transform_only;
  transform_only.add_standard_scaler();
  cls::FeatureBatch batch{};
  batch.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  transform_only.learn_map(batch);
  const std::vector<pluss::Series> empty_model_state = transform_only.dump_model_state().value();
  std::cout << "empty dump size = " << empty_model_state.size() << "\n";

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow with_model = made.move_value();
  const cls::Status loaded = with_model.load_model_state(empty_model_state);
  std::cout << "load onto model flow ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.message << "\"\n";

  return 0;
}
```

Output

```text
empty dump size = 0
load onto model flow ok = false
message = "Flow::load_model_state requires non-empty model state for model flows."
```

### Example 2

The correct pairing: a transform-only flow's empty state loads onto another
transform-only flow without error.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow source;
  source.add_standard_scaler();
  const std::vector<pluss::Series> empty_state = source.dump_model_state().value();

  cls::Flow target;
  target.add_standard_scaler();
  const cls::Status loaded = target.load_model_state(empty_state);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";

  return 0;
}
```

Output

```text
ok = true
```

### Example 3

The ordinary successful case: restoring a real fitted model's state onto a
fresh `Flow` built from the same `ModelSpec` — see
[`dump_model_state`'s Example 2](dump_model_state.md#example-2) for this same
round trip with matching predictions verified afterward.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::ModelSpec spec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}};
  cls::Flow original = cls::make_flow(spec).move_value();
  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0}, {3}, f64);
  original.learn(batch);
  const std::vector<pluss::Series> state = original.dump_model_state().value();

  cls::Flow restored = cls::make_flow(spec).move_value();
  const cls::Status loaded = restored.load_model_state(state);
  std::cout << "load ok = " << std::boolalpha << loaded.ok() << "\n";

  return 0;
}
```

Output

```text
load ok = true
```

## See Also

- [`dump_model_state`](dump_model_state.md) — the producing call.
- [`load_transform_states`](load_transform_states.md) — the stage half.
- [`mark_trained`](mark_trained.md) — the required last step.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_blueprint.html">
    <span class="label">Previous</span>
    <span class="title">load_blueprint</span>
  </a>
  <a class="next-link" href="load_transform_states.html">
    <span class="label">Next</span>
    <span class="title">load_transform_states</span>
  </a>
</div>
<!-- apiref:nav:end -->
