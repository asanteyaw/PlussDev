<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 48a46937a578 -->

# `pluss::ai::classic::Flow::mark_trained`

`mark_trained` sets `trained()` directly, bypassing `learn`/`learn_map`
entirely. It exists for exactly one situation: reconstructing a `Flow` from
persisted state — stages restored via
[`load_transform_states`](load_transform_states.md), a model restored via
[`load_model_state`](load_model_state.md) — where every fitted parameter is
already in place and nothing needs fitting, only the `trained()` flag needs
to reflect that.

## Signature

```cpp
void classic::Flow::mark_trained(bool trained) noexcept;
```

## Parameters

`trained` — `bool`
: The value `trained()` reports afterward — `true` once a full state
  restoration is complete, or `false` to deliberately invalidate a `Flow`
  without touching its stages or model (rarely useful; `clear_transforms()` /
  `clear_model()` are the ordinary ways to invalidate a fit).

## Returns

Nothing. Never fails — this only writes one `bool`, with no validation of
whether the underlying stages or model are actually in a state consistent
with the value you pass.

## Errors

None. **`mark_trained(true)` does not check that stages or a model actually
hold fitted parameters** — calling it on a freshly-constructed `Flow` with no
prior `load_transform_states`/`load_model_state` call reports `trained() ==
true` while every stage's own `map`/`infer` would still fail with its own
`Status::precondition`. Use it only as the last step of a full state
restoration, never as a shortcut past `learn`.

## Practical Examples

### Example 1

The intended use: restoring a transform-only flow's state, then marking it
trained so `map` accepts it — mirroring what a full
`load_transform_states` + `load_model_state` restoration does for a
model-backed flow.

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
  restored.add_standard_scaler();  // same stage shape, freshly constructed
  std::cout << "trained() before restore = " << std::boolalpha << restored.trained() << "\n";
  restored.load_transform_states(states);
  restored.mark_trained(true);
  std::cout << "trained() after restore  = " << restored.trained() << "\n";

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({2.0}, {1}, f64).reshape({1, 1});
  std::cout << "map ok = " << restored.map(probe).ok() << "\n";

  return 0;
}
```

Output

```text
trained() before restore = false
trained() after restore  = true
map ok = true
```

### Example 2

The misuse the Errors section warns about: `mark_trained(true)` on a `Flow`
whose stage was never actually fit — `trained()` reports `true`, but the
stage's own `map` still fails.

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
  flow.mark_trained(true);  // lying about it -- the scaler was never fit
  std::cout << "trained() = " << std::boolalpha << flow.trained() << "\n";

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0}, {1}, f64).reshape({1, 1});
  const cls::StatusOr<pluss::Series> mapped = flow.map(probe);
  std::cout << "map ok = " << mapped.ok() << "\n";
  std::cout << "message = \"" << mapped.status().message << "\"\n";

  return 0;
}
```

Output

```text
trained() = true
map ok = false
message = "StandardScaler::map requires learn() before map()."
```

### Example 3

`mark_trained(false)` on an otherwise fully-fit, model-backed `Flow` blocks
[`infer`](infer.md) (which checks `trained()` directly), but **not**
[`map`](map.md) — `map` only depends on each stage's own fitted state, never
on `Flow::trained()`, so the already-fit scaler keeps working regardless.

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
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0}, {3}, f64);
  flow.learn(batch);
  std::cout << "trained() after learn        = " << std::boolalpha << flow.trained() << "\n";

  flow.mark_trained(false);
  std::cout << "trained() after mark_trained = " << flow.trained() << "\n";
  std::cout << "infer ok now (blocked)        = " << flow.infer(batch.features).ok() << "\n";
  std::cout << "map ok now (unaffected)       = " << flow.map(batch.features).ok() << "\n";

  return 0;
}
```

Output

```text
trained() after learn        = true
trained() after mark_trained = false
infer ok now (blocked)        = false
map ok now (unaffected)       = true
```

## See Also

- [`load_transform_states`](load_transform_states.md) /
  [`load_model_state`](load_model_state.md) — the restoration calls
  `mark_trained` completes.
- [`learn`](learn.md) — the ordinary way to reach `trained() == true`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="nested_param.html">
    <span class="label">Next</span>
    <span class="title">nested_param</span>
  </a>
</div>
<!-- apiref:nav:end -->
