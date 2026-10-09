<!-- pluss-hand-authored -->

# Series::StatAccessor::normr

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::normr() const
Series SeriesStatProxy::normr() const
```

## What It Does

`normr()` scales each row to Euclidean length one while preserving shape and
direction. For row vector `r`, it computes `r / ||r||_2`. A rank-1 input is
treated as one vector. An all-zero row remains zero.

This is magnitude normalization, not statistical standardization: it does not
subtract a mean, and its output need not have zero mean or unit variance.

## Returns And Call Styles

The result is an out-of-place floating `Series` with the owner's shape. Direct
`x.stat.normr()` and proxy `x.stat().normr()` spellings are equivalent; no peer
is bound for this unary operation.

## Examples

Example 1. Each row receives its own denominator.

```cpp
auto x = pluss::make_series({{3.0, 4.0}, {0.0, 5.0}});
std::cout << x.stat.normr() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.6, 0.8],
  [0, 1]]
```

The first row divides by `5`; the second also divides by `5`. Direction is
preserved even though magnitude is removed.

Example 2. Squared row components sum to one after normalization.

```cpp
std::cout << x.stat().normr().square().sum(1) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

The squared values in each nonzero row sum to one, proving unit L2 length.

Example 3. Zero vectors stay finite but cannot be made unit length.

```cpp
auto x = pluss::make_series({{0.0, 0.0}, {3.0, 4.0}});
std::cout << x.stat.normr() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, 0],
  [0.6, 0.8]]
```

## Gradient Geometry

For nonzero `r`, reverse mode projects sensitivity onto the tangent plane of
the unit sphere and divides by `||r||`. Changes parallel to `r` alter magnitude
only, so normalization removes that component.

Example 4. Reverse mode includes both the division and the input-dependent row
norm.

```cpp
auto x = pluss::make_series({{3.0, 4.0}, {5.0, 12.0}});
x.tracks_slope_(true);
x.stat.normr().sum().reverse();
std::cout << x.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0.032, -0.024],
  [0.038233955, -0.015930815]]
```

## See Also

- [`normc`](normc.md): normalize columns instead.
- [`standardize`](standardize.md): center and scale by statistical spread.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normc.html"><span class="label">Previous</span><span class="title">normc</span></a>
  <a class="next-link" href="pdf.html"><span class="label">Next</span><span class="title">pdf</span></a>
</div>
