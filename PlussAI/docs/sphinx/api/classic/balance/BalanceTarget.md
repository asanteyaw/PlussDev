# `pluss::ai::classic::BalanceTarget`

The desired class mix for [`plan_balance`](plan_balance.md)/
[`balance_rows`](balance_rows.md) and how to reach it: which class counts
to aim for, whether extra rows are plain duplicates or synthesized by
blending real neighbors, and whether any rows are removed as
mislabeled-looking noise first. Also the type behind
[`TrainOptions::balance`](../core/TrainOptions.md#balance), the row
rebalancing every classifier applies before fitting.

## Fields

### `kind`

`BalanceTargetKind kind = BalanceTargetKind::Keep`

Which per-class target-count rule [`plan_balance`](plan_balance.md)
resolves before resampling/synthesizing:

- `Keep` (the default) — every class's target count is its own current
  count; nothing changes size. `TrainOptions::balance`'s own default
  `BalanceTarget{}` uses this, so a fit is unaffected unless you
  explicitly opt in.
- `Equalize` — every class's target count becomes the **largest**
  current class's own count.
- `Ratio` — **requires exactly two classes.** The minority class grows
  (never shrinks) until `minority / majority >= ratio`; the majority
  class's own count is left unchanged. See `ratio` below.
- `Counts` — the exact target count per class, read from `counts`
  below.

See [`plan_balance`'s Examples 1–4](plan_balance.md#example-1) for all
four demonstrated directly.

### `ratio`

`double ratio = 1.0`

Consulted only when `kind == Ratio`. Must be finite and in `(0, 1]` — a
minority/majority ratio to grow *toward*, never past (the desired
minority count is `max(current_minority, ceil(ratio * majority))`, so a
`ratio` at or below the class's current actual ratio changes nothing).

### `counts`

`pluss::Series counts = pluss::series::undefined()`

Consulted only when `kind == Counts`. Must be defined, length equal to
the number of distinct classes (in the same sorted-ascending order
[`class_mix`](class_mix.md) itself uses), every value finite and `>= 1`
— each value is rounded to the nearest integer target count.

### `growth`

`BalanceGrowthKind growth = BalanceGrowthKind::ReuseRows`

How a class that needs to *grow* gets its extra rows:

- `ReuseRows` (the default) — extra rows are duplicates of real rows
  from that class, sampled **with replacement**
  (`BalancePlan::uses_replacement` records when this actually happened).
  Works with either [`plan_balance`](plan_balance.md) overload.
- `BlendNeighbors` — extra rows are synthesized by picking a real row,
  one of its `neighbors` nearest same-class neighbors, and a random
  point on the line segment between them (the SMOTE algorithm) — see
  [`plan_balance`'s Example 6](plan_balance.md#example-6). **Requires
  the `LabeledBatch` overload** of `plan_balance` (real feature values
  are needed to find neighbors and interpolate); the `Series`-only
  overload rejects it outright. A class with fewer than 2 rows falls
  back to `ReuseRows` for that class alone, silently, since no neighbor
  exists to blend toward.

### `neighbors`

`std::int64_t neighbors = 5`

Consulted only when `growth == BlendNeighbors`. How many of a row's own
nearest same-class neighbors are candidates to interpolate toward. Must
be `> 0`. Silently capped at `class_size - 1` per class if larger — a
tiny class with `>= 2` but fewer than `neighbors + 1` rows still works,
using whatever neighbors actually exist.

### `clean`

`BalanceCleanKind clean = BalanceCleanKind::None`

Whether to remove rows that look mislabeled *before* resampling, using
each row's own nearest same-class-or-not neighbors in feature space:

- `None` (the default) — no cleaning.
- `BoundaryPairs` — remove a row whose single nearest neighbor is a
  **mutual** nearest neighbor (each is the other's own nearest) **and**
  carries a different label — a classic boundary/Tomek-link removal.
- `NeighborAgreement` — remove a row based on its `clean_neighbors`
  nearest neighbors' labels, per `clean_vote` below (Edited-Nearest-
  Neighbor style).
- `BoundaryPairsAndNeighborAgreement` — both criteria, combined (a row
  removed by either one is removed).

**Requires the `LabeledBatch` overload** of `plan_balance` — the
`Series`-only overload rejects any `clean` value other than `None`
outright. Cleaning always runs *before* the target counts are resolved,
against the **post-cleaning** class distribution, not the original one —
see [`plan_balance`'s Example 8](plan_balance.md#example-8) for exactly
what that means for `Equalize`/`Ratio` targets.

### `clean_neighbors`

`std::int64_t clean_neighbors = 3`

Consulted only when `clean` includes `NeighborAgreement`. How many
nearest neighbors (of any class) vote on a row's label. Must be `> 0`;
silently capped at `sample_count - 1`.

### `clean_vote`

`BalanceCleanVote clean_vote = BalanceCleanVote::AnyDisagreement`

Consulted only when `clean` includes `NeighborAgreement`:

- `AnyDisagreement` (the default) — remove the row if even **one** of
  its `clean_neighbors` nearest neighbors carries a different label (the
  strictest, most aggressive rule).
- `MajorityDisagreement` — remove the row only if **fewer than half**
  of its `clean_neighbors` nearest neighbors share its label.

### `protect_small_classes`

`bool protect_small_classes = true`

`true` (the default): a row belonging to whichever class currently has
the **fewest** rows is never eligible for cleaning removal, regardless
of what `clean`/`clean_vote` would otherwise flag — cleaning can shrink
a majority class but never the minority it exists to protect. `false`:
every row is eligible, including the smallest class's own.

## Guidance

Leave `BalanceTarget{}` (`kind = Keep`) alone unless a genuinely
imbalanced classification target is hurting the fit. Reach for
`kind = Equalize` or `kind = Ratio` (two classes only) as the ordinary
starting point; `kind = Counts` only when you need an exact, non-uniform
target (for instance, deliberately keeping one class rarer than the
others). Prefer `growth = BlendNeighbors` over the default `ReuseRows`
once a class needs to roughly double or more — plain duplication can
make a linear model overconfident on the duplicated exact points, while
blended neighbors add genuinely new, plausible rows. Reach for `clean`
only when you suspect real label noise near the class boundary, not as a
default — it can remove real, correctly-labeled but naturally-ambiguous
rows too, and always leave `protect_small_classes = true` (the default)
so cleaning cannot itself worsen the imbalance it runs before correcting.

## See Also

- [`plan_balance`](plan_balance.md) / [`balance_rows`](balance_rows.md) —
  the two consumers.
- [`class_mix`](class_mix.md) / [`ClassMix`](ClassMix.md) — the class
  distribution every target-count rule reads.
- [`TrainOptions`](../core/TrainOptions.md#balance) — `.balance`, where
  this struct reaches every classifier automatically.
