# `pluss::ai::classic::prep::NeighborImputerOptions`

The neighbor count, weighting scheme, and distance floor consumed by
[`NeighborImputer::learn`](NeighborImputer.md) / `map`.

## Fields

### `neighbors`

`std::int64_t neighbors = 5`

How many of the closest overlapping training rows to average per missing
value. Must be `> 0`. If fewer than `neighbors` training rows actually
overlap with a query row on the relevant feature, every overlapping row
that exists is used instead — this is a cap, not a requirement to have
exactly that many.

### `weighting`

`NeighborImputerWeighting weighting = NeighborImputerWeighting::Distance`

How the kept neighbors are combined:

- `Uniform` — a plain mean of the `neighbors` closest values.
- `Distance` (the default) — inverse-distance weighted, so closer neighbors
  count more. A neighbor at squared distance `<= distance_floor` is treated
  as an exact match; once any exact match is present among the kept
  neighbors, farther ones are dropped from the average entirely rather than
  diluting an exact match with a small weighted contribution.

### `distance_floor`

`double distance_floor = 1e-12`

The squared-distance threshold below which a neighbor counts as an exact
match under `Distance` weighting (irrelevant to `Uniform`). Must be a
positive, finite number.

### `empty_feature_fallback`

`double empty_feature_fallback = 0.0`

The value a feature's fitted mean falls back to when the feature has zero
non-`NaN` training values — the same role
[`SimpleImputerOptions::empty_feature_fallback`](SimpleImputerOptions.md#empty_feature_fallback)
plays. Also the value used at `map` time (via the fitted mean, which itself
starts from this fallback) when a query row shares no overlapping feature
with any training row.

## Guidance

Raise `neighbors` for a smoother, more averaged fill; lower it toward `1`
for a fill that follows the single closest row more closely. Leave
`weighting = Distance` unless you specifically want every kept neighbor
weighted equally regardless of how much closer some are than others.

## See Also

- [`NeighborImputer`](NeighborImputer.md) — the consuming type.
- [`SimpleImputerOptions`](SimpleImputerOptions.md) — the equivalent
  fallback field on the simpler sibling imputer.
