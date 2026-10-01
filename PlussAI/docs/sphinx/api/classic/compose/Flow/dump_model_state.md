<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 020138db4d6b -->

# `pluss::ai::classic::Flow::dump_model_state`

`dump_model_state` returns the attached classifier's or regressor's own
[`EstimatorBase::dump_state()`](../../estimators/EstimatorBase/dump_state.md)
result directly — the model half of persisting a `Flow`, paired with
[`load_model_state`](load_model_state.md). A transform-only `Flow` (no model)
has nothing to dump here; see Returns.

## Signature

```cpp
[[nodiscard]] classic::StatusOr<std::vector<pluss::Series>> classic::Flow::dump_model_state() const;
```

## What It Does

Passes straight through to the attached model's own
[`EstimatorBase::dump_state()`](../../estimators/EstimatorBase/dump_state.md)
and returns its result unchanged -- the model half of persisting a `Flow`,
paired with [`load_model_state`](load_model_state.md). A transform-only
`Flow` with no model attached returns an empty vector rather than an
error, since there is genuinely nothing to persist.

## Parameters

None.

## Returns

A `StatusOr<std::vector<Series>>` — the model's fitted parameters, exactly as
that model family's own `dump_state()` would return them. **A `Flow` with no
model attached (`has_model() == false`) returns an empty vector, not an
error** — this is not a failure, it is the correct "nothing to persist" answer
for a transform-only pipeline.

## Errors

Whatever the attached model's own `dump_state()` can fail with — see
[`EstimatorBase::dump_state`](../../estimators/EstimatorBase/dump_state.md#errors).
No `Flow`-level validation of its own; this is a direct pass-through.

## Practical Examples

### Example 1

A model-backed flow's state dumped, and a transform-only flow's empty dump
side by side.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow with_model = made.move_value();
  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64);
  with_model.learn(batch);
  std::cout << "with model:    dumped tensors = " << with_model.dump_model_state().value().size() << "\n";

  cls::Flow transform_only;
  transform_only.add_standard_scaler();
  transform_only.learn_map(batch.features);
  std::cout << "transform-only: dumped tensors = " << transform_only.dump_model_state().value().size() << "\n";

  return 0;
}
```

Output

```text
with model:    dumped tensors = 2
transform-only: dumped tensors = 0
```

### Example 2

The full round trip: dump a fitted model's state, restore it onto a fresh
`Flow` built from the same `ModelSpec`, and confirm identical predictions.

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
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64);
  original.learn(batch);
  const std::vector<pluss::Series> state = original.dump_model_state().value();

  cls::Flow restored = cls::make_flow(spec).move_value();
  restored.load_model_state(state);
  restored.mark_trained(true);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0}, {1}, f64).reshape({1, 1});
  std::cout << "original predicts = " << original.infer(probe).value().item<double>() << "\n";
  std::cout << "restored predicts = " << restored.infer(probe).value().item<double>() << "\n";

  return 0;
}
```

Output

```text
original predicts = 10
restored predicts = 10
```

### Example 3

Dumping an attached-but-unfit model's state — the family still returns a
state list rather than an error, the same honest behavior
[`EstimatorBase::dump_state`](../../estimators/EstimatorBase/dump_state.md#errors)
documents for its own untrained case.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});
  cls::Flow flow = made.move_value();  // has a model, never learned

  const cls::StatusOr<std::vector<pluss::Series>> state = flow.dump_model_state();
  std::cout << "trained() = " << std::boolalpha << flow.trained() << "\n";
  std::cout << "dump ok   = " << state.ok() << "\n";
  std::cout << "tensors   = " << state.value().size() << "\n";

  return 0;
}
```

Output

```text
trained() = false
dump ok   = true
tensors   = 2
```

## See Also

- [`load_model_state`](load_model_state.md) — the restoring half.
- [`dump_transform_states`](dump_transform_states.md) — the stages' own
  state, dumped separately.
- [`EstimatorBase::dump_state`](../../estimators/EstimatorBase/dump_state.md) —
  the contract this delegates to.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clone.html">
    <span class="label">Previous</span>
    <span class="title">clone</span>
  </a>
  <a class="next-link" href="dump_transform_states.html">
    <span class="label">Next</span>
    <span class="title">dump_transform_states</span>
  </a>
</div>
<!-- apiref:nav:end -->
