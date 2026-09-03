<!-- pluss-hand-authored -->

# Series::cumprod

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::cumprod(std::int64_t dim) const -> Series
Series::cumprod(std::int64_t dim, bool keepdim) const -> Series
Series& Series::cumprod_(std::int64_t dim)
Series& Series::cumprod_(std::int64_t dim, bool keepdim)
```

## What It Does

Computes a cumulative product along one axis.

Like [`cumsum()`](cumsum.md), this is a scan rather than a reduction. The shape
stays the same, but each position becomes the product of all earlier positions
in the lane, including itself.

For `[1, 2, 3]`, the rowwise cumulative product is `[1, 2, 6]`.

The prefix idea is the same as [`cumsum()`](cumsum.md), but multiplication
changes the behavior materially:

- early zeros wipe out later prefixes
- large values grow faster
- tiny values vanish faster

## Parameters

**dim** (`std::int64_t`)

The axis along which the running product is accumulated.

Important details:

- `dim = 0` scans down the first axis
- `dim = 1` scans across the second axis
- negative dims count from the back
- the result keeps the same shape because this is a scan, not a reduction

**keepdim** (`bool`)

Accepted for API parity, but currently ignored for shape because scans preserve
shape already.

## Returns

`Series` or `Series&`
    The prefix-product result, either as a fresh value or as the mutated
    current series for the underscore overloads.

## Gradient

As of the current implementation, `cumprod()` does **not** attach a reverse-mode
source link. The forward scan works, but downstream `reverse()` does not
populate source sensitivity through `cumprod()`.

That limitation is especially worth documenting because a multiplicative scan
is often expected to behave analogously to the fully differentiable
`cumsum()` path. Right now it does not.

## Examples

The examples on this page build incrementally.

Example 1. `dim = 1` computes rowwise running products.

```cpp
auto mat = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << mat.cumprod(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 6],
  [4, 20, 120]]
```

Example 2. Scanning down columns produces a different prefix structure.

```cpp
std::cout << mat.cumprod(0);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 3],
  [4, 10, 18]]
```

The first row is unchanged because each column scan starts there.

Example 3. Negative dimensions count from the back, so `-1` means the last
axis.

```cpp
std::cout << mat.cumprod(-1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 6],
  [4, 20, 120]]
```

On this shape, that matches `cumprod(1)` because the last axis is the row-local
column axis.

Example 4. `keepdim` is currently shape-neutral here too.

```cpp
std::cout << (mat.cumprod(1, false).sizes() == mat.cumprod(1, true).sizes() ? "true" : "false");
```

### Output

```text
true
```

Example 5. Once a zero appears in a lane, later prefixes in that lane stay zero.

```cpp
auto with_zero = pluss::make_series({
    {2.0, 0.0, 4.0},
    {3.0, 5.0, 1.0},
});

std::cout << with_zero.cumprod(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2, 0, 0],
  [3, 15, 15]]
```

This is one of the most important practical quirks of cumulative products:
after multiplication by zero, the later prefixes in that lane cannot recover.

Example 6. The in-place overload mutates the current `Series` but keeps the same
scan semantics.

```cpp
auto inplace = mat.clone();
inplace.cumprod_(1);

std::cout << inplace;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 6],
  [4, 20, 120]]
```

Example 7. The current reverse-mode path does not populate source sensitivity.

```cpp
auto tracked_cumprod = mat.clone();
tracked_cumprod.tracks_slope_(true);

auto scanned_prod = tracked_cumprod.cumprod(1);
scanned_prod.sum().reverse();

std::cout << "y=" << scanned_prod << "\n";
std::cout << "has_dx=" << (tracked_cumprod.has_sensitivity() ? "true" : "false") << "\n";
if (tracked_cumprod.has_sensitivity()) {
  std::cout << "dx=" << tracked_cumprod.sensitivity() << "\n";
}
```

### Output

```text
y=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 2, 6],
  [4, 20, 120]]
has_dx=false
```

## Gotchas And Failure Modes

- `cumprod()` is the multiplicative scan counterpart to [`cumsum()`](cumsum.md).
- `keepdim` is currently accepted but shape-neutral.
- Reverse-mode users should treat `cumprod()` as a current limitation point.
- Early zeros flatten later prefixes in the same lane.
- Choosing the wrong axis changes the meaning completely.
- A `dim` outside the valid axis range fails.

## Summary

Use `cumprod()` when you need every multiplicative prefix, not just the final
product:

- `cumprod(dim)` preserves shape and stores all prefixes
- `keepdim` is currently accepted but does not change shape
- zeros propagate forward through the rest of a lane
- `cumprod_` mutates the current `Series`
- reverse-mode support is currently a limitation on this surface

## See Also

- [`Series::cumsum`](cumsum.md): additive prefix scan with reverse-mode support.
- [`Series::prod`](prod.md): multiplicative reduction instead of scan.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cummin.html">
    <span class="label">Previous</span>
    <span class="title">cummin</span>
  </a>
  <a class="next-link" href="cumsum.html">
    <span class="label">Next</span>
    <span class="title">cumsum</span>
  </a>
</div>
