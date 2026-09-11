# `pluss::ai::classic::RandomState`

`RandomState` is the seed-and-determinism pair every stochastic `classic`
routine takes — shuffling folds, sampling rows for balancing or the isolation
forest, initializing a tuning sampler. It is small on purpose: two fields, no
per-call RNG object to manage.

`RandomState` is not itself a random-number generator. It is a *recipe* a
routine uses to build one internally, fresh, on each call — so the same
`RandomState` value given to the same call always produces the same draw
sequence, and different `RandomState` values (or `deterministic = false`)
produce different ones.

## Fields

### `seed`

`std::uint64_t seed = 0`

The seed value. Only consulted when `deterministic` is `true`. Two calls with
the same `seed` (and the same `deterministic = true`) draw the identical
sequence of random choices, regardless of what else changed about the call —
this is what makes a `TrainOptions::random` or a `SplitOptions::random` value
reproducible across runs, machines, and rebuilds.

### `deterministic`

`bool deterministic = true`

`true` (the default): the routine seeds its RNG from `seed`, so results are
reproducible. `false`: the routine seeds from a real entropy source instead
and `seed` is ignored — every call draws a different sequence, including two
calls in the same process with identical `RandomState{}` values otherwise.

## Guidance

Leave `RandomState{}` (`seed = 0`, `deterministic = true`) alone for anything
you will compare across runs — a tuning sweep, a benchmark, a test fixture.
Set `deterministic = false` only when you specifically want fresh randomness
each call (an online production system re-sampling live traffic, for
instance) and reproducibility does not matter.

Changing `seed` alone (keeping `deterministic = true`) is how you get a
*different but still reproducible* draw — useful for repeated-CV (see
[`selection`](../../lymph/selection.md) in `lymph`, which threads a `RandomState`
per repeat) or for running the same pipeline under several seeds to check
sensitivity to the split.

## Practical Example

The same seed reproduces the same fold assignment; a different seed (or
`deterministic = false`) does not.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::SplitOptions split{};
  split.plan = cls::CVPlanKind::CrossVal;
  split.folds = 3;
  split.shuffle = true;
  split.random.seed = 7;
  split.random.deterministic = true;

  const cls::StatusOr<cls::SplitPlan> a = cls::crossval_plan(9, split);
  const cls::StatusOr<cls::SplitPlan> b = cls::crossval_plan(9, split);  // same seed
  split.random.seed = 8;
  const cls::StatusOr<cls::SplitPlan> c = cls::crossval_plan(9, split);  // different seed

  auto first_test_row = [](const cls::StatusOr<cls::SplitPlan>& p) {
    return p.ok() ? p.value().folds.front().test_indices.slice(0, 0, 1).item<double>() : -1.0;
  };

  std::cout << "a first test row = " << first_test_row(a) << "\n";
  std::cout << "b first test row = " << first_test_row(b) << "  (same seed as a)\n";
  std::cout << "c first test row = " << first_test_row(c) << "  (different seed)\n";

  return 0;
}
```

Output

```text
PENDING_VERIFICATION_RANDOMSTATE_1
```

## Changelog

- **0.1** — `seed` + `deterministic`.

## See Also

- [`TrainOptions`](TrainOptions.md) — carries a `RandomState` in `.random`.
- [`split`](../split.md) — `SplitOptions::random` controls fold shuffling.
- [`prep::outlier`](../prep/outlier.md) — `OutlierIsolatorOptions::random`
  seeds the isolation forest.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
