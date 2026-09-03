<!-- pluss-hand-authored -->

# SeriesStatAccessor::var

[Back to SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference.md)

## Signatures

```cpp
Series SeriesStatAccessor::var(bool unbiased = true) const
Series SeriesStatAccessor::var(std::int64_t dim, bool unbiased = true,
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

## Examples

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << x.stat.var() << "\n";          // global sample
std::cout << x.stat.var(false) << "\n";     // global population
std::cout << x.stat().var(1, false) << "\n";
std::cout << x.stat.var(0, false, true) << "\n";
```

### Output

```text
data=[3.5]
data=2.9166667
data=[0.66666667, 0.66666667]
data=[[2.25, 2.25, 2.25]]
```

The first two values differ only because their denominators are `5` and `6`.
The final result retains `dim=0`, so shape `[1,3]` can broadcast over `x`.

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
