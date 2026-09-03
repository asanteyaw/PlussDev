<!-- pluss-hand-authored -->

# SeriesStatAccessor::normr

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::normr() const
Series SeriesStatProxy::normr() const
```

## What It Does

`normr()` scales each row to Euclidean length one while preserving shape and
direction. For row vector `r`, it computes `r / ||r||_2`. A rank-1 input is
treated as one vector. An all-zero row remains zero.

This is magnitude normalization, not statistical standardization: it does not
subtract a mean, and its output need not have zero mean or unit variance.

## Examples

```cpp
auto x = pluss::make_series({{3.0, 4.0}, {0.0, 5.0}});
std::cout << x.stat.normr() << "\n";
std::cout << x.stat().normr().square().sum(1) << "\n";
```

### Output

```text
data=[[0.6, 0.8],
  [0, 1]]
data=[1, 1]
```

The squared values in each nonzero row sum to one, proving unit L2 length.

Example 2. Zero vectors stay finite.

```cpp
auto x = pluss::make_series({{0.0, 0.0}, {3.0, 4.0}});
std::cout << x.stat.normr() << "\n";
```

### Output

```text
data=[[0, 0],
  [0.6, 0.8]]
```

## Gradient Geometry

For nonzero `r`, reverse mode projects sensitivity onto the tangent plane of
the unit sphere and divides by `||r||`. Changes parallel to `r` alter magnitude
only, so normalization removes that component.

## See Also

- [`normc`](normc.md): normalize columns instead.
- [`standardize`](standardize.md): center and scale by statistical spread.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normc.html"><span class="label">Previous</span><span class="title">normc</span></a>
  <a class="next-link" href="pdf.html"><span class="label">Next</span><span class="title">pdf</span></a>
</div>
