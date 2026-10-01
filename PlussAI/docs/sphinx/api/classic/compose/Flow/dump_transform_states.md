<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 134cc5403c3b -->

# `pluss::ai::classic::Flow::dump_transform_states`

`dump_transform_states` collects every stage's own
[`TransformerBase::dump_state()`](../../estimators/TransformerBase.md) into
one ordered list — one fitted-parameter bundle per stage, in stage order.
Paired with [`load_transform_states`](load_transform_states.md), it is the
stage half of persisting a `Flow`; [`dump_model_state`](dump_model_state.md) /
[`load_model_state`](load_model_state.md) are the model half.

## Signature

```cpp
[[nodiscard]] classic::StatusOr<std::vector<std::vector<pluss::Series>>>
classic::Flow::dump_transform_states() const;
```

## What It Does

Calls `dump_state()` on every transform stage, in stage order, and
collects the results into one nested vector -- the fitted state of the
entire preprocessing pipeline, ready to feed back into
[`load_transform_states`](load_transform_states.md) later. This is the
stage half of persisting a `Flow`; [`dump_model_state`](dump_model_state.md)
covers the attached model separately.

## Parameters

None.

## Returns

A `StatusOr<std::vector<std::vector<Series>>>` — the outer vector has one
entry per stage, in the same order `add_*`/`add_named_transform` added them;
each inner `std::vector<Series>` is that stage's own `dump_state()` result,
untouched. A `Flow` with no stages returns an empty outer vector.

## Errors

- `Status::not_implemented` if any stage's own `dump_state()` fails — most
  stages implement real persistence (see
  [`scaling/Introduction`](../../prep/scaling/Introduction.md#persistence)
  for the scaler family's shape), but a stage relying on
  `TransformerBase`'s base-class default surfaces that failure here,
  prefixed with which stage index it was.
- `Status::internal` if the `Flow` somehow holds a null transformer pointer
  (not reachable through the public API; guards against a corrupted `Flow`).

## Practical Examples

### Example 1

A two-stage flow's states dumped, then round-tripped through
[`load_transform_states`](load_transform_states.md) onto a freshly
constructed `Flow` with the same stage shape.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow original;  // transform-only
  original.add_standard_scaler({}, {0});
  original.add_one_hot_encoder({}, {1});

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 0.0, 20.0, 1.0, 30.0, 0.0}, {6}, f64).reshape({3, 2});
  original.learn_map(train);

  const std::vector<std::vector<pluss::Series>> states = original.dump_transform_states().value();
  std::cout << "stage count in dump = " << states.size() << "\n";
  std::cout << "stage 0 tensors     = " << states[0].size() << "\n";
  std::cout << "stage 1 tensors     = " << states[1].size() << "\n";

  return 0;
}
```

Output

```text
stage count in dump = 2
stage 0 tensors     = 3
stage 1 tensors     = 5
```

### Example 2

A `Flow` with no stages at all dumps an empty outer vector — not an error.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;  // no stages
  const cls::StatusOr<std::vector<std::vector<pluss::Series>>> states = flow.dump_transform_states();
  std::cout << "ok    = " << std::boolalpha << states.ok() << "\n";
  std::cout << "stages = " << states.value().size() << "\n";

  return 0;
}
```

Output

```text
ok    = true
stages = 0
```

### Example 3

Re-fitting a stage on different data changes what the next dump captures —
the dump always reflects the *most recent* `learn`, not the first one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow;
  flow.add_standard_scaler();

  cls::FeatureBatch first{};
  first.x = pluss::make_series({0.0, 10.0}, {2}, f64).reshape({2, 1});
  flow.learn_map(first);
  const pluss::Series first_center = flow.dump_transform_states().value()[0][1];

  cls::FeatureBatch second{};
  second.x = pluss::make_series({100.0, 200.0}, {2}, f64).reshape({2, 1});
  flow.learn_map(second);  // re-fits from scratch
  const pluss::Series second_center = flow.dump_transform_states().value()[0][1];

  std::cout << "center after first learn  = " << first_center << "\n";
  std::cout << "center after second learn = " << second_center << "\n";

  return 0;
}
```

Output

```text
center after first learn  = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[5]
center after second learn = pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[150]
```

## See Also

- [`load_transform_states`](load_transform_states.md) — the restoring half.
- [`dump_model_state`](dump_model_state.md) / [`load_model_state`](load_model_state.md) —
  the model's own state, dumped separately.
- [`save_blueprint`](save_blueprint.md) — the *structure* (stage kinds,
  routes, options), as distinct from fitted parameters.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dump_model_state.html">
    <span class="label">Previous</span>
    <span class="title">dump_model_state</span>
  </a>
  <a class="next-link" href="flow_summary.html">
    <span class="label">Next</span>
    <span class="title">flow_summary</span>
  </a>
</div>
<!-- apiref:nav:end -->
