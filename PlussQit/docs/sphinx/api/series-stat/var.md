<!-- pluss-hand-authored -->

# Series::StatAccessor::var

[Back to Series::StatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series Series::StatAccessor::var(bool unbiased = true) const
Series Series::StatAccessor::var(std::int64_t dim, bool unbiased = true,
                               bool keepdim = false) const
Series SeriesStatProxy::var(bool unbiased = true) const
Series SeriesStatProxy::var(std::int64_t dim, bool unbiased = true,
                            bool keepdim = false) const
```

## What It Does

`var()` measures squared spread around a mean. The important choice is the
denominator: `unbiased=true` computes sample variance with `N-1`, while
`unbiased=false` computes population variance with `N`.

Use sample variance when observed values estimate a larger population. Use
population variance when the lane itself is the complete population, as in a
deterministic activation tensor.

## Axis And Estimator Together

Axis selection decides which values form one population; `unbiased` decides
how that population is normalized. Those are independent choices. On `[2,3]`,
`var(1, false)` computes two population variances, each from three values.

## Parameters And Returns

`dim` chooses the reduced axis and accepts negative indexing. `unbiased`
chooses divisor `N-1` when true and `N` when false. `keepdim` retains a
dimensional reduction axis at size one. The result is floating-point and uses
squared input units.

The global overloads currently expose different one-value geometries: the
default sample form has shape `[1]`, while `var(false)` has scalar shape `[]`.
Do not erase that distinction when writing shape-sensitive code.

## Examples

Example 1. The default computes sample variance over all six values.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.var() << "\n";
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[3.5]
```

The squared deviations total `17.5`; dividing by `6-1` gives `3.5`.

Example 2. Passing `false` treats the six values as the complete population.

```cpp
std::cout << x.stat.var(false) << "\n";
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=2.9166667
```

Example 3. Axis `1` creates one population per row. The proxy spelling is
semantically identical to calling through `x.stat` directly.

```cpp
std::cout << x.stat().var(1, false) << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66666667, 0.66666667]
```

Example 4. Axis `0` groups columns, and `keepdim=true` leaves that axis at
size one so the result can broadcast over `x`.

```cpp
std::cout << x.stat.var(0, false, true) << "\n";
```

### Output

```text
pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[2.25, 2.25, 2.25]]
```

Each column contains two values three units apart, so each has population
variance `2.25`.

## Gradient

For population variance, each value receives `2(x_i-mean)/N` times the
upstream sensitivity. Sample variance uses `N-1`. A constant lane therefore
has zero variance and zero gradient.

## Gotchas And Failure Modes

- Sample variance requires at least two values in every reduced lane.
- Variance is in squared units; use `std()` for original units.
- Ordinary `var()` propagates NaNs; use `nanvar()` only when NaN means missing.

## See Also

- [`Series::var`](../series/var.md): full root-surface examples and edge cases.
- [`std`](std.md): square root of the selected variance estimator.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="std.html"><span class="label">Previous</span><span class="title">std</span></a>
  <a class="next-link" href="../../series_api_reference.html"><span class="label">Next</span><span class="title">Series API Reference</span></a>
</div>
