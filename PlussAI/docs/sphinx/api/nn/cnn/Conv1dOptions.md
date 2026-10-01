# `pluss::ai::nn::Conv1dOptions`

The channel counts, kernel geometry, and where the layer's weights are
allocated.

Passed to [`Conv1d`](Conv1d.md)'s constructor -- or, more commonly, spelled
out as separate positional arguments, since `Conv1d` also has a
constructor that takes the geometry fields individually. See
[`Conv1d`](Conv1d.md)'s Construction section for both forms side by side.

## Fields

### `in_channels`

`std::int64_t in_channels = 1`
: The number of channels the input's second axis (`[N, C, L]` or `[C,
  L]`) must have. Must be divisible by `groups`.

### `out_channels`

`std::int64_t out_channels = 1`
: The number of channels the output produces -- `weight`'s first
  dimension, and the length of `bias` when `bias` is enabled.

### `kernel_size`

`std::int64_t kernel_size = 3`
: The width of the sliding window along the length axis. Must be `> 0`.

### `stride`

`std::int64_t stride = 1`
: How far the window moves between consecutive outputs. Must be `> 0`;
  larger values downsample the output length.

### `padding`

`std::int64_t padding = 0`
: Symmetric zero-padding added to both ends of the length axis before
  convolving. Must be `>= 0`. Ignored whenever `padding_lr` is non-zero on
  either side -- see `padding_lr` below.

### `padding_lr`

`pluss::ai::Padding1d padding_lr = {0, 0}` (a `std::array<std::int64_t, 2>`, `{left, right}`)
: Asymmetric padding, independently controlling the left and right ends of
  the length axis. When either entry is non-zero, this **overrides**
  `padding` entirely -- the two are not combined. This is what
  [`causal_padding1d`](../functional/causal_padding1d.md) produces (left
  padding only, sized so the layer never looks at future positions).

### `dilation`

`std::int64_t dilation = 1`
: The spacing between kernel taps. `1` (the default) is an ordinary dense
  kernel; larger values spread the same `kernel_size` taps over a wider
  receptive field without adding parameters. Must be `> 0`.

### `groups`

`std::int64_t groups = 1`
: Splits both `in_channels` and `out_channels` into `groups` independent
  chunks, each convolved separately -- `weight`'s second dimension becomes
  `in_channels / groups`, not `in_channels`. `groups == in_channels ==
  out_channels` is the depthwise-convolution case: every output channel
  sees exactly one input channel. Must divide `in_channels` evenly.

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
  `bias`'s). The same `seed` on the same geometry always reproduces the
  exact same initial weight and bias.

## Guidance

Set `in_channels`/`out_channels` to match the data; `kernel_size` is the
main architecture choice, with `stride`/`padding`/`dilation` tuning the
output length and receptive field. Reach for `padding_lr` specifically
when the padding needs to be asymmetric (most commonly: causal, via
[`causal_padding1d`](../functional/causal_padding1d.md)) -- for ordinary
symmetric padding, `padding` alone is simpler. Reach for `groups` to trade
capacity for a cheaper, more parameter-efficient layer (depthwise
convolution being the extreme case). Leave `cfg` at its default and use
`.to(...)` on the constructed module for platform/dtype changes.

## See Also

- [`Conv1d`](Conv1d.md) -- the only consumer of this options struct.
- [`ConvOptions`](ConvOptions.md) / [`Conv3dOptions`](Conv3dOptions.md) --
  the same fields generalized to 2 and 3 spatial axes.
