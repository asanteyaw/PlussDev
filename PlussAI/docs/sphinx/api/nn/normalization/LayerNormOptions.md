# `pluss::ai::nn::LayerNormOptions`

The normalized width, the variance-floor epsilon, and where the learned
`weight`/`bias` are allocated.

Passed to [`LayerNorm`](LayerNorm.md)'s constructor. Unlike every options
struct in [`activation`](../activation.md), `normalized_size` has no
sensible default (`0` is not a usable width) -- `LayerNorm` has no
no-argument constructor, so a `LayerNormOptions` with at least
`normalized_size` set must always be passed explicitly.

## Fields

### `normalized_size`

`std::int64_t normalized_size = 0`
: The width of the trailing axis `forward` normalizes over, and the shape
  of both `weight` and `bias` (`[normalized_size]`). Must match the size of
  `input`'s last axis at call time, or `forward`'s broadcast against
  `weight`/`bias` throws.

### `eps`

`double eps = 1e-5`
: Added to the per-row variance before taking the reciprocal square root,
  so a row whose values are all identical (variance exactly `0`) still
  produces a finite result instead of dividing by zero. Large enough values
  measurably shrink the normalized output's scale below `1`; the default is
  small enough to be invisible except at extreme precision.

### `cfg`

`pluss::Config cfg = {}`
: The dtype and platform `weight` and `bias` are allocated with. Defaults
  to the engine's default `Config` (Float32 on CPU, matching
  [`PReLUOptions::cfg`](../activation/PReLUOptions.md)'s default). Moving
  the whole module afterward with `.to(...)` keeps `options().cfg` in sync
  automatically -- see [`LayerNorm`](LayerNorm.md)'s State section.

## Guidance

Set `normalized_size` to match the width of whatever axis is being
normalized -- almost always the feature/channel dimension coming out of the
preceding layer. Leave `eps` at its default unless a row of exactly
constant values is a real possibility in your data (constant rows are the
only case `eps` meaningfully affects). Leave `cfg` at its default and use
`.to(...)` on the constructed module for platform/dtype changes, the same
as every other parameterized layer in this reference.

## See Also

- [`LayerNorm`](LayerNorm.md) -- the only consumer of this options struct.
