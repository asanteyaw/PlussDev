<!-- pluss-hand-authored -->

# Series::StatAccessor::std

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::std(bool unbiased = true) const
Series Series::StatAccessor::std(std::int64_t dim, bool unbiased = true,
                               bool keepdim = false) const
Series SeriesStatProxy::std(bool unbiased = true) const
Series SeriesStatProxy::std(std::int64_t dim, bool unbiased = true,
                            bool keepdim = false) const
```

## What It Does

`std()` computes the square root of `var()` using the same axis, estimator, and
shape rules. It is usually easier to interpret than variance because its units
match the input: seconds remain seconds rather than seconds squared.

`unbiased=true` selects sample standard deviation (`N-1`); `false` selects
population standard deviation (`N`).

## Parameters And Returns

`dim` selects the reduced axis and accepts negative indexing. `unbiased`
chooses the sample or population denominator. `keepdim` retains a selected
axis at size one. The result is floating-point and uses the input's original
units.

As with `var()`, the global default currently has shape `[1]`, while the
explicit population form has scalar shape `[]`.

## Examples

Example 1. The default is global sample standard deviation.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.std() << "\n";
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1.8708287]
```

Example 2. Population standard deviation is the square root of `2.9166667`.

```cpp
std::cout << x.stat.std(false) << "\n";
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1.7078251
```

Example 3. Reducing `dim=1` reports one population spread per row.

```cpp
std::cout << x.stat().std(1, false) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.81649658, 0.81649658]
```

Example 4. Column-wise spread with `keepdim=true` produces shape `[1,3]`.

```cpp
std::cout << x.stat.std(0, false, true) << "\n";
```

### Output

```text
pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1.5, 1.5, 1.5]]
```

Squaring each result recovers the corresponding `var()` result, up to floating
rounding. `keepdim=true` is especially useful in manual normalization:
`(x - x.stat.mean(0, true)) / x.stat.std(0, false, true)`.

## Gradient

Away from zero spread, the population derivative is
`(x_i-mean)/(N*std)`, scaled by the upstream sensitivity. At an exactly
constant lane the mathematical derivative is not unique; do not rely on it as
a useful optimization signal.

## Gotchas And Failure Modes

- The default is sample, not population, standard deviation.
- Sample standard deviation rejects singleton lanes.
- Use `nanstd()` when and only when NaNs represent omitted observations.

## See Also

- [`Series::std`](../series/std.md): complete root-surface treatment.
- [`standardize`](standardize.md): subtract a mean and divide by population std.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="standardize.html"><span class="label">Previous</span><span class="title">standardize</span></a>
  <a class="next-link" href="var.html"><span class="label">Next</span><span class="title">var</span></a>
</div>
