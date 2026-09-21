<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 15f67a47b731 -->

# `pluss::ai::classic::Flow::load_transform_states`

`load_transform_states` restores every stage's fitted parameters from a
[`dump_transform_states()`](dump_transform_states.md) result — the stage half
of reconstructing a `Flow` without re-running `learn`. The target `Flow`
must already have the same stages added (same kinds, same count, same order)
as the one the states were dumped from; only the fitted numbers are
restored, not the stage structure itself — see
[`save_blueprint`](save_blueprint.md) for that.

## Signature

```cpp
classic::Status classic::Flow::load_transform_states(
    /* a range of std::vector<pluss::Series> -- pass a
       std::vector<std::vector<Series>> or braced list */);
```

## Parameters

`states` — a range of `std::vector<pluss::Series>`
: One entry per stage, in stage order — ordinarily a
  `std::vector<std::vector<Series>>` from a prior
  [`dump_transform_states()`](dump_transform_states.md) call, passed
  directly (it converts implicitly).

## Returns

A `Status`. `.ok()` on success. **Does not set `trained()`** — call
[`mark_trained(true)`](mark_trained.md) afterward once every stage (and the
model, if any, via [`load_model_state`](load_model_state.md)) has been
restored.

## Errors

- `Status::shape_mismatch`, `"Flow::load_transform_states requires one state
  per transform stage."` — `states.size()` does not equal the `Flow`'s
  current `stage_count()`. Add the same stages first (matching kinds and
  count), then load.
- `Status::invalid_argument` if a stage's own `load_state` rejects its entry
  (wrong tensor count for that stage's family, for instance) — the message
  names which stage index failed.
- `Status::internal` for a corrupted `Flow` holding a null stage pointer
  (not reachable through the public API).

## Practical Examples

### Example 1

The full stage-state round trip: dump from a fitted flow, restore onto a
freshly built one with the same stage shape, verify identical `map` output.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow original;
  original.add_standard_scaler();
  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  original.learn_map(train);
  const std::vector<std::vector<pluss::Series>> states = original.dump_transform_states().value();

  cls::Flow restored;
  restored.add_standard_scaler();  // same stage, not yet fit
  const cls::Status loaded = restored.load_transform_states(states);
  restored.mark_trained(true);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0}, {1}, f64).reshape({1, 1});
  std::cout << "load ok  = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "original = " << original.map(probe).value() << "\n";
  std::cout << "restored = " << restored.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
load ok  = true
original = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[2.236068]]
restored = pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[2.236068]]
```

### Example 2

A stage-count mismatch — restoring two states onto a `Flow` with only one
stage.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow original;
  original.add_standard_scaler({}, {0});
  original.add_one_hot_encoder({}, {1});
  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 0.0, 20.0, 1.0}, {4}, f64).reshape({2, 2});
  original.learn_map(train);
  const std::vector<std::vector<pluss::Series>> states = original.dump_transform_states().value();

  cls::Flow one_stage;
  one_stage.add_standard_scaler();  // only 1 stage -- states has 2
  const cls::Status loaded = one_stage.load_transform_states(states);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow::load_transform_states requires one state per transform stage."
```

### Example 3

Matching stage *count* is not enough — the stages must match in *kind* and
*order* too, or the per-stage `load_state` call rejects the mismatched
tensors it actually receives.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow original;
  original.add_standard_scaler({}, {0});
  original.add_one_hot_encoder({}, {1});
  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 0.0, 20.0, 1.0}, {4}, f64).reshape({2, 2});
  original.learn_map(train);
  const std::vector<std::vector<pluss::Series>> states = original.dump_transform_states().value();

  cls::Flow swapped;  // same count, kinds reversed
  swapped.add_one_hot_encoder({}, {1});
  swapped.add_standard_scaler({}, {0});
  const cls::Status loaded = swapped.load_transform_states(states);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Flow::load_transform_states failed at stage 0: OneHotEncoder::load_state expects 5 state tensors."
```

## See Also

- [`dump_transform_states`](dump_transform_states.md) — the producing call.
- [`mark_trained`](mark_trained.md) — the required last step.
- [`load_model_state`](load_model_state.md) — the model half of the same
  restoration.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_model_state.html">
    <span class="label">Previous</span>
    <span class="title">load_model_state</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
<!-- apiref:nav:end -->
