# `pluss::ai::nn::FusionOptions`

The layer's input/output width, whether it learns a bias, and where its
weights are allocated.

Passed to [`Fusion`](Fusion.md)'s constructor -- or, more commonly, spelled
out as separate positional arguments, since `Fusion` also has a
constructor that takes the same fields individually. See
[`Fusion`](Fusion.md)'s Construction section for both forms side by side.

## Fields

### `in_features`

`std::int64_t in_features = 1`
: The width of the input's last axis. `forward` requires every input's
  trailing dimension to equal this exactly; any number of leading
  (batch/sequence) dimensions are accepted.

### `out_features`

`std::int64_t out_features = 1`
: The width of the output's last axis -- the number of rows in `weight`,
  and the length of `bias` when `bias` is enabled.

### `bias`

`bool bias = true`
: When `true` (the default), a learned `[out_features]` bias vector is
  registered and added after the matrix multiply. When `false`, no bias
  parameter is registered at all -- `bias()` stays an undefined `Series`,
  not a zero-filled one, and `forward` computes the pure linear map with
  no additive shift.

### `cfg`

`pluss::Config cfg = {}`
: The dtype and platform `weight` (and `bias`, when enabled) are allocated
  with. Defaults to the engine's default `Config` (Float32 on CPU), the
  same default every other parameterized layer in this reference uses.

### `seed`

`std::uint64_t seed = 0`
: The seed behind `weight`'s random initialization (and, offset by one,
  `bias`'s). The same `seed` on the same `in_features`/`out_features`/`cfg`
  always reproduces the exact same initial weight and bias.

## Guidance

`in_features` must match whatever feeds this layer (the previous layer's
`out_features`, or the raw feature count for a first layer); `out_features`
is a genuine architecture choice with no formula. Leave `bias` at its
default `true` unless the layer is immediately followed by something that
makes an additive shift redundant (a normalization layer that re-centers
its input, for instance). Leave `cfg` at its default and use `.to(...)` on
the constructed module for platform/dtype changes. Set `seed` explicitly
only when exact reproducibility across runs matters.

## See Also

- [`Fusion`](Fusion.md) -- the only consumer of this options struct.
- [`EmbeddingOptions`](../embedding/EmbeddingOptions.md) -- another
  options struct with a `seed` field driving the same kind of
  uniform-bound initialization.
