# `pluss::ai::nn::Conv3dOptions`

The channel counts, kernel geometry, and where the layer's weights are
allocated -- the 3-D counterpart of [`ConvOptions`](ConvOptions.md), with
every geometry field now a `{depth, height, width}` triple instead of a
pair.

Passed to [`Conv3d`](Conv3d.md)'s constructor -- or, more commonly,
spelled out as separate positional arguments. The same construction gotcha
[`Conv`](Conv.md) documents applies here too: a bare `{d, h, w}` brace
literal does not deduce through the positional constructor's generic
forwarding, so an explicit `std::array<std::int64_t, 3>` is required
there; the `Conv3dOptions` form has no such restriction.

## Fields

### `in_channels`

`std::int64_t in_channels = 1`
: The number of channels the input's second axis (`[N, C, D, H, W]` or
  `[C, D, H, W]`) must have. Must be divisible by `groups`.

### `out_channels`

`std::int64_t out_channels = 1`
: The number of channels the output produces -- `weight`'s first
  dimension, and the length of `bias` when `bias` is enabled.

### `kernel_size`

`std::array<std::int64_t, 3> kernel_size = {3, 3, 3}`
: The `{depth, height, width}` of the sliding window.

### `stride`

`std::array<std::int64_t, 3> stride = {1, 1, 1}`
: How far the window moves between consecutive outputs, per axis.

### `padding`

`std::array<std::int64_t, 3> padding = {0, 0, 0}`
: Symmetric zero-padding added to both ends of each spatial axis, per
  axis.

### `dilation`

`std::array<std::int64_t, 3> dilation = {1, 1, 1}`
: The spacing between kernel taps, per axis.

### `groups`

`std::int64_t groups = 1`
: Splits both `in_channels` and `out_channels` into `groups` independent
  chunks, each convolved separately -- `weight`'s second dimension becomes
  `in_channels / groups`, not `in_channels`.

### `bias`

`bool bias = true`
: When `true` (the default), a learned `[out_channels]` bias is registered
  and added after the convolution.

### `cfg`

`pluss::Config cfg = {}`
: The dtype and platform `weight` (and `bias`, when enabled) are allocated
  with. Defaults to the engine's default `Config` (Float32 on CPU).

### `seed`

`std::uint64_t seed = 0`
: The seed behind `weight`'s random initialization (and, offset by one,
  `bias`'s).

## Guidance

Every geometry field is a `{depth, height, width}` triple -- pass `{n, n,
n}` for the common cubic-kernel case. `Conv3d` is the layer to reach for
over [`Conv`](Conv.md) specifically for genuinely volumetric data (video
clips, medical scan volumes) where the third axis carries real spatial or
temporal meaning, not just an extra batch-like dimension.

## See Also

- [`Conv3d`](Conv3d.md) -- the only consumer of this options struct.
- [`Conv1dOptions`](Conv1dOptions.md) / [`ConvOptions`](ConvOptions.md) --
  the same fields specialized to 1 and 2 spatial axes.
