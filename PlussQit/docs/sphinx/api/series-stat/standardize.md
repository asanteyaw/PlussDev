<!-- pluss-hand-authored -->

# SeriesStatAccessor::standardize

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::standardize() const
Series SeriesStatAccessor::standardize(std::int64_t dim) const
Series SeriesStatProxy::standardize() const
Series SeriesStatProxy::standardize(std::int64_t dim) const
```

## What It Does

`standardize()` converts values to population z-scores:

```text
z = (x - mean) / population_std
```

The output keeps the input shape. Nonconstant reduced lanes have mean zero and
population variance one. A constant lane has no meaningful scale; this surface
returns zeros rather than infinities or NaNs.

## Axis Defines The Reference Population

Global `standardize()` compares every element against one global mean and
spread. `standardize(0)` standardizes each column independently;
`standardize(1)` standardizes each row. Choose the axis from the semantics of
your data, not from whichever output looks most convenient.

## Parameters And Returns

`dim` selects independently standardized lanes and accepts negative indexing.
The output is floating-point, out-of-place, and always preserves shape. There
is no `unbiased` parameter: this transform consistently uses population
spread.

## Examples

Example 1. Global standardization uses one center and one scale for all six
values.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.standardize() << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.4638501, -0.87831007, -0.29277002],
  [0.29277002, 0.87831007, 1.4638501]]
```

Example 2. Row standardization gives each row its own mean and population
spread, making the two translated rows identical after normalization.

```cpp
std::cout << x.stat().standardize(1) << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.2247449, 0, 1.2247449],
  [-1.2247449, 0, 1.2247449]]
```

Example 3. Column standardization uses the two values in each column as a
separate population.

```cpp
std::cout << x.stat.standardize(0) << "\n";
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1, -1, -1],
  [1, 1, 1]]
```

The result is exactly `-1` and `1` because each two-value column lies
symmetrically around its own mean and population standard deviation.

Example 2. Constant lanes map to zero.

```cpp
auto constant = pluss::make_series({{4.0, 4.0}, {9.0, 9.0}});
std::cout << constant.stat.standardize(1) << "\n";
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, 0],
  [0, 0]]
```

## Gotchas And Failure Modes

- This uses population spread, not the default sample convention of `std()`.
- Standardizing rows and normalizing row norms answer different questions.
- Near-constant lanes can amplify floating noise even when exact constants are guarded.

## See Also

- [`Series::standardize`](../series/standardize.md): expanded derivation and examples.
- [`normr`](normr.md): unit Euclidean row length without mean removal.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="skewness.html"><span class="label">Previous</span><span class="title">skewness</span></a>
  <a class="next-link" href="std.html"><span class="label">Next</span><span class="title">std</span></a>
</div>
