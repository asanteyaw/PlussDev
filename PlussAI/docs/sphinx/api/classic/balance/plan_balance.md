# `pluss::ai::classic::plan_balance`

Computes the full resample / synthesize / clean recipe for reaching a
[`BalanceTarget`](BalanceTarget.md)'s desired class mix — **without
moving any data**. [`balance_rows`](balance_rows.md) is this function
plus the one extra step of actually materializing the plan into a new
[`LabeledBatch`](../data/LabeledBatch.md); reach for `plan_balance`
directly when you want to inspect or reuse the recipe itself (which rows
were kept, duplicated, synthesized from, or removed) before committing to
it.

## Signature

```cpp
classic::StatusOr<classic::BalancePlan> classic::plan_balance(
    const pluss::Series& labels,
    const classic::BalanceTarget& target,
    classic::RandomState random = {});

classic::StatusOr<classic::BalancePlan> classic::plan_balance(
    const classic::LabeledBatch& batch,
    const classic::BalanceTarget& target,
    classic::RandomState random = {});
```

## Parameters

`labels` (`Series`-only overload)
: Passed to [`class_mix`](class_mix.md) — same validity contract. This
  overload has **no feature data at all**, so it only supports plain
  index resampling: `target.clean` must be `None` and `target.growth`
  must be `ReuseRows` — either one set otherwise is rejected outright,
  naming the `LabeledBatch` overload as the fix. See
  [Example 5](#example-5).

`batch` (`LabeledBatch` overload)
: Must pass [`validate_labeled`](../data/LabeledBatch.md). **When
  `target.clean == None` and (`target.growth == ReuseRows` or
  `target.kind == Keep`), this overload simply calls the `Series`-only
  overload on `batch.y` internally** — the two are byte-identical for
  every "simple" `BalanceTarget`, confirmed directly in
  [Example 5](#example-5). Only a `target` that actually needs feature
  data (`clean != None`, or `growth == BlendNeighbors` with real work to
  do) runs the richer path this overload alone provides.

`target` (`BalanceTarget`)
: See [`BalanceTarget`](BalanceTarget.md) for every field. `kind`
  resolves the per-class target count; `growth` decides how extra rows
  are produced; `clean` optionally removes rows first.

`random` (`RandomState`, default `{}`)
: Seeds every random draw this function makes — which rows survive
  downsampling, which duplicate rows are picked for `ReuseRows` growth,
  and which base/neighbor/interpolation-step triples are picked for
  `BlendNeighbors` synthesis. Same `deterministic` contract as every
  other [`RandomState`](../core/RandomState.md) consumer.

## Returns

A `StatusOr<BalancePlan>` — see [`BalancePlan`](BalancePlan.md) for
every field. `before` always reflects the **original**, pre-cleaning
class distribution; when `target.clean != None`, the target counts
`plan_balance` actually resolves against are computed from the
**post-cleaning** distribution instead — the two can disagree. See
[Example 11](#example-11).

## Errors

- Everything [`class_mix`](class_mix.md#errors) can already fail with,
  applied to the labels.
- `Series`-only overload: `target.clean != None`, or `target.growth ==
  BlendNeighbors` — `Status::invalid_argument` naming the `LabeledBatch`
  overload.
- `target.kind == Ratio` with a class count other than `2` —
  `Status::invalid_argument`.
- `target.kind == Ratio` with `target.ratio` non-finite or outside
  `(0, 1]` — `Status::invalid_argument`.
- `target.kind == Counts` with `target.counts` undefined, a length not
  matching the class count, or a value non-finite or `< 1` —
  `Status::invalid_argument` / `Status::shape_mismatch`.
- `LabeledBatch` overload: `target.growth == BlendNeighbors` with
  `target.neighbors <= 0` — `Status::invalid_argument`.
- `LabeledBatch` overload: `target.clean != None` with
  `target.clean_neighbors <= 0` — `Status::invalid_argument`.
- `LabeledBatch` overload: cleaning removes every row, or removes an
  entire class outright — `Status::invalid_argument`.
- `batch.features` fails [`validate_features`](../data/FeatureBatch.md),
  or is not rank-2 — whatever that validation returns, or
  `Status::not_implemented` (`BlendNeighbors`/cleaning currently require
  a dense, rank-2 feature matrix).

## Practical Examples

All four `BalanceTargetKind` values, the two overloads proven identical
for a simple target, plain oversampling with replacement,
`BlendNeighbors` synthesis (including the small-class fallback),
`BoundaryPairs` cleaning verified by hand on a tiny constructed case,
the two `NeighborAgreement` vote rules contrasted, `protect_small_classes`
toggled, cleaning's target computed from the post-cleaning distribution,
and an error roundup.

### Example 1

`kind = Keep` (the default) — nothing changes size.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);

  const cls::BalancePlan plan = cls::plan_balance(labels, cls::BalanceTarget{}).value();
  std::cout << "target_counts (unchanged: [6, 2]) = " << plan.target_counts << "\n";
  std::cout << "indices.numel() (still 8) = " << plan.indices.numel() << "\n";
  std::cout << "uses_replacement = " << std::boolalpha << plan.uses_replacement << "\n";

  return 0;
}
```

Output

```text
target_counts (unchanged: [6, 2]) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 2]
indices.numel() (still 8) = 8
uses_replacement = false
```

### Example 2

`kind = Equalize` — every class grows to the current largest class's
count.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Equalize;
  cls::RandomState random{};
  random.seed = 1;
  const cls::BalancePlan plan = cls::plan_balance(labels, target, random).value();
  std::cout << "target_counts (both -> 6: [6, 6]) = " << plan.target_counts << "\n";
  std::cout << "after.counts = " << plan.after.counts << "\n";
  std::cout << "uses_replacement (class 1 needed duplicates) = " << std::boolalpha << plan.uses_replacement << "\n";

  return 0;
}
```

Output

```text
target_counts (both -> 6: [6, 6]) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 6]
after.counts = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 6]
uses_replacement (class 1 needed duplicates) = true
```

### Example 3

`kind = Counts` — an explicit, non-uniform target per class.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Counts;
  target.counts = pluss::make_series({4.0, 4.0}, {2}, f64);  // downsample class 0, oversample class 1
  cls::RandomState random{};
  random.seed = 2;
  const cls::BalancePlan plan = cls::plan_balance(labels, target, random).value();
  std::cout << "target_counts = " << plan.target_counts << "\n";
  std::cout << "after.counts = " << plan.after.counts << "\n";

  return 0;
}
```

Output

```text
target_counts = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 4]
after.counts = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4, 4]
```

### Example 4

`kind = Ratio` (two classes only) — the minority grows toward the ratio,
never past it, and never shrinks; the majority is untouched.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 0: 10 rows, class 1: 2 rows -- current ratio 0.2.
  std::vector<double> raw(10, 0.0);
  raw.push_back(1.0);
  raw.push_back(1.0);
  const pluss::Series labels = pluss::make_series(raw, {static_cast<std::int64_t>(raw.size())}, f64);

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Ratio;
  target.ratio = 0.5;  // desired minority >= ceil(0.5 * 10) = 5
  cls::RandomState random{};
  random.seed = 3;
  const cls::BalancePlan plan = cls::plan_balance(labels, target, random).value();
  std::cout << "target_counts (majority unchanged=10, minority grows to 5) = " << plan.target_counts << "\n";

  return 0;
}
```

Output

```text
target_counts (majority unchanged=10, minority grows to 5) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[10, 5]
```

### Example 5

The two overloads are byte-identical for a "simple" target (`clean =
None`, `growth = ReuseRows`) — the `LabeledBatch` overload just
delegates to the `Series`-only one internally.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0}, {8}, f64).reshape({8, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Equalize;
  cls::RandomState random{};
  random.seed = 5;

  const cls::BalancePlan via_labels = cls::plan_balance(batch.y, target, random).value();
  const cls::BalancePlan via_batch = cls::plan_balance(batch, target, random).value();
  const double diff = (via_labels.indices.to(pluss::ScalarType::Float64) - via_batch.indices.to(pluss::ScalarType::Float64))
                           .abs().amax(0, false).item<double>();
  std::cout << "max abs diff between the two overloads' indices = " << diff << "\n";

  return 0;
}
```

Output

```text
max abs diff between the two overloads' indices = 0
```

### Example 6

`growth = ReuseRows` explicitly oversampling — extra rows are real
duplicates (with replacement); `uses_replacement` records it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 100.0, 200.0}, {8}, f64).reshape({8, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Equalize;
  target.growth = cls::BalanceGrowthKind::ReuseRows;
  cls::RandomState random{};
  random.seed = 7;
  const cls::BalancePlan plan = cls::plan_balance(batch, target, random).value();
  std::cout << "uses_replacement = " << std::boolalpha << plan.uses_replacement << "\n";
  std::cout << "uses_synthesis (false -- ReuseRows never synthesizes) = " << plan.uses_synthesis << "\n";
  std::cout << "indices.numel() (12 = 6+6) = " << plan.indices.numel() << "\n";

  return 0;
}
```

Output

```text
uses_replacement = true
uses_synthesis (false -- ReuseRows never synthesizes) = false
indices.numel() (12 = 6+6) = 12
```

### Example 7

`growth = BlendNeighbors` — extra rows are synthesized by interpolating
between a real row and one of its nearest same-class neighbors (SMOTE);
a class with fewer than 2 rows silently falls back to `ReuseRows` for
itself alone, since no neighbor exists to blend toward.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 0: 5 rows spread out; class 1: exactly 1 row (too few to blend).
  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 100.0}, {6}, f64).reshape({6, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Counts;
  target.counts = pluss::make_series({8.0, 3.0}, {2}, f64);
  target.growth = cls::BalanceGrowthKind::BlendNeighbors;
  target.neighbors = 2;
  cls::RandomState random{};
  random.seed = 11;
  const cls::BalancePlan plan = cls::plan_balance(batch, target, random).value();

  std::cout << "uses_synthesis = " << std::boolalpha << plan.uses_synthesis << "\n";
  std::cout << "uses_replacement (class 1's fallback) = " << plan.uses_replacement << "\n";
  std::cout << "synthetic_labels = " << plan.synthetic_labels << "\n";
  std::cout << "synthetic_steps (each in [0,1)) = " << plan.synthetic_steps << "\n";

  return 0;
}
```

Output

```text
uses_synthesis = true
uses_replacement (class 1's fallback) = true
synthetic_labels = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 0, 0]
synthetic_steps (each in [0,1)) = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.061947905, 0.51502788, 0.092841096]
```

### Example 8

`clean = BoundaryPairs` on a tiny, hand-constructed case: two rows of
different classes are each other's mutual nearest neighbor — a textbook
boundary pair — and get removed.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // rows 2 and 3 sit right next to each other (distance 0.1) but carry
  // different labels, and each is the other's own nearest neighbor --
  // a boundary pair. Every other row is far from everything else.
  const pluss::Series x = pluss::make_series(
      {0.0, 10.0, 20.0, 20.1, 40.0, 50.0}, {6}, f64).reshape({6, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.clean = cls::BalanceCleanKind::BoundaryPairs;
  target.protect_small_classes = false;  // both classes are size 3, nothing to protect
  const cls::BalancePlan plan = cls::plan_balance(batch, target).value();
  std::cout << "uses_cleaning = " << std::boolalpha << plan.uses_cleaning << "\n";
  std::cout << "removed_indices (expect [2, 3]) = " << plan.removed_indices << "\n";

  return 0;
}
```

Output

```text
uses_cleaning = true
removed_indices (expect [2, 3]) = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[2, 3]
```

### Example 9

`clean = NeighborAgreement`: `AnyDisagreement` (strict — one disagreeing
neighbor is enough) removes more rows than `MajorityDisagreement` on the
same data. A stray label-1 point sits just outside a label-0 cluster,
close enough to be one of its edge row's 2 nearest neighbors: under
`AnyDisagreement` that single disagreement is enough to remove the edge
row too, but under `MajorityDisagreement` one disagreeing neighbor out
of two is not a majority, so the edge row survives.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // cluster 0 (label 0): 0..7; a stray label-1 point at -0.5 sits just
  // outside it, closest to row "0.0"; cluster 1 (label 1): 100,101,102,
  // far enough away to never affect anything in cluster 0.
  const pluss::Series x = pluss::make_series(
      {-0.5, 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 100.0, 101.0, 102.0},
      {12}, f64).reshape({12, 1});
  const pluss::Series y = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {12}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget any_vote{};
  any_vote.clean = cls::BalanceCleanKind::NeighborAgreement;
  any_vote.clean_neighbors = 2;
  any_vote.clean_vote = cls::BalanceCleanVote::AnyDisagreement;
  any_vote.protect_small_classes = false;
  const cls::BalancePlan any_plan = cls::plan_balance(batch, any_vote).value();

  cls::BalanceTarget majority_vote = any_vote;
  majority_vote.clean_vote = cls::BalanceCleanVote::MajorityDisagreement;
  const cls::BalancePlan majority_plan = cls::plan_balance(batch, majority_vote).value();

  std::cout << "AnyDisagreement removed_indices      = " << any_plan.removed_indices << "\n";
  std::cout << "MajorityDisagreement removed_indices = " << majority_plan.removed_indices << "\n";

  return 0;
}
```

Output

```text
AnyDisagreement removed_indices      = pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[0, 1]
MajorityDisagreement removed_indices = pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[0]
```

### Example 10

`protect_small_classes`: with it left at the default `true`, the
currently-smallest class's own row is protected — but a mutual boundary
pair has *two* rows, and only the small-class side is protected: its
majority-class partner is still eligible on its own and gets removed
anyway. Only with `protect_small_classes = false` does the small-class
row itself become eligible too — removing *both* sides of the pair here,
which wipes out the entire (size-1) class and fails outright.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 1 has only 1 row, sitting right at a boundary with class 0.
  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 4.1}, {6}, f64).reshape({6, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 1.0}, {6}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget protected_target{};
  protected_target.clean = cls::BalanceCleanKind::BoundaryPairs;
  // protect_small_classes defaults to true.
  const cls::StatusOr<cls::BalancePlan> protected_plan = cls::plan_balance(batch, protected_target);
  std::cout << "protect_small_classes=true  ok = " << std::boolalpha << protected_plan.ok() << "\n";
  if (protected_plan.ok()) {
    std::cout << "  removed_indices (row 4, class 0's side of the pair -- row 5 survives) = "
              << protected_plan.value().removed_indices << "\n";
  }

  cls::BalanceTarget unprotected_target = protected_target;
  unprotected_target.protect_small_classes = false;
  const cls::StatusOr<cls::BalancePlan> unprotected_plan = cls::plan_balance(batch, unprotected_target);
  std::cout << "protect_small_classes=false ok = " << unprotected_plan.ok() << "\n";
  if (!unprotected_plan.ok()) {
    std::cout << "  message = \"" << unprotected_plan.status().message << "\"\n";
  }

  return 0;
}
```

Output

```text
protect_small_classes=true  ok = true
  removed_indices (row 4, class 0's side of the pair -- row 5 survives) = pluss::Series(shape=[1], strides=[1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=[4]
protect_small_classes=false ok = false
  message = "plan_balance cleaning removed an entire class."
```

### Example 11

Cleaning runs **before** target counts are resolved, against the
**post-cleaning** distribution — `Equalize` targets the post-cleaning
majority, not the original one.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // class 0 starts at 6 rows; one is a boundary pair with class 1 and
  // will be cleaned away, leaving class 0 at 5.
  const pluss::Series x = pluss::make_series(
      {0.0, 1.0, 2.0, 3.0, 4.0, 20.0, 20.1, 40.0}, {8}, f64).reshape({8, 1});
  const pluss::Series y = pluss::make_series(
      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0}, {8}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;

  cls::BalanceTarget target{};
  target.kind = cls::BalanceTargetKind::Equalize;
  target.clean = cls::BalanceCleanKind::BoundaryPairs;
  target.protect_small_classes = false;
  const cls::BalancePlan plan = cls::plan_balance(batch, target).value();
  std::cout << "before.counts (original, pre-cleaning: [6, 2]) = " << plan.before.counts << "\n";
  std::cout << "removed_indices.numel() (the one boundary-pair row) = " << plan.removed_indices.numel() << "\n";
  std::cout << "target_counts (Equalize against the POST-cleaning max, 5 -- not the original 6) = " << plan.target_counts << "\n";

  return 0;
}
```

Output

```text
before.counts (original, pre-cleaning: [6, 2]) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 2]
removed_indices.numel() (the one boundary-pair row) = 2
target_counts (Equalize against the POST-cleaning max, 5 -- not the original 6) = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5, 5]
```

### Example 12

An error roundup: the `Series`-only overload rejecting `clean` and
`BlendNeighbors`, `Ratio` on three classes, an invalid `Counts` value,
and `BlendNeighbors` with `neighbors <= 0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const pluss::Series labels = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);

  cls::BalanceTarget wants_clean{};
  wants_clean.clean = cls::BalanceCleanKind::BoundaryPairs;
  std::cout << "Series-only overload + clean set ok = " << std::boolalpha << cls::plan_balance(labels, wants_clean).ok() << "\n";

  cls::BalanceTarget wants_blend{};
  wants_blend.growth = cls::BalanceGrowthKind::BlendNeighbors;
  std::cout << "Series-only overload + BlendNeighbors ok = " << cls::plan_balance(labels, wants_blend).ok() << "\n";

  const pluss::Series three_classes = pluss::make_series({0.0, 1.0, 2.0}, {3}, f64);
  cls::BalanceTarget ratio_target{};
  ratio_target.kind = cls::BalanceTargetKind::Ratio;
  std::cout << "Ratio with 3 classes ok = " << cls::plan_balance(three_classes, ratio_target).ok() << "\n";

  cls::BalanceTarget bad_counts{};
  bad_counts.kind = cls::BalanceTargetKind::Counts;
  bad_counts.counts = pluss::make_series({0.0, 4.0}, {2}, f64);  // 0 is < 1
  std::cout << "Counts with a value < 1 ok = " << cls::plan_balance(labels, bad_counts).ok() << "\n";

  const pluss::Series x = pluss::make_series({0.0, 1.0, 2.0, 3.0}, {4}, f64).reshape({4, 1});
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = labels;
  cls::BalanceTarget bad_neighbors{};
  bad_neighbors.growth = cls::BalanceGrowthKind::BlendNeighbors;
  bad_neighbors.neighbors = 0;
  std::cout << "BlendNeighbors with neighbors=0 ok = " << cls::plan_balance(batch, bad_neighbors).ok() << "\n";

  return 0;
}
```

Output

```text
Series-only overload + clean set ok = false
Series-only overload + BlendNeighbors ok = false
Ratio with 3 classes ok = false
Counts with a value < 1 ok = false
BlendNeighbors with neighbors=0 ok = false
```

## Changelog

- **0.1** — `plan_balance` (`Series` and `LabeledBatch` overloads).

## See Also

- [`BalancePlan`](BalancePlan.md) — the returned type, every field
  explained.
- [`BalanceTarget`](BalanceTarget.md) — every field this function reads.
- [`balance_rows`](balance_rows.md) — runs this function, then actually
  materializes the plan into a new `LabeledBatch`.
- [`class_mix`](class_mix.md) — the class distribution `before`/`after`
  and the target-count resolution are both built on.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
