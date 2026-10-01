# `pluss::ai::classic::ClassReportOptions`

The label order, zero-division fallback, sample weights, and
balance-quality tuning consumed by
[`classification_summary`](classification_summary.md).

## Fields

### `label_order`

`pluss::Series label_order = pluss::series::undefined()`

As [`ConfusionOptions::label_order`](ConfusionOptions.md#label_order) — the
class set and its order. Undefined (the default) discovers it as the sorted
union of every value seen.

### `zero_division`

`double zero_division = 0.0`

The value substituted whenever a ratio's denominator is exactly `0` —
precision with no predicted positives for a class, recall with no true
occurrences of a class, and so on. Must be exactly `0.0` or `1.0`; any other
value (including a value merely close to one of them) is rejected. `0.0`
(the default) is the conservative choice — an undefined ratio counts as the
worst possible score rather than the best.

### `sample_weights`

`pluss::Series sample_weights = pluss::series::undefined()`

As [`ConfusionOptions::sample_weights`](ConfusionOptions.md#sample_weights) —
per-row weights feeding the underlying confusion matrix, and therefore
every statistic derived from it.

### `balance_quality_alpha`

`double balance_quality_alpha = 0.1`

How strongly [`ClassReport::balance_quality`](ClassReport.md#state) (and its
micro/macro/weighted aggregates) penalizes a gap between recall and
specificity, beyond their geometric mean alone. `0.0` reduces
`balance_quality` to (`square_balance_quality`'s square of) the geometric
mean itself, with no adjustment for the gap. Must be finite and
non-negative.

### `square_balance_quality`

`bool square_balance_quality = true`

Whether `balance_quality`'s base term is the geometric mean of recall and
specificity (`false`) or its square (`true`, the default) — squaring makes
the base term equal to the product `recall * specificity` directly, and
generally makes `balance_quality` fall off faster as either recall or
specificity drops.

## Guidance

Leave `zero_division = 0.0` unless you have a specific reason an undefined
ratio should read as a perfect score instead of a failing one — `1.0`
mainly matters for a metric average you do not want dragged down by a class
with literally no predictions or no true occurrences. `balance_quality_alpha`
and `square_balance_quality` only matter if you are actually consulting
`balance_quality`; leave them at their defaults otherwise.

## See Also

- [`classification_summary`](classification_summary.md) — the consuming
  function.
- [`ClassReport`](ClassReport.md) — the returned report, including
  `balance_quality`.
- [`ConfusionOptions`](ConfusionOptions.md) — the shared `label_order` /
  `sample_weights` fields, documented in full there.
