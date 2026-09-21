# `pluss::ai::classic::prep::QuantileMapOptions`

The output scale and clamp margin consumed by
[`QuantileMap::learn`](QuantileMap.md) / `map`.

## Fields

### `output`

`QuantileMapOutput output = QuantileMapOutput::Uniform`

The scale `map` produces:

- `Uniform` (the default) — the clamped percentile itself, in
  `[clip, 1 - clip]`.
- `Gaussian` — that percentile passed through the inverse normal CDF,
  landing on a standard-normal scale instead (unbounded, but concentrated
  near `0`).

### `clip`

`double clip = 1e-7`

How far the fitted percentile is kept from the extremes `0` and `1` before
`output` is applied — required to be in `[0, 0.5)`. Without any clamp, a
value at or beyond the fitted range's edge would produce a percentile of
exactly `0` or `1`, which `Gaussian` output would then map to `±infinity`;
the small default margin keeps that from happening while barely affecting
`Uniform` output's numeric range.

## Guidance

Leave `clip` at its default for `Uniform` output — it exists mainly to keep
`Gaussian` output finite. Raise it if you specifically want `Uniform`
output visibly pulled in from the extremes too (see
[`QuantileMap`'s Example 3](QuantileMap.md#example-3)), or if `Gaussian`
output's extreme values are inconveniently large in practice.

## See Also

- [`QuantileMap`](QuantileMap.md) — the consuming type.
- [`PowerMapOptions`](PowerMapOptions.md) — the equivalent options for the
  smooth-analytic alternative.
