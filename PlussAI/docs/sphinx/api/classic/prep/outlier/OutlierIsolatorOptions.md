# `pluss::ai::classic::prep::OutlierIsolatorOptions`

The forest size, per-tree sampling, and anomaly-threshold fields consumed by
[`OutlierIsolator::learn`](OutlierIsolator.md).

## Fields

### `estimators`

`std::int64_t estimators = 100`

The number of randomized trees in the forest. Must be `> 0`. More trees
give a smoother, more stable score at proportionally higher fit and
`score_samples`/`infer` cost; the default is the conventional choice for
isolation forests.

### `max_samples`

`std::int64_t max_samples = 256`

The number of rows each tree is built from, when `max_samples_auto` is
`false`. Must be `> 0`; clamped down to the training batch's actual sample
count if smaller. Ignored when `max_samples_auto` is `true`.

### `max_samples_auto`

`bool max_samples_auto = true`

`true` (the default) uses `min(256, sample_count)` per tree — the
conventional isolation-forest default, chosen because isolation trees need
surprisingly few rows to separate an anomaly from the bulk of the data.
Set `false` to control `max_samples` directly.

### `max_features_ratio`

`double max_features_ratio = 1.0`

The fraction of features each tree's splits are drawn from. Must be in
`(0, 1]`. `1.0` (the default) lets every tree consider every feature;
lowering it introduces feature-subsampling diversity across trees, the same
role a random forest's own feature subsampling plays.

### `bootstrap`

`bool bootstrap = false`

Whether each tree's row sample is drawn with replacement (`true`) or
without (`false`, the default). Bootstrap sampling lets the same row appear
more than once in a single tree's sample.

### `contamination`

`double contamination = 0.1`

The expected fraction of anomalous rows, used to fit `decision_offset()`
when `contamination_auto` is `false` — the offset becomes this quantile of
the *training batch's own* fitted scores. Must be in `(0, 0.5]`. Ignored
when `contamination_auto` is `true`.

### `contamination_auto`

`bool contamination_auto = true`

`true` (the default) fits `decision_offset()` to a fixed `-0.5` — a
data-independent "auto" convention, unaffected by the training data's own
score distribution. Set `false` to fit the offset from
`contamination` directly instead, when you have a specific expected anomaly
rate in mind.

### `random`

`RandomState random = {}`

The seed/determinism pair controlling every random draw during fitting —
row and feature subsampling, and the random split points and features
chosen at every tree node. See [`RandomState`](../../core/RandomState.md).

## Guidance

Leave `estimators`/`max_samples_auto`/`max_features_ratio`/`bootstrap` at
their defaults for the ordinary case — they match the conventional
isolation-forest configuration. `contamination_auto` is the field worth a
deliberate choice: leave it `true` unless you have a genuine estimate of
the anomaly rate in your data, in which case setting it `false` with a
matching `contamination` gives a threshold calibrated to that rate instead
of the fixed convention.

## See Also

- [`OutlierIsolator`](OutlierIsolator.md) — the consuming type.
- [`RandomState`](../../core/RandomState.md) — the seed/determinism pair.
