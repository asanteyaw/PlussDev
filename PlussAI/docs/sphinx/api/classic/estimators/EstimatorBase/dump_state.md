<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1c72bf553d73 -->
# `pluss::ai::classic::EstimatorBase::dump_state`

`dump_state` captures a trained estimator's fitted parameters as a flat list
of `Series` — the low-level half of persistence that
[`io::save`](../../io/save.md) uses internally. Pair it with
[`load_state`](load_state.md) to restore a freshly-constructed estimator of
the *same* family and options to an identical fitted state, without re-running
`learn`.

## Signature

```cpp
classic::StatusOr<std::vector<pluss::Series>> classic::EstimatorBase::dump_state() const;
```

## Parameters

None.

## Returns

A `StatusOr<std::vector<Series>>`. On success, a family-specific list of
tensors (weights, calibration parameters, category maps, …) — treat its
length and contents as opaque; the only contract is that the same list, fed
back through [`load_state`](load_state.md) on a fresh estimator built from the
same `ModelSpec`, reproduces the original fit.

## Errors

- `Status::not_implemented` — the base class default, returned by any family
  that has not overridden `dump_state`. Every classifier and regressor family
  shipped today overrides it; treat this as a real possibility for a family
  you have not checked rather than assuming it always succeeds.
- Calling this on an untrained estimator returns whatever the family's
  override does with untrained state — check `trained()` first if that
  matters to you.

## Practical Examples

### Example 1

Dumping a trained classifier's state and inspecting the shape of what comes
back.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 2.0, 2.2, 2.4, 2.6}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  made.value()->learn(batch);

  const cls::StatusOr<std::vector<pluss::Series>> dumped = made.value()->dump_state();
  std::cout << "ok = " << std::boolalpha << dumped.ok() << "\n";
  std::cout << "tensor count = " << dumped.value().size() << "\n";

  return 0;
}
```

Output

```text
ok = true
tensor count = 5
```

### Example 2

The full round trip: dump a trained classifier's state, build a *second*,
freshly-constructed classifier from the same spec, load the dumped state into
it, and confirm it predicts identically without ever calling `learn` on it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series(
      {1.0, 1.2, 1.4, 1.6, 2.0, 2.2, 2.4, 2.6}, {8}, f64).reshape({8, 1});
  const cls::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear};

  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> original = cls::make_classifier(spec);
  original.value()->learn(batch);
  const std::vector<pluss::Series> state = original.value()->dump_state().value();

  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> restored = cls::make_classifier(spec);
  std::cout << "restored.trained() before load_state = " << std::boolalpha
            << restored.value()->trained() << "\n";
  const cls::Status loaded = restored.value()->load_state(state);
  std::cout << "load_state ok = " << loaded.ok() << "\n";
  std::cout << "restored.trained() after load_state  = " << restored.value()->trained() << "\n";

  const pluss::Series original_pred = original.value()->infer(batch.features).value();
  const pluss::Series restored_pred = restored.value()->infer(batch.features).value();
  std::cout << "original predictions = " << original_pred << "\n";
  std::cout << "restored predictions = " << restored_pred << "\n";

  return 0;
}
```

Output

```text
restored.trained() before load_state = false
load_state ok = true
restored.trained() after load_state  = true
original predictions = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
restored predictions = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 3

`dump_state` on an untrained estimator — the family still returns a state
list (empty of any real fit); reflected honestly rather than assumed.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});

  const cls::StatusOr<std::vector<pluss::Series>> dumped = made.value()->dump_state();
  std::cout << "trained() = " << std::boolalpha << made.value()->trained() << "\n";
  std::cout << "dump_state ok = " << dumped.ok() << "\n";
  std::cout << "tensor count = " << dumped.value().size() << "\n";

  return 0;
}
```

Output

```text
trained() = false
dump_state ok = true
tensor count = 2
```

## See Also

- [`load_state`](load_state.md) — the restoring half of the round trip.
- [`io::save`](../../io/save.md) / [`io::load`](../../io/load.md) — the
  higher-level persistence built on this pair.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assess.html">
    <span class="label">Previous</span>
    <span class="title">assess</span>
  </a>
  <a class="next-link" href="infer.html">
    <span class="label">Next</span>
    <span class="title">infer</span>
  </a>
</div>
<!-- apiref:nav:end -->
