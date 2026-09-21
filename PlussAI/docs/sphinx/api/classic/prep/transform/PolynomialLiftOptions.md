# `pluss::ai::classic::prep::PolynomialLiftOptions`

The maximum degree, interactions-only toggle, and bias-column toggle
consumed by [`PolynomialLift::learn`](PolynomialLift.md).

## Fields

### `degree`

`std::int64_t degree = 2`

The highest total degree of term to generate — `2` (the default) includes
every linear term, every square, and every pairwise cross term. Must be
`>= 1`. Output width grows combinatorially with both `degree` and the input
feature count; raise it deliberately, not by default.

### `interactions_only`

`bool interactions_only = false`

`false` (the default) includes pure-power terms (`x_i^2`, `x_i^3`, …, and
mixed terms with a repeated feature like `x_i^2 * x_j`). `true` restricts
every term to **distinct** feature indices — no feature multiplied by
itself anywhere in the term — keeping only genuine cross-feature
interactions.

### `include_bias`

`bool include_bias = false`

Whether a constant `1.0` column is prepended ahead of every generated term.
Useful when the downstream model does not fit its own intercept and needs
one represented explicitly as a feature instead.

## Guidance

Raise `degree` cautiously — the output width grows fast (combinatorially in
the feature count), and each additional degree multiplies the risk of
numerical scale problems for a downstream linear fit (pair this with
[`scaling`](../scaling.md) afterward). Reach for `interactions_only` when
you want cross-feature effects without the separate curvature terms
`degree`'s pure powers introduce.

## See Also

- [`PolynomialLift`](PolynomialLift.md) — the consuming type.
