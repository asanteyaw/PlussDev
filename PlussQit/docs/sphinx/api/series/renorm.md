<!-- pluss-hand-authored -->

# Series::renorm

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::renorm(double p, std::int64_t dim, double maxnorm) const
Series Series::renorm(double p) const
Series& Series::renorm_(double p, std::int64_t dim, double maxnorm) const
```

## What It Does

Caps the norm of every sub-tensor along one axis.

Pick an axis `dim`. Each index along that axis selects a sub-tensor — for a
`[rows, cols]` matrix and `dim = 0`, that is each row. `renorm` computes the
`p`-norm of every such sub-tensor and, for any whose norm exceeds `maxnorm`,
scales the whole sub-tensor down so its norm lands exactly on `maxnorm`.
Sub-tensors already within the cap are passed through untouched.

The classic use is keeping the rows of an embedding table from growing without
bound during training: after each step, `weights.renorm_(2, 0, max_norm)` pulls
any over-long row back onto the ball of radius `max_norm` and leaves the rest
alone.

Three forms:

- **`renorm(p, dim, maxnorm)`** — the full form.
- **`renorm(p)`** — a convenience: `dim = 0`, `maxnorm = 1`. Caps each row at
  unit norm.
- **`renorm_(p, dim, maxnorm)`** — the in-place form. Mutates the receiver and
  returns a reference to it.

The norm of a sub-tensor is taken over **every axis except `dim`**, so for a
higher-rank input each `dim`-slice is flattened before its norm is measured.

## Parameters

**p** (`double`): which norm to constrain — `1`, `2`, any positive real. Must
be `> 0`.

**dim** (`std::int64_t`): the axis whose sub-tensors are normalized. Negative
values count from the end.

**maxnorm** (`double`): the ceiling. Sub-tensors with norm above this are
rescaled to hit it exactly; `maxnorm = 0` zeroes every non-zero sub-tensor.
Must be `>= 0`.

## Returns

`Series`
    Same shape and dtype as the input, in `float64` precision internally then
    cast back. `renorm_` returns `Series&` to the mutated receiver.

The input must have **at least 2 dimensions**.

## Gradient

Differentiable. For a sub-tensor left untouched the gradient passes straight
through. For a rescaled sub-tensor the scale factor `maxnorm / norm` itself
depends on the data, so the backward carries both the direct `scale * grad`
term and the contribution through the shrinking factor.

## Examples

Example 1. `dim = 0`: each row is a vector; the over-long ones are pulled in.

```cpp
auto x = pluss::make_series({{3.0, 4.0},     // 2-norm 5  -> rescaled
                             {0.3, 0.4},     // 2-norm 0.5 -> kept
                             {6.0, 8.0}});   // 2-norm 10 -> rescaled
std::cout << x.renorm(2.0, 0, 2.0);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1.2, 1.6],
  [0.3, 0.4],
  [1.2, 1.6]]
```

Row 0 was scaled by `2/5`, row 2 by `2/10`, row 1 (already under `2`) is
verbatim.

Example 2. Confirm the contract: each rescaled row now has norm exactly
`maxnorm`, each kept row is unchanged.

```cpp
auto x = pluss::make_series({{3.0, 4.0}, {0.3, 0.4}, {6.0, 8.0}});
auto y = x.renorm(2.0, 0, 2.0);
std::cout << "row 2-norms = " << y.mul(y).sum(1).sqrt();
```

### Output

```text
row 2-norms = pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 0.5, 2]
```

The cap is a ceiling, not a target — row 1 keeps its norm of `0.5`.

Example 3. `dim = 1` treats each column as the sub-tensor instead.

```cpp
auto x = pluss::make_series({{3.0, 0.3},
                             {4.0, 0.4}});   // col 0 norm 5, col 1 norm 0.5
std::cout << x.renorm(2.0, 1, 2.0);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.2, 0.3],
  [1.6, 0.4]]
```

Column 0 (norm `5`) is scaled by `2/5`; column 1 (norm `0.5`) is left alone.
Same data as Example 1 transposed, and the result is the transpose too.

Example 4. `p = 1` caps the sum of absolute values of each row.

```cpp
auto x = pluss::make_series({{1.0, -2.0, 3.0},   // L1 = 6 -> scaled by 4/6
                             {0.5,  0.5, 0.0}});  // L1 = 1 -> kept
std::cout << x.renorm(1.0, 0, 4.0);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.66666666, -1.3333333, 2],
  [0.5, 0.5, 0]]
```

`p` only changes which norm is measured and constrained; the rescale is still a
single scalar multiply per row, so signs and ratios within a row are preserved.

Example 5. The one-argument form: `p` only, with `dim = 0` and `maxnorm = 1`.

```cpp
auto x = pluss::make_series({{3.0, 4.0}, {0.6, 0.8}});
std::cout << x.renorm(2.0);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.59999999, 0.79999998],
  [0.6, 0.8]]
```

Row 0 (norm `5`) becomes a unit vector; row 1 already has norm `1`, so
`renorm` leaves it exactly as it was — the cap is `>`, not `>=`.

Example 6. Gradient flows through the scaling.

```cpp
auto x = pluss::make_series({{3.0, 4.0}, {6.0, 8.0}});
x.tracks_slope_(true);
x.renorm(2.0, 0, 2.0).sum().reverse();
std::cout << x.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.064000005, -0.04799999],
  [0.032000001, -0.023999998]]
```

The sensitivities are small and mixed in sign because rescaling a row does two
things at once: it shrinks the row (a positive push along the direction that
grows the sum) and it makes the shrink factor `maxnorm / norm` respond to the
row's own components (a negative pull back). Both rows started over the cap, so
both receive a gradient.

Example 7. A 1-D input is rejected — `renorm` needs an axis to hold the
sub-tensors and an axis to take the norm over.

```cpp
try {
  auto y = pluss::make_series({1.0, 2.0, 3.0}).renorm(2.0, 0, 1.0);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
renorm: expected input with at least 2 dimensions
```

Example 8. `maxnorm = 0` zeroes every non-zero sub-tensor.

```cpp
auto x = pluss::make_series({{3.0, 4.0}, {0.0, 0.0}});
std::cout << x.renorm(2.0, 0, 0.0);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, 0],
  [0, 0]]
```

Row 0 has norm `5 > 0` so it is scaled to zero; row 1 already has norm `0`, so
it is "within" the cap and untouched (still zero).

Example 9. The in-place form mutates the receiver and returns it.

```cpp
auto x = pluss::make_series({{3.0, 4.0}, {6.0, 8.0}});
x.renorm_(2.0, 0, 2.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1.2, 1.6],
  [1.2, 1.6]]
```

Both rows were over the cap, so both are pulled onto norm `2`.

## Gotchas And Failure Modes

- **The norm of a sub-tensor is taken over every axis except `dim`.** `dim` is
  the axis you are indexing *into* to pick a sub-tensor, not the axis you
  reduce. `renorm(2, 0, m)` normalizes rows; `renorm(2, 1, m)` normalizes
  columns.
- The cap is strict-greater: a sub-tensor whose norm equals `maxnorm` exactly
  is left alone, not rescaled.
- Input must be at least 2-D. To cap a single vector's norm use
  [`clamp_max`](clamp_max.md) on its [`norm`](norm.md), or
  `x.unsqueeze(0).renorm(...)`.
- `maxnorm = 0` collapses every non-zero sub-tensor to zeros.
- The computation runs in `float64` and casts back, so a low-precision input
  round-trips through `float64` (visible as trailing `...9999` in the examples).
- `renorm_` mutates in place — do not call it on a tensor that other live
  tensors view.

## Summary

`renorm(p, dim, maxnorm)` caps the `p`-norm of every sub-tensor along `dim`:

- each `dim`-slice with norm `> maxnorm` is scaled down to hit `maxnorm`
  exactly; the rest pass through
- the norm is measured over all axes except `dim`
- `renorm(p)` is the shorthand for `dim = 0`, `maxnorm = 1`
- `renorm_` is the in-place form
- input must be 2-D or higher; differentiable

## See Also

- [`Series::norm`](norm.md): the norm `renorm` measures and constrains.
- [`Series::dist`](dist.md): the other norm-constraint neighbor — the `p`-norm
  distance between two tensors.
- [`Series::clamp_max`](clamp_max.md): cap raw *values*; `renorm` caps a
  *norm*.
- [`Series::standardize`](standardize.md): a different per-row rescale — to
  unit variance rather than to a norm ceiling.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="remainder.html">
    <span class="label">Previous</span>
    <span class="title">remainder</span>
  </a>
  <a class="next-link" href="repeat.html">
    <span class="label">Next</span>
    <span class="title">repeat</span>
  </a>
</div>
