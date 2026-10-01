# `pluss::ai::nn::ConvOptions`

The channel counts, kernel geometry, and where the layer's weights are
allocated -- the 2-D counterpart of [`Conv1dOptions`](Conv1dOptions.md).
Also spelled `Conv2dOptions`/`ConvNdOptions` (both plain aliases).

Passed to [`Conv`](Conv.md)'s constructor -- or, more commonly, spelled
out as separate positional arguments, since `Conv` also has a constructor
that takes the geometry fields individually. See [`Conv`](Conv.md)'s
Construction section for both forms side by side, including a real
difference in how easy each form is to call correctly.

## Fields

### `in_channels`

`std::int64_t in_channels = 1`
: The number of channels the input's second axis (`[N, C, H, W]` or `[C,
  H, W]`) must have. Must be divisible by `groups`.

### `out_channels`

`std::int64_t out_channels = 1`
: The number of channels the output produces -- `weight`'s first
  dimension, and the length of `bias` when `bias` is enabled.

### `kernel_size`

`std::array<std::int64_t, 2> kernel_size = {3, 3}`
: The `{height, width}` of the sliding window. Unlike
  [`Conv1dOptions::kernel_size`](Conv1dOptions.md), this is a pair, not a
  single value -- the two axes need not match.

### `stride`

`std::array<std::int64_t, 2> stride = {1, 1}`
: How far the window moves between consecutive outputs, per axis.

### `padding`

`std::array<std::int64_t, 2> padding = {0, 0}`
: Symmetric zero-padding added to both ends of each spatial axis, per
  axis. There is no asymmetric `padding_lr` equivalent here the way
  [`Conv1dOptions`](Conv1dOptions.md) has.

### `dilation`

`std::array<std::int64_t, 2> dilation = {1, 1}`
: The spacing between kernel taps, per axis.

### `groups`

`std::int64_t groups = 1`
: Splits both `in_channels` and `out_channels` into `groups` independent
  chunks, each convolved separately -- `weight`'s second dimension becomes
  `in_channels / groups`, not `in_channels`. Must divide `in_channels`
  evenly.

### `bias`

`bool bias = true`
: When `true` (the default), a learned `[out_channels]` bias is registered
  and added after the convolution. When `false`, no bias parameter is
  registered -- `bias()` stays an undefined `Series`.

### `cfg`

`pluss::Config cfg = {}`
: The dtype and platform `weight` (and `bias`, when enabled) are allocated
  with. Defaults to the engine's default `Config` (Float32 on CPU).

### `seed`

`std::uint64_t seed = 0`
: The seed behind `weight`'s random initialization (and, offset by one,
  `bias`'s).

## Guidance

Every geometry field here is a `{height, width}` pair -- pass `{n, n}` for
the common square-kernel case, or two different values when the layer
genuinely needs asymmetric geometry (a tall-and-narrow kernel over a
spectrogram, for instance). See [`Conv`](Conv.md)'s Construction section
for a real gotcha: passing a bare `{h, w}` brace literal compiles cleanly
through this struct's own aggregate initialization, but does **not**
deduce correctly when passed positionally to `Conv`'s non-`ConvOptions`
constructor overload.

## See Also

- [`Conv`](Conv.md) -- the only consumer of this options struct (also
  reachable as `Conv2d`/`ConvNd`).
- [`Conv1dOptions`](Conv1dOptions.md) / [`Conv3dOptions`](Conv3dOptions.md) --
  the same fields specialized to 1 and 3 spatial axes.
