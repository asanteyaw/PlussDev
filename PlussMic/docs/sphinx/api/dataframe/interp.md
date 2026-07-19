<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1ef3d7a86725 -->

# DataFrame::interp

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::interp(pluss::RangeView<std::string_view> features = {}, std::int64_t limit = -1) const -> DataFrame

DataFrame::interp(const FillOptions& options) const -> DataFrame
```

## What It Does

Fills numeric gaps by linear interpolation between the nearest observed value on each
side. For a null between two observed values, `interp()` computes a straight-line
value between them weighted by position — the middle of a two-cell gap gets exactly
the midpoint, a cell one-third of the way across the gap gets one third of the way
between the two neighbors, and so on.

Unlike [`ffill()`](ffill.md) and [`bfill()`](bfill.md), `interp()` only ever operates
on numeric `Series` features — passing a non-numeric feature name explicitly throws.
A leading or trailing run of nulls with no observed value on *both* sides is left
`NULL`, because there is no second endpoint to interpolate toward.

## Parameters

**features** (`pluss::RangeView<std::string_view>`, optional)

Feature subset to interpolate. Leave empty (the default) to target every numeric
`Series` feature on the frame automatically.

**limit** (`std::int64_t`, default `-1`)

Maximum number of consecutive nulls to fill per gap, counted from the left edge of the
gap.

- `-1` (the default): no limit, fill every interior null that has observed values on
  both sides.
- A non-negative value: fill at most that many nulls per gap; any remaining nulls
  further into the gap stay `NULL`.

**options** (`FillOptions`)

Structured form combining `features` and `limit` in one object; equivalent to the
simple overload.

## Returns

`DataFrame`
    A new frame, same shape as the source, with interpolated values in the targeted
    numeric features. Untargeted and non-numeric features are unchanged.

## Examples

Example 1. Interpolate an interior gap with no limit.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"reading", pluss::make_series({
        std::numeric_limits<double>::quiet_NaN(),
        10.0,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        40.0,
        std::numeric_limits<double>::quiet_NaN(),
    })},
});

std::cout << readings;
```

### Output (before)

```text
==============
     | reading
--------------
0    | NULL   
1    | 10     
2    | NULL   
3    | NULL   
4    | 40     
5    | NULL   
--------------
[6 cs x 1 feat]
```

```cpp
auto interpolated = readings.interp();
std::cout << interpolated;
```

### Output (after)

```text
==============
     | reading
--------------
0    | NULL   
1    | 10     
2    | 20     
3    | 30     
4    | 40     
5    | NULL   
--------------
[6 cs x 1 feat]
```

Cases `2` and `3` sit between the observed `10` (case `1`) and `40` (case `4`), so they
land at the two evenly spaced points between them: `20` and `30`. Case `0` (before any
observed value) and case `5` (after the last observed value) stay `NULL` — there is no
second endpoint on the missing side to interpolate toward.

Example 2. Cap the fill with `limit = 1`.

```cpp
auto partially_interpolated = readings.interp({}, 1);
std::cout << partially_interpolated;
```

### Output

```text
==============
     | reading
--------------
0    | NULL   
1    | 10     
2    | 20     
3    | NULL   
4    | 40     
5    | NULL   
--------------
[6 cs x 1 feat]
```

With `limit = 1`, only the null closest to the left edge of the gap (case `2`) is
filled; case `3`, one step further into the same gap, stays `NULL`.

## Notes

- `interp()` only interpolates numeric `Series` features. Passing a non-numeric
  feature name in `features` throws.
- A gap needs an observed value on *both* sides to be filled at all — leading and
  trailing runs of nulls are always left `NULL`, regardless of `limit`.
- For copying a neighboring observation instead of computing a linear midpoint, see
  [`ffill()`](ffill.md) and [`bfill()`](bfill.md).

## See Also

- [`DataFrame::ffill`](ffill.md) / [`DataFrame::bfill`](bfill.md): propagate an
  existing observation instead of interpolating a new value.
- [`DataFrame::fillnull`](fillnull.md): write an explicit constant replacement value.
- [`DataFrame::dropnull`](dropnull.md): remove missing rows entirely instead of filling
  them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="insert.html">
    <span class="label">Previous</span>
    <span class="title">insert</span>
  </a>
  <a class="next-link" href="invalid.html">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>
