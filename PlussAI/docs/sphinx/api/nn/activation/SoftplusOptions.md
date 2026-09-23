# `pluss::ai::nn::SoftplusOptions`

The beta sharpness and the linear-fallback threshold.

Passed to [`Softplus`](Softplus.md)'s constructor; both fields have a
default, so `SoftplusOptions{}` (or omitting the argument entirely)
reproduces the standard curve.

## Fields

### `beta`

`double beta = 1.0`
: Scales the curve's sharpness inside `(1/beta) * log(1 + exp(beta * x))`.
  Larger values sharpen the transition around `0` toward `ReLU`'s sharp
  corner; smaller values spread it out into a gentler curve. `beta <= 0` is
  not a meaningful activation shape and is not validated -- pass a positive
  value.

### `threshold`

`double threshold = 20.0`
: Once `beta * x` exceeds this value, `forward` returns `x` directly instead
  of computing the full formula -- numerically safer (avoids `exp` overflow
  for large `x`) and visually indistinguishable from the true curve out
  there. Lowering it trades a little accuracy very close to the transition
  for an earlier, cheaper linear fallback; raising it delays the fallback
  and relies on the underlying float type's range to avoid overflow.

## Guidance

The defaults reproduce the standard `softplus` curve. Raise `beta` when you
want `Softplus` to behave more like `ReLU` while staying differentiable
everywhere; there is rarely a reason to touch `threshold` unless profiling
shows the `exp` evaluation itself is a bottleneck at your working input
range.

## See Also

- [`Softplus`](Softplus.md) -- the only consumer of this options struct.
