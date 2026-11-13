<!-- pluss-hand-authored -->

# Series::nansum

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::nansum() const
Series Series::nansum(std::int64_t dim) const
Series Series::nansum(std::int64_t dim, bool keepdim) const
```

## What It Does

`nansum()` adds up the values along an axis, treating every `NaN` as `0`.

It is the missing-data version of [`sum()`](sum.md): where `sum()` returns
`NaN` the moment a single `NaN` enters the total, `nansum()` skips those
positions and totals the rest. A reduced axis with no finite values at all sums
to `0`.

The no-argument form totals the whole series to a scalar. The `dim` forms
reduce one axis, with `keepdim` controlling whether it is kept as a length-1
dimension.

## Parameters

**dim** (`std::int64_t`, optional)

The axis to reduce. Negative values count from the back. Omitted: reduce
everything to a scalar.

**keepdim** (`bool`, default `false`)

Keep the reduced axis as size `1` instead of removing it.

## Returns

`Series`
    The `NaN`-skipping total. A scalar for the no-argument form; otherwise the
    input shape with `dim` removed (or kept as `1`). An all-`NaN` reduced axis
    gives `0`.

## Gradient

`nansum()` is differentiable. The backward carries the upstream sensitivity to
every finite position and `0` to the `NaN` positions:

```
d/dx_i nansum(x) = 0 if x_i is NaN, else 1
```

This is the behavior you want for masked or missing data — the skipped entries
receive no gradient rather than a `NaN` one.

## Examples

Example 1. `NaN` entries are skipped, not propagated.

```cpp
double nan = std::nan("");

pluss::series::print(
    std::cout,
    pluss::make_series({1.0, nan, 3.0, nan, 5.0}).nansum());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=9.0000
```

`1 + 3 + 5 = 9`. Plain [`sum()`](sum.md) here would be `NaN`.

Example 2. A reduced axis with no finite values sums to `0`.

```cpp
double nan = std::nan("");

pluss::series::print(std::cout, pluss::make_series({nan, nan}).nansum());
```

### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0.0000
```

Example 3. Reducing one axis of a matrix.

```cpp
double nan = std::nan("");
auto m = pluss::make_series({{1.0, nan}, {3.0, 4.0}});

pluss::series::print(std::cout, m.nansum(0));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[4.0000, 4.0000]
```

Column `0` is `1 + 3`; column `1` skips the `NaN` and keeps just `4`.

Example 4. The backward carries `1` to finite positions and `0` to `NaN`
positions.

```cpp
double nan = std::nan("");
auto x = pluss::make_series({1.0, nan, 3.0, 5.0});
x.tracks_slope_(true);

x.nansum().reverse();

std::cout << "dx=" << x.sensitivity();
```

### Output

```text
dx=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 0, 1, 1]
```

The `NaN` position gets `0`; every finite position gets the upstream `1`.

## Gotchas And Failure Modes

- `nansum()` of an all-`NaN` axis is `0`, which is indistinguishable from a
  finite total that happens to be `0`. Pair it with a count if you need to tell
  them apart.
- `+inf` and `-inf` are *not* skipped; only `NaN` is.
- Floating-point input.

## Summary

`nansum(x)` totals an axis with `NaN` treated as `0`:

- all-`NaN` axis reduces to `0`
- `dim` / `keepdim` behave like the other reductions
- differentiable, backward is `1` on finite positions and `0` on `NaN`
  positions

## See Also

- [`Series::sum`](sum.md): the plain total, which returns `NaN` on any `NaN`
  input.
- [`Series::nanmean`](nanmean.md): the `NaN`-skipping average.
- [`Series::nan_to_num`](nan_to_num.md): replace `NaN` (and infinities)
  explicitly before an ordinary reduction.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nanstd.html">
    <span class="label">Previous</span>
    <span class="title">nanstd</span>
  </a>
  <a class="next-link" href="nanvar.html">
    <span class="label">Next</span>
    <span class="title">nanvar</span>
  </a>
</div>
