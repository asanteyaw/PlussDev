<!-- pluss-hand-authored -->

# Series::mean

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::mean() const -> Series
Series::mean(std::int64_t dim) const -> Series
Series::mean(std::int64_t dim, bool keepdim) const -> Series
```

## What It Does

Computes the arithmetic mean, either over the whole series or along one axis.

Shape-wise, `mean()` behaves like [`sum()`](sum.md): full reduction returns a
scalar, and `mean(dim, keepdim)` removes or preserves the reduced axis
depending on `keepdim`.

The main extra detail is dtype behavior. A mean is inherently fractional, so
integral inputs are promoted in the current CPU implementation instead of
staying integral.

## Parameters

**dim** (`std::int64_t`)

The axis to reduce. Negative dims are accepted and count from the back.

**keepdim** (`bool`)

Whether the reduced axis remains as size `1`.

## Returns

`Series`
    A reduced series whose values are arithmetic means rather than totals.

## Gradient

`mean()` is differentiable.

Its backward is the same broadcast pattern as `sum()`, but scaled by the size
of the reduced lane:

- full mean over `N` elements sends `1 / N` to each element
- `mean(dim, ...)` sends `1 / size(dim)` to each element in the reduced lane

This is why `mean()` is often the more stable training objective spelling when
you do not want gradient magnitude to grow with batch size or sequence length.

## Examples

The examples on this page build incrementally.

Example 1. Full reduction returns one scalar mean.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << mat.mean();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3.5
```

Example 2. Reducing along `dim = 0` averages column-by-column.

```cpp
std::cout << mat.mean(0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.5, 3.5, 4.5]
```

This is the usual "column means" interpretation for a matrix.

Example 3. `keepdim=true` preserves the reduced axis for later broadcasting.

```cpp
std::cout << mat.mean(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[2],
  [5]]
```

Example 4. Integral input promotes to floating output in the current CPU path.

```cpp
auto int_mat = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
});

std::cout << int_mat.mean();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float32, platform=cpu, numel=1, contiguous=true)
data=3.5
```

That promotion is worth knowing because it affects downstream dtype expectations.
You are no longer in integer space after the mean.

Example 5. The backward pass divides the upstream sensitivity by the reduced
lane length.

```cpp
auto tracked_mean = mat.clone();
tracked_mean.tracks_slope_(true);

auto averaged = tracked_mean.mean(1);
averaged.sum().reverse();

std::cout << "y=" << averaged << "\n";
std::cout << "dx=" << tracked_mean.sensitivity();
```

### Output

```text
y=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[2, 5]
dx=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[0.33333333, 0.33333333, 0.33333333],
  [0.33333333, 0.33333333, 0.33333333]]
```

Each row has length `3`, so each contributing element receives `1 / 3`.

## Notes

- `mean()` follows `sum()`'s shape rules.
- Integer input does not imply integer output.
- `mean()` is often preferable to `sum()` inside losses because the gradient is
  normalized by lane size.

## See Also

- [`Series::sum`](sum.md): same reduction geometry without normalization.
- [`Series::prod`](prod.md): multiplicative reduction instead of additive.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="prod.html">
    <span class="label">Next</span>
    <span class="title">prod</span>
  </a>
</div>
