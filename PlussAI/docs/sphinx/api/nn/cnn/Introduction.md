# `nn::cnn` Introduction

## What This Category Owns

`cnn` owns the convolutional layers, one per spatial dimensionality:

- [`Conv1d`](Conv1d.md) -- a kernel sliding over a single axis. The
  natural choice for a 1-D signal (audio, a sensor time series, a
  character/token sequence treated positionally rather than recurrently).
- [`Conv`](Conv.md) (also [`Conv2d`, `ConvNd`](Conv.md#signature)) -- a
  kernel sliding over two axes. The conventional choice for images.
- [`Conv3d`](Conv3d.md) -- a kernel sliding over three axes. For genuinely
  volumetric data (video clips, medical scan volumes) where the third axis
  carries real spatial or temporal meaning.

All three share the same core idea and the same option field names
(`in_channels`, `out_channels`, `kernel_size`, `stride`, `padding`,
`dilation`, `groups`, `bias`) scaled to however many spatial axes that
dimensionality has -- learning any one of them transfers directly to the
other two.

## Why A Sliding Kernel Instead Of A Full Linear Map

[`Fusion`](../fusion/Fusion.md) connects every input feature to every
output feature with its own independent weight -- for a large image, that
means millions of parameters just to look at one layer's worth of pixels,
and no structural assumption that nearby pixels relate to each other.
Convolution makes the opposite trade: one small kernel, shared across
every window position, so the parameter count depends only on
`kernel_size`/`in_channels`/`out_channels`, never on the input's spatial
extent. This assumes nearby positions carry related information (true for
images, audio, and most spatial/temporal data) and is dramatically cheaper
when that assumption holds.

## The Shared Geometry Vocabulary

- `kernel_size` -- how large the sliding window is.
- `stride` -- how far the window moves between outputs; larger values
  downsample.
- `padding` -- zeros added to the input's edges before sliding, so the
  kernel can center on edge positions and/or the output can be kept the
  same size as the input.
- `dilation` -- spacing between kernel taps, widening the receptive field
  without adding parameters.
- `groups` -- splits channels into independent chunks, each convolved
  separately; `groups == in_channels == out_channels` is depthwise
  convolution, the cheapest form.

Every one of these fields follows the same output-size formula per axis:
`(padded_input - dilation * (kernel_size - 1) - 1) / stride + 1`.

## Batched And Unbatched Input, Automatically

All three layers accept either a batched input (a leading sample axis) or
a single unbatched sample, and detect which one was given purely by rank
-- `[N, C, L]` vs `[C, L]` for `Conv1d`, `[N, C, H, W]` vs `[C, H, W]` for
`Conv`, `[N, C, D, H, W]` vs `[C, D, H, W]` for `Conv3d`. There is no flag
to set; passing one fewer axis than the batched form is the entire
signal.

## Choosing Which One

1. Match the layer to the data's real dimensionality: `Conv1d` for
   sequences, `Conv` for images, `Conv3d` for volumes -- there is no
   general-purpose default among the three.
2. Start with `groups=1` (the default, full connectivity between input and
   output channels) and reach for a larger `groups` only when parameter
   count or compute budget is a real constraint.
3. Reach for asymmetric padding (`Conv1d`'s `padding_lr` field) only when
   the ordinary symmetric case genuinely does not fit -- most commonly, a
   causal layer that must never look at future positions, built via
   [`causal_padding1d`](../functional/causal_padding1d.md).

## Related Pages

- [`module` Introduction](../module/Introduction.md) -- the `Module`
  contract every type here implements, and how layers compose into a
  `Schedule`.
- [`fusion`](../fusion.md) -- the layer conventionally placed after a
  stack of convolutions has been flattened.
- [`activation`](../activation.md) -- the elementwise nonlinearities
  conventionally placed after each convolution.
