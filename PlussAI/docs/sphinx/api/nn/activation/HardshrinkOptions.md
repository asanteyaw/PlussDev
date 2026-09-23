# `pluss::ai::nn::HardshrinkOptions`

The zeroing threshold.

Passed to [`Hardshrink`](Hardshrink.md)'s constructor; has one field with a
default, so `HardshrinkOptions{}` (or omitting the argument entirely)
reproduces the standard curve.

## Fields

### `lambd`

`double lambd = 0.5`
: Everything in `[-lambd, lambd]` is zeroed; everything outside it passes
  through unchanged: `|x| > lambd ? x : 0`. The implementation takes
  `std::abs(lambd)` before comparing, so a negative value has the same
  effect as its positive counterpart.

## Guidance

The default `0.5` matches the conventional hard-shrinkage threshold. Raise
`lambd` to zero out a wider band around `0` (useful for sparsifying
activations more aggressively); lower it toward `0` to zero out less, which
in the limit `lambd=0` makes `Hardshrink` the identity function.

## See Also

- [`Hardshrink`](Hardshrink.md) -- the only consumer of this options struct.
