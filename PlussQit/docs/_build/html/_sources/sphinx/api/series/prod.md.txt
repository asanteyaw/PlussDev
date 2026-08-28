<!-- pluss-hand-authored -->

# Series::prod

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::prod() const -> Series
Series::prod(std::int64_t dim) const -> Series
Series::prod(std::int64_t dim, bool keepdim) const -> Series
```

## What It Does

`prod()` multiplies values together, either across the whole `Series` or along
one chosen axis.

Shape-wise, `prod()` follows the same reduction rules as
[`sum()`](sum.md)/[`mean()`](mean.md):

- full reduction returns a scalar
- `prod(dim)` removes one axis
- `prod(dim, true)` keeps that axis as size `1`

The numerical intuition is different, though. `prod()` is very sensitive to
zeros and large magnitudes. One zero in a lane zeros the whole lane product.

That sensitivity is not an implementation quirk. It is the actual mathematical
character of a multiplicative reduction, so `prod()` deserves more caution than
`sum()` or `mean()`.

## Parameters

**dim** (`std::int64_t`)

The axis to reduce. Negative dims are normalized from the back.

**keepdim** (`bool`)

Whether the reduced axis remains as size `1`.

## Returns

`Series`
    A reduced series whose values are products rather than sums or means.

## Gradient

As of the current implementation, `prod()` does **not** attach a reverse-mode
source link. In other words, the forward computation works, but `reverse()` on
an expression containing `prod()` does not currently populate sensitivity on
the source series.

That is important enough to state plainly because the mathematical derivative of
`prod()` exists in the ordinary nonzero case, so users may reasonably expect it
to behave like `sum()` or `mean()`. Right now it does not.

## Examples

The examples on this page build incrementally.

Example 1. Full reduction multiplies the entire series into one scalar.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << mat.prod();
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=720
```

Example 2. `prod(dim)` multiplies one lane at a time.

```cpp
std::cout << mat.prod(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 120]
```

The first row product is `1 * 2 * 3 = 6`. The second row product is
`4 * 5 * 6 = 120`.

Example 3. `keepdim=true` preserves the reduced axis.

```cpp
std::cout << mat.prod(1, true);
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[6],
  [120]]
```

Example 4. Negative dimensions count from the back, so `-1` means the last
axis.

```cpp
std::cout << mat.prod(-1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 120]
```

On this shape, that matches the row-wise product because the last axis is the
row-local column axis.

Example 5. One zero is enough to zero the whole lane product.

```cpp
auto prod_with_zero = pluss::make_series({
    {2.0, 0.0, 4.0},
    {3.0, 5.0, 1.0},
});

std::cout << prod_with_zero.prod(1);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0, 15]
```

This is one of the main reasons `prod()` is numerically and algorithmically
less forgiving than `sum()` or `mean()`.

Example 6. The current reverse-mode path does not populate source sensitivity.

```cpp
auto tracked_prod = mat.clone();
tracked_prod.tracks_slope_(true);

auto produced = tracked_prod.prod(1);
produced.sum().reverse();

std::cout << "y=" << produced << "\n";
std::cout << "has_dx=" << (tracked_prod.has_sensitivity() ? "true" : "false") << "\n";
if (tracked_prod.has_sensitivity()) {
  std::cout << "dx=" << tracked_prod.sensitivity() << "\n";
}
```

### Output

```text
y=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[6, 120]
has_dx=false
```

That is current behavior, not a typo in the example.

## Gotchas And Failure Modes

- `prod()` has the same shape contract as other reductions.
- Zeros dominate multiplicatively.
- Reverse-mode users should treat `prod()` as a current limitation point.
- Large magnitudes can grow or underflow quickly because the operation is
  multiplicative rather than additive.
- A `dim` outside the valid axis range fails through the shared reduction path.

## Summary

Use `prod()` when multiplication itself is the statistic you need:

- `prod()` is the multiplicative analogue of `sum()`
- one zero wipes out the rest of a lane product
- `keepdim=true` is only about shape retention
- reverse-mode support is currently a limitation on this surface

## See Also

- [`Series::sum`](sum.md): additive reduction with full reverse-mode support.
- [`Series::mean`](mean.md): normalized additive reduction.
- [`Series::cumprod`](cumprod.md): prefix products without collapsing the axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="cumsum.html">
    <span class="label">Next</span>
    <span class="title">cumsum</span>
  </a>
</div>
