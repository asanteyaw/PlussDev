# `pluss::ai::nn::LogSoftmaxOptions`

The axis normalized over.

Passed to [`LogSoftmax`](LogSoftmax.md)'s constructor; has one field with a
default, so `LogSoftmaxOptions{}` (or omitting the argument entirely)
normalizes over the last axis.

## Fields

### `dim`

`std::int64_t dim = -1`
: The axis `forward` normalizes over -- the same axis argument
  [`SoftmaxOptions::dim`](SoftmaxOptions.md) takes, since `log_softmax` is
  `softmax` followed by `log`. Negative values count from the end (`-1` is
  the last axis).

## Guidance

The default `-1` is the conventional choice for the common case where the
last axis holds per-class logits. Set `dim` explicitly when class logits
live on a different axis -- see [`SoftmaxOptions`](SoftmaxOptions.md)'s
guidance, which applies identically here.

## See Also

- [`LogSoftmax`](LogSoftmax.md) -- the only consumer of this options struct.
- [`SoftmaxOptions`](SoftmaxOptions.md) -- the equivalent options struct for
  `softmax`.
