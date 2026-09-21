<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a02f70facba9 -->
# `pluss::ai::classic::EstimatorBase::load_state`

`load_state` restores a fitted state — previously captured with
[`dump_state`](dump_state.md) on an estimator of the same family and options —
onto a fresh, untrained estimator, without re-running `learn`.

## Signature

```cpp
classic::Status classic::EstimatorBase::load_state(
    /* a range of pluss::Series -- pass a std::vector<Series> or braced list */);
```

## Parameters

`state`
: The tensor list a matching [`dump_state`](dump_state.md) call produced.
  Must come from the *same* family and (for families whose shape depends on
  it) the same options — loading a `Tree`-family dump into a `Linear`
  estimator, or a state list with the wrong tensor count, fails.

## Returns

A `Status`. On success, the receiving estimator's `trained()` becomes `true`
and it predicts identically to the estimator `dump_state` was called on.

## Errors

- `Status::not_implemented` — the base class default, for any family that has
  not overridden it (see [`dump_state`](dump_state.md)'s Errors section — the
  same caveat applies here).
- `Status::invalid_argument` — the state list has the wrong tensor count or
  otherwise does not match what this family's `load_state` expects (loading a
  different family's dump, or a hand-built list that does not match the
  contract).

## Practical Examples

### Example 1

The full round trip — see also [`dump_state`'s Example 2](dump_state.md#example-2)
for the same pattern from the dumping side.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const cls::Series x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4}, f64).reshape({4, 1});
  const cls::Series y = pluss::make_series({2.0, 4.0, 6.0, 8.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  const cls::ModelSpec spec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}};
  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> original = cls::make_regressor(spec);
  original.value()->learn(batch);
  const std::vector<pluss::Series> state = original.value()->dump_state().value();

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> restored = cls::make_regressor(spec);
  const cls::Status loaded = restored.value()->load_state(state);
  std::cout << "load_state ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "restored.trained() = " << restored.value()->trained() << "\n";

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({5.0}, {1}, f64).reshape({1, 1});
  std::cout << "original prediction = " << original.value()->infer(probe).value().item<double>() << "\n";
  std::cout << "restored prediction = " << restored.value()->infer(probe).value().item<double>() << "\n";

  return 0;
}
```

Output

```text
load_state ok = true
restored.trained() = true
original prediction = 10
restored prediction = 10
```

### Example 2

Loading a state with the wrong tensor count — the receiving family validates
the shape it is given rather than crashing on garbage.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  const std::vector<pluss::Series> empty_state{};
  const cls::Status loaded = made.value()->load_state(empty_state);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.message << "\"\n";
  std::cout << "trained() after the failed load = " << made.value()->trained() << "\n";

  return 0;
}
```

Output

```text
ok = false
message = "LinearRegressor::load_state expects 2 tensors."
trained() after the failed load = false
```

### Example 3

A second family's own shape validation — every concrete family checked for
this reference (including `TransformerBase` families like the k-means
transformer here) overrides `load_state` with a real implementation rather
than falling back to the base class's `Status::not_implemented`, and each
validates its own expected tensor count independently.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;

  cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made =
      cls::make_transformer(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster,
                                           cls::ClusterFamilyOptions{}});

  const std::vector<pluss::Series> state{};
  const cls::Status loaded = made.value()->load_state(state);
  std::cout << "ok = " << std::boolalpha << loaded.ok() << "\n";
  std::cout << "message = \"" << loaded.message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "KMeansTransformer::load_state expects 2 tensors."
```

## See Also

- [`dump_state`](dump_state.md) — the capturing half of the round trip.
- [`io::load`](../../io/load.md) — the higher-level persistence built on this
  pair.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="learn.html">
    <span class="label">Previous</span>
    <span class="title">learn</span>
  </a>
  <a class="next-link" href="task_kind.html">
    <span class="label">Next</span>
    <span class="title">task_kind</span>
  </a>
</div>
<!-- apiref:nav:end -->
