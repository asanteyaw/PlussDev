<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000a9b7625cd -->

# `pluss::ai::classic::Flow::clone`

`clone` returns a fully independent deep copy of this `Flow` — every stage
rebuilt from its own `TransformSpec`, the model rebuilt from its own
`ModelSpec`, and (if this `Flow` is already fit) every fitted parameter
copied over too, so the clone is immediately usable without a second
`learn`. It exists because `Flow` itself is move-only (see
[`Flow`](../Flow.md)) — `clone` is the explicit way to get a second,
independent copy when you actually want one (branching a fit pipeline to try
two different downstream tweaks, for instance).

## Signature

```cpp
[[nodiscard]] classic::StatusOr<classic::Flow> classic::Flow::clone() const;
```

## What It Does

Rebuilds every stage from its own `TransformSpec` and the model from its
own `ModelSpec`, then -- if this `Flow` is already trained -- copies every
fitted parameter across too, producing a fully independent `Flow` that
shares no state with the original and is immediately usable without a
second `learn`. This exists specifically because `Flow` is move-only:
`clone` is the explicit way to get a genuine second copy (to branch a
fitted pipeline and try two different downstream tweaks, for instance).

## Parameters

None.

## Returns

A `StatusOr<Flow>` — a new `Flow`, sharing no state with the original.
Mutating the clone (adding a stage, calling `set_nested_param`, re-fitting)
never affects the original, and vice versa.

- If this `Flow` is trained, the clone is trained too, with byte-identical
  fitted parameters (stages restored via
  [`dump_transform_states`](dump_transform_states.md)/[`load_transform_states`](load_transform_states.md),
  the model via [`dump_model_state`](dump_model_state.md)/[`load_model_state`](load_model_state.md)).
- If this `Flow` is not yet trained, the clone has the same stage/model
  *structure* but is equally untrained.

## Errors

- `Status::invalid_argument`, `"Flow::clone does not support opaque
  append_transform stages."` — a stage was added via
  [`append_transform`](append_transform.md), which has no declarative
  `TransformSpec` to rebuild from — the same restriction
  [`save_blueprint`](save_blueprint.md) has, for the same reason.
- `Status::invalid_argument`, `"Flow::clone requires a ModelSpec for
  model-backed flows."` — a model was attached via
  [`set_classifier`](set_classifier.md)/[`set_regressor`](set_regressor.md)
  directly (a hand-built estimator, not built from a `ModelSpec`) rather than
  `set_model`/`make_flow` — there is no spec to rebuild the model from.
- Any failure rebuilding a stage, rebuilding the model, or restoring fitted
  state propagates with a message naming which step failed
  (`"...failed to rebuild transform stage: ..."`,
  `"...failed to rebuild model: ..."`,
  `"...failed to load transform states: ..."`,
  `"...failed to load model state: ..."`).

## Practical Examples

### Example 1

Cloning a fitted flow, then reconfiguring and re-fitting the clone with a
stronger regularization — the original keeps its own separate fit and
prediction throughout.

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
  cls::Flow original = made.move_value();
  original.add_standard_scaler();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64);
  original.learn(batch);

  cls::Flow clone = original.clone().move_value();

  // Reconfigure and re-fit the clone only -- set_nested_param on model::* resets
  // trained() (see set_nested_param's own page), so the clone needs a fresh learn().
  clone.set_nested_param("model::opt.regularization_strength", 1e3);
  clone.learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0}, {1}, f64).reshape({1, 1});
  std::cout << "original.trained() = " << std::boolalpha << original.trained() << "\n";
  std::cout << "clone.trained()    = " << clone.trained() << "\n";
  std::cout << "original predicts  = " << original.infer(probe).value().item<double>() << "\n";
  std::cout << "clone predicts     = " << clone.infer(probe).value().item<double>() << "\n";

  return 0;
}
```

Output

```text
original.trained() = true
clone.trained()    = true
original predicts  = 10
clone predicts     = 5.01992
```

### Example 2

`append_transform`'s opaque stage blocks `clone`, the same way it blocks
`save_blueprint`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <memory>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.append_transform(std::make_unique<cls::prep::StandardScaler>());
  const cls::StatusOr<cls::Flow> cloned = flow.clone();
  std::cout << "ok = " << std::boolalpha << cloned.ok() << "\n";
  std::cout << "message = \"" << cloned.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow::clone does not support opaque append_transform stages."
```

### Example 3

Cloning an unfit `Flow` copies structure only, as expected — the clone is
equally untrained, and fitting one afterward does not fit the other.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow original;  // transform-only, never fit
  original.add_standard_scaler();
  std::cout << "original.trained() before clone = " << std::boolalpha << original.trained() << "\n";

  cls::Flow clone = original.clone().move_value();
  std::cout << "clone.trained()    = " << clone.trained() << "\n";

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  clone.learn_map(train);
  std::cout << "clone.trained() after fitting the clone = " << clone.trained() << "\n";
  std::cout << "original.trained() (untouched)           = " << original.trained() << "\n";

  return 0;
}
```

Output

```text
original.trained() before clone = false
clone.trained()    = false
clone.trained() after fitting the clone = true
original.trained() (untouched)           = false
```

## See Also

- [`save_blueprint`](save_blueprint.md) — a text serialization of structure
  alone, versus `clone`'s in-memory, fully-fitted copy.
- [`append_transform`](append_transform.md) — the one stage kind `clone`
  (and `save_blueprint`) cannot handle.
- [`has_opaque_transforms`](../Flow.md#state) — check before cloning if
  unsure.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_transforms.html">
    <span class="label">Previous</span>
    <span class="title">clear_transforms</span>
  </a>
  <a class="next-link" href="dump_model_state.html">
    <span class="label">Next</span>
    <span class="title">dump_model_state</span>
  </a>
</div>
<!-- apiref:nav:end -->
