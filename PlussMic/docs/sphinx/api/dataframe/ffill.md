<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1513f55ed159 -->

# DataFrame::ffill

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::ffill(std::int64_t limit = -1) const -> DataFrame

DataFrame::ffill(const FillOptions& options) const -> DataFrame
```

## What It Does

Forward-fills missing values: each null cell is replaced by the last *observed* value
above it in the same feature, propagating downward. `ffill()` works on every `Slate`
kind PlussMic supports — `Series`, `Note`, `Timeline`, and `TimeIndex` — not just
numeric features.

A leading run of nulls with no earlier observed value in the same feature is left
`NULL` — there is nothing above it to carry forward.

## Parameters

**limit** (`std::int64_t`, default `-1`)

Maximum number of consecutive nulls to fill per run, counted starting from the null
closest to the last observed value and moving away from it.

- `-1` (the default): no limit, fill every null that has an earlier observed value.
- A non-negative value: fill at most that many nulls per run; any remaining nulls
  further from the anchor stay `NULL`.

**options** (`FillOptions`)

Structured form for targeting a feature subset:

- `features`: the feature names to fill. Empty (the default) targets every feature on
  the frame.
- `limit`: same meaning as the simple overload's `limit`.

## Returns

`DataFrame`
    A new frame, same shape as the source, with nulls forward-filled in the targeted
    features. Untargeted features are unchanged.

## Examples

Example 1. Forward-fill a numeric feature with no run-length limit.

```cpp
using namespace pluss::table;

auto sensor = DataFrame({
    {"sales", pluss::make_series({
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        10.0,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::quiet_NaN(),
        20.0,
    })},
});

std::cout << sensor;
```

### Output (before)

```text
============
     | sales
------------
0    | NULL 
1    | NULL 
2    | 10   
3    | NULL 
4    | NULL 
5    | NULL 
6    | 20   
------------
[7 cs x 1 feat]
```

```cpp
auto filled = sensor.ffill();
std::cout << filled;
```

### Output (after)

```text
============
     | sales
------------
0    | NULL 
1    | NULL 
2    | 10   
3    | 10   
4    | 10   
5    | 10   
6    | 20   
------------
[7 cs x 1 feat]
```

Cases `0` and `1` stayed `NULL` — there is no earlier observed value to carry forward
into them. Cases `3` through `5` all picked up `10`, the last observed value above them.

Example 2. Cap the run length with `limit = 2`.

```cpp
auto partially_filled = sensor.ffill(2);
std::cout << partially_filled;
```

### Output

```text
============
     | sales
------------
0    | NULL 
1    | NULL 
2    | 10   
3    | 10   
4    | 10   
5    | NULL 
6    | 20   
------------
[7 cs x 1 feat]
```

With `limit = 2`, only the first two nulls after `10` (cases `3` and `4`) get filled;
case `5`, the third null in that same run, stays `NULL` because the run already used
up its two allowed fills.

## Notes

- `ffill()` propagates downward feature by feature; values never cross feature
  boundaries.
- Unlike [`interp()`](interp.md), `ffill()` is not restricted to numeric `Series`
  features — it works on `Note`, `Timeline`, and `TimeIndex` columns too.
- A leading run of nulls with no earlier observed value in that feature stays `NULL`,
  no matter the `limit`.
- For the opposite direction (propagating the next observed value backward), see
  [`bfill()`](bfill.md). For numeric linear interpolation between two observed
  neighbors, see [`interp()`](interp.md).

## See Also

- [`DataFrame::bfill`](bfill.md): propagate the next observed value backward instead
  of the previous one forward.
- [`DataFrame::interp`](interp.md): fill numeric gaps by interpolating between
  neighbors instead of copying one of them.
- [`DataFrame::fillnull`](fillnull.md): write an explicit replacement value instead of
  borrowing a neighboring observation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="features.html">
    <span class="label">Previous</span>
    <span class="title">features</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
