<!-- pluss-hand-authored -->

# SeriesStatAccessor::standardize

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::standardize() const
Series SeriesStatAccessor::standardize(std::int64_t dim) const
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

## Examples

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.standardize() << "\n";
std::cout << x.stat().standardize(1) << "\n";
std::cout << x.stat.standardize(0) << "\n";
```

### Output

```text
data=[[-1.4638501, -0.87831007, -0.29277002],
  [0.29277002, 0.87831007, 1.4638501]]
data=[[-1.2247449, 0, 1.2247449],
  [-1.2247449, 0, 1.2247449]]
data=[[-1, -1, -1],
  [1, 1, 1]]
```

The column result is exactly `-1` and `1` because each two-value column lies
symmetrically around its own mean.

Example 2. Constant lanes map to zero.

```cpp
auto constant = pluss::make_series({{4.0, 4.0}, {9.0, 9.0}});
std::cout << constant.stat.standardize(1) << "\n";
```

### Output

```text
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
