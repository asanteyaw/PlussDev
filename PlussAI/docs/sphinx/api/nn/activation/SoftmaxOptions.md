# `pluss::ai::nn::SoftmaxOptions`

The axis normalized over.

Passed to [`Softmax`](Softmax.md)'s constructor; has one field with a
default, so `SoftmaxOptions{}` (or omitting the argument entirely)
normalizes over the last axis.

## Fields

### `dim`

`std::int64_t dim = -1`
: The axis `forward` normalizes over: the entries along `dim` are
  exponentiated and divided by their sum, so they become non-negative and
  sum to `1` along that axis. Negative values count from the end, the same
  convention every other `Series` axis argument uses (`-1` is the last
  axis).

## Guidance

The default `-1` is the conventional choice for the common case where the
last axis holds per-class logits (shape `[..., num_classes]`). Set `dim`
explicitly when class logits live on a different axis, e.g. `dim=1` for a
`[batch, num_classes, ...]` layout such as per-pixel segmentation logits.

## See Also

- [`Softmax`](Softmax.md) -- the only consumer of this options struct.
- [`LogSoftmaxOptions`](LogSoftmaxOptions.md) -- the equivalent options
  struct for `log_softmax`.
